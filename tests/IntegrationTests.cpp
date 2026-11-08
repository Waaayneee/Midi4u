#include "DeviceSupervisor.h"
#include "Engine.h"
#include "SharedMemory.h"
#include "TapPluginSide.h"
#include "TestUtils.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <vector>

using namespace midi4u;
using namespace testutils;

namespace
{
// A device slot whose devices can be unplugged and replugged by the test.
class FakeEndpoint final : public AudioEndpoint
{
public:
    std::vector<DeviceInfo> devices;
    std::optional<DeviceInfo> opened;
    bool failOpen = false;
    int openCalls = 0;
    int listCalls = 0;
    uint64_t callbacks = 0;

    std::vector<DeviceInfo> listDevices() override
    {
        ++listCalls;
        return devices;
    }

    std::optional<DeviceInfo> defaultDevice() override
    {
        return devices.empty() ? std::nullopt : std::optional(devices.front());
    }

    std::string open(const DeviceInfo& device, double) override
    {
        ++openCalls;
        if (failOpen)
            return "Device is busy";
        opened = device;
        return {};
    }

    void close() override { opened.reset(); }
    uint64_t callbackCount() const override { return callbacks; }

    // One audio callback, if the opened device is still plugged in.
    void pump()
    {
        if (opened && std::any_of(devices.begin(), devices.end(), [&](const auto& d) { return d.id == opened->id; }))
            ++callbacks;
    }

    void unplug(const std::string& id)
    {
        devices.erase(std::remove_if(devices.begin(), devices.end(), [&](const auto& d) { return d.id == id; }), devices.end());
    }
};

// Runs two independent clocks (for example Ableton and the output device) side by side.
struct Clocks
{
    double now = 0; // seconds

    void run(double seconds, double rateA, int blockA, const std::function<void()>& a,
             double rateB, int blockB, const std::function<void()>& b)
    {
        const double end = now + seconds;
        uint64_t ticksA = 0, ticksB = 0;
        const double start = now;

        for (;;)
        {
            const double tA = start + double(ticksA) * blockA / rateA;
            const double tB = start + double(ticksB) * blockB / rateB;

            if (tA >= end && tB >= end)
                break;

            if (tA <= tB)
            {
                now = tA;
                if (a)
                    a();
                ++ticksA;
            }
            else
            {
                now = tB;
                if (b)
                    b();
                ++ticksB;
            }
        }

        now = end;
    }

    uint64_t ms() const { return uint64_t(now * 1000.0); }
};

// Renders the engine like an output device and keeps what it produced.
struct OutputRecorder
{
    Engine& engine;
    std::vector<float> left = std::vector<float>(480), right = std::vector<float>(480);
    std::vector<float> recorded;

    void operator()()
    {
        engine.renderOutput(left.data(), right.data(), 480);
        recorded.insert(recorded.end(), left.begin(), left.end());
    }

    std::vector<float> last(double seconds, double rate = 48000) const
    {
        const size_t n = std::min(recorded.size(), size_t(seconds * rate));
        return {recorded.end() - long(n), recorded.end()};
    }
};
} // namespace

// ---------------------------------------------------------------- Device supervisor

TEST_CASE("Unplugged mic goes red, never switches, and comes back when replugged", "[integration][devices]")
{
    FakeEndpoint mic;
    mic.devices = {{"A", "Headset Microphone"}, {"B", "Laptop Microphone"}};
    DeviceSupervisor supervisor(mic);
    uint64_t t = 0;

    auto advance = [&](uint64_t ms) {
        for (uint64_t elapsed = 0; elapsed < ms; elapsed += 100)
        {
            t += 100;
            mic.pump();
            supervisor.tick(t);
        }
    };

    supervisor.select(DeviceInfo{"A", "Headset Microphone"}, t);
    CHECK(supervisor.state() == DeviceState::starting);
    advance(500);
    CHECK(supervisor.state() == DeviceState::running);

    mic.unplug("A");
    advance(DeviceSupervisor::kStallMs + 200);
    CHECK(supervisor.state() == DeviceState::missing);
    CHECK_FALSE(mic.opened); // closed, and not switched to B
    CHECK(mic.openCalls == 1);

    // Keeps looking about every 3 seconds, still never opening B.
    const int listsBefore = mic.listCalls;
    advance(9000);
    CHECK(mic.listCalls - listsBefore == 3);
    CHECK_FALSE(mic.opened);

    mic.devices.push_back({"A", "Headset Microphone"});
    advance(DeviceSupervisor::kRetryMs + 500);
    REQUIRE(mic.opened);
    CHECK(mic.opened->id == "A");
    CHECK(supervisor.state() == DeviceState::running);
}

TEST_CASE("Output that will not open retries every 3 seconds", "[integration][devices]")
{
    FakeEndpoint output;
    output.devices = {{"C", "CABLE Input (VB-Audio Virtual Cable)"}};
    output.failOpen = true;
    DeviceSupervisor supervisor(output);

    supervisor.select(output.devices[0], 0);
    CHECK(supervisor.state() == DeviceState::failed);
    CHECK(supervisor.lastError() == "Device is busy");
    CHECK(output.openCalls == 1);

    supervisor.tick(2999);
    CHECK(output.openCalls == 1);
    supervisor.tick(3000);
    CHECK(output.openCalls == 2);

    output.failOpen = false;
    supervisor.tick(6000);
    CHECK(supervisor.state() == DeviceState::starting);
    output.pump();
    supervisor.tick(6100);
    CHECK(supervisor.state() == DeviceState::running);

    supervisor.select(std::nullopt, 7000);
    CHECK(supervisor.state() == DeviceState::notSelected);
    CHECK_FALSE(output.opened);
}

TEST_CASE("Device that stops calling back is reopened", "[integration][devices]")
{
    FakeEndpoint output;
    output.devices = {{"C", "CABLE Input"}};
    DeviceSupervisor supervisor(output);

    supervisor.select(output.devices[0], 0);
    output.pump();
    supervisor.tick(100);
    REQUIRE(supervisor.state() == DeviceState::running);

    // Still listed, but silent (driver hiccup): reopened after the stall time.
    supervisor.tick(100 + DeviceSupervisor::kStallMs);
    CHECK(output.openCalls == 2);
    CHECK(supervisor.state() == DeviceState::starting);
}

// ---------------------------------------------------------------- Engine with a fake mic

TEST_CASE("Mic is resampled to the output and survives rate changes and unplugging", "[integration][engine]")
{
    Engine engine;
    engine.outputStarting(48000, 480);
    OutputRecorder output{engine};
    Clocks clocks;

    Sine sine{1000, 44100};
    std::vector<float> mic(441);
    auto capture = [&] {
        sine.fill(mic.data(), int(mic.size()));
        engine.micCaptured(mic.data(), mic.data(), int(mic.size()));
    };

    engine.micStarting(44100, 441);
    clocks.run(6, 44100, 441, capture, 48000, 480, std::ref(output));
    CHECK(measureFrequency(output.last(1), 48000) == Catch::Approx(1000).margin(1.0));

    // Windows changed the mic's rate.
    sine = Sine{1000, 48000};
    mic.resize(480);
    engine.micStarting(48000, 480);
    clocks.run(6, 48000, 480, capture, 48000, 480, std::ref(output));
    CHECK(measureFrequency(output.last(1), 48000) == Catch::Approx(1000).margin(1.0));

    // Mic unplugged: the mix keeps running, silent.
    engine.micStopped();
    clocks.run(1, 48000, 480, nullptr, 48000, 480, std::ref(output));
    CHECK(peak(output.last(0.5)) == 0.0f);
}

// ---------------------------------------------------------------- Plugin and app over real shared memory

TEST_CASE("Plugin and app talk over real shared memory", "[integration][bridge]")
{
    const std::string name = uniqueMemoryName("bridge");
    SharedMemory::removeName(name);
    Clocks clocks;

    auto plugin = std::make_unique<TapPluginSide>(name, 1);
    plugin->setSampleRate(48000);
    plugin->tick(clocks.ms());
    CHECK(plugin->status() == TapPluginStatus::waitingForApp);

    std::string error;
    auto appMemory = SharedMemory::openOrCreate(name, kTapMappingBytes, error);
    REQUIRE(appMemory);
    REQUIRE(attachTap(appMemory->data(), appMemory->size()) == TapCheck::ok);
    auto& header = *static_cast<TapHeader*>(appMemory->data());

    Engine engine;
    engine.setTapMemory(appMemory->data(), appMemory->size());
    engine.outputStarting(48000, 480);
    OutputRecorder output{engine};

    Sine sine{1000, 48000};
    std::vector<float> block(256);
    uint64_t nextTickMs = 0;
    auto ableton = [&] {
        sine.fill(block.data(), int(block.size()));
        plugin->process(block.data(), block.data(), uint32_t(block.size()));
        if (clocks.ms() >= nextTickMs) // the plugin's UI timer
        {
            plugin->tick(clocks.ms());
            nextTickMs = clocks.ms() + 50;
        }
    };

    SECTION("music arrives, stops when Ableton closes, and returns")
    {
        clocks.run(6, 48000, 256, ableton, 48000, 480, std::ref(output));
        CHECK(engine.tapStatus() == TapReaderStatus::connected);
        CHECK(plugin->status() == TapPluginStatus::connected);
        CHECK(measureFrequency(output.last(1), 48000) == Catch::Approx(1000).margin(1.0));
        CHECK(engine.counters().musicUnderruns == 0);

        clocks.run(2, 48000, 256, nullptr, 48000, 480, std::ref(output)); // Ableton closed
        CHECK(engine.tapStatus() == TapReaderStatus::waitingForPlugin);
        CHECK(peak(output.last(0.5)) == 0.0f);

        clocks.run(6, 48000, 256, ableton, 48000, 480, std::ref(output));
        CHECK(engine.tapStatus() == TapReaderStatus::connected);
        CHECK(measureFrequency(output.last(1), 48000) == Catch::Approx(1000).margin(1.0));
    }

    SECTION("Ableton changes sample rate")
    {
        clocks.run(2, 48000, 256, ableton, 48000, 480, std::ref(output));
        plugin->setSampleRate(44100);
        sine = Sine{1000, 44100};
        clocks.run(6, 44100, 256, ableton, 48000, 480, std::ref(output));
        CHECK(header.sampleRate.load() == 44100);
        CHECK(engine.tapStatus() == TapReaderStatus::connected);
        CHECK(measureFrequency(output.last(1), 48000) == Catch::Approx(1000).margin(1.0));
    }

    SECTION("a second Tap passes audio through and waits its turn")
    {
        TapPluginSide second(name, 2);
        second.setSampleRate(48000);
        clocks.run(1, 48000, 256, ableton, 48000, 480, std::ref(output));
        second.tick(clocks.ms());
        CHECK(second.status() == TapPluginStatus::anotherTapActive);

        const uint64_t before = header.writeIndex.load();
        second.process(block.data(), block.data(), 256);
        CHECK(header.writeIndex.load() == before); // only the owner writes

        plugin.reset(); // first Tap removed from the project
        second.tick(clocks.ms() + 10);
        CHECK(header.ownerId.load() == 2);
        CHECK(second.status() != TapPluginStatus::anotherTapActive);
    }

    SECTION("the app going away never blocks the plugin")
    {
        appMemory.reset(); // app closed: nobody reads
        engine.setTapMemory(nullptr, 0);
        clocks.run(5, 48000, 256, ableton, 48000, 480, nullptr); // ring fills, copies are dropped
        CHECK(plugin->status() == TapPluginStatus::waitingForApp);
    }

    plugin.reset();
    SharedMemory::removeName(name);
}

TEST_CASE("Version mismatch is reported and the data ignored", "[integration][bridge]")
{
    const std::string name = uniqueMemoryName("ver");
    SharedMemory::removeName(name);

    std::string error;
    auto appMemory = SharedMemory::openOrCreate(name, kTapMappingBytes, error);
    REQUIRE(appMemory);
    REQUIRE(attachTap(appMemory->data(), appMemory->size()) == TapCheck::ok);
    static_cast<TapHeader*>(appMemory->data())->version = kTapVersion + 1; // written by a newer Midi4u

    TapPluginSide plugin(name, 1);
    plugin.setSampleRate(48000);
    plugin.tick(0);
    CHECK(plugin.status() == TapPluginStatus::versionMismatch);

    Engine engine;
    engine.setTapMemory(appMemory->data(), appMemory->size());
    engine.outputStarting(48000, 480);
    std::vector<float> l(480), r(480);
    engine.renderOutput(l.data(), r.data(), 480);
    CHECK(engine.tapStatus() == TapReaderStatus::versionMismatch);
    CHECK(peak(l) == 0.0f);

    SharedMemory::removeName(name);
}
