#include "Sync.h"
#include "TestUtils.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace midi4u;
using namespace testutils;

namespace
{
struct SimConfig
{
    double seconds = 10;
    double sourceRate = 48000; // what the source device believes its rate is
    double drift = 0;          // how far its real clock is off, as a fraction
    int sourceBlock = 256;
    double outputRate = 48000;
    int outputBlock = 480;
    double toneHz = 0; // 0 sends silence (fast)
    double recordLastSeconds = 0;
    double targetMs = 5;
};

struct SimResult
{
    uint64_t underruns = 0, overflows = 0;
    double maxCorrection = 0;
    uint32_t maxFill = 0;
    uint32_t finalFill = 0;
    std::vector<float> recorded;
};

// Runs a source and the output device on their own clocks, the way real devices do.
SimResult simulate(const SimConfig& c)
{
    HeapRing heap(65536);
    auto& ring = heap.ring();
    Sync sync(Sync::Quality::sinc);
    sync.prepare(c.outputBlock);
    sync.reset(c.sourceRate, c.outputRate);
    sync.setTargetMs(c.targetMs);

    Sine sine{c.toneHz, c.sourceRate};
    std::vector<float> in(size_t(c.sourceBlock), 0.0f), outL(size_t(c.outputBlock)), outR(size_t(c.outputBlock));

    const double realSourceRate = c.sourceRate * (1.0 + c.drift);
    const double recordFrom = c.seconds - c.recordLastSeconds;
    SimResult result;
    uint64_t produced = 0, consumed = 0;

    for (;;)
    {
        const double tSource = double(produced) * c.sourceBlock / realSourceRate;
        const double tOutput = double(consumed) * c.outputBlock / c.outputRate;

        if (tOutput >= c.seconds)
            break;

        if (tSource <= tOutput)
        {
            if (c.toneHz > 0)
                sine.fill(in.data(), c.sourceBlock);
            ring.write(in.data(), in.data(), uint32_t(c.sourceBlock));
            ++produced;
        }
        else
        {
            sync.process(ring, outL.data(), outR.data(), c.outputBlock);
            result.maxCorrection = std::max(result.maxCorrection, std::abs(sync.correction()));
            result.maxFill = std::max(result.maxFill, ring.available());
            if (tOutput >= recordFrom)
                result.recorded.insert(result.recorded.end(), outL.begin(), outL.end());
            ++consumed;
        }
    }

    result.underruns = sync.underruns();
    result.overflows = sync.overflows();
    result.finalFill = ring.available();
    return result;
}
} // namespace

TEST_CASE("One hour of clock drift stays within buffer limits", "[unit][sync][slow]")
{
    SimConfig config;
    config.seconds = 3600;

    SECTION("source clock 0.1 percent fast") { config.drift = 0.001; }
    SECTION("source clock 0.1 percent slow") { config.drift = -0.001; }
    SECTION("44.1 kHz Ableton with 512 blocks into 48 kHz output")
    {
        config.sourceRate = 44100;
        config.sourceBlock = 512;
        config.drift = 0.0002;
    }

    const auto result = simulate(config);

    CHECK(result.underruns == 0);
    CHECK(result.overflows == 0);
    CHECK(result.maxCorrection <= Sync::kMaxCorrection);
    // Never close to the ring's 65536 frames; stays within target + 50 ms.
    CHECK(result.maxFill < uint32_t((config.targetMs + 50.0) * config.sourceRate / 1000.0));
}

TEST_CASE("A test tone keeps its pitch through Sync", "[unit][sync]")
{
    SimConfig config;
    config.seconds = 30;
    config.toneHz = 1000;
    config.recordLastSeconds = 10;

    SECTION("same rate") {}
    SECTION("44.1 kHz into 48 kHz")
    {
        config.sourceRate = 44100;
        config.sourceBlock = 441;
    }
    SECTION("48 kHz into 44.1 kHz")
    {
        config.outputRate = 44100;
        config.outputBlock = 441;
    }

    const auto result = simulate(config);
    REQUIRE(result.underruns == 0);

    // Whole window, and every one second slice, so wobble would show up too.
    CHECK(measureFrequency(result.recorded, config.outputRate) == Catch::Approx(1000.0).margin(0.5));

    const size_t slice = size_t(config.outputRate);
    for (size_t start = 0; start + slice <= result.recorded.size(); start += slice)
    {
        std::vector<float> part(result.recorded.begin() + long(start), result.recorded.begin() + long(start + slice));
        CHECK(measureFrequency(part, config.outputRate) == Catch::Approx(1000.0).margin(1.5));
    }
}

TEST_CASE("Sync outputs silence when dry and fades back in", "[unit][sync]")
{
    HeapRing heap(8192);
    auto& ring = heap.ring();
    Sync sync;
    sync.prepare(480);
    sync.reset(48000, 48000);
    sync.setTargetMs(5);

    std::vector<float> ones(480, 1.0f), l(480), r(480);

    // Not enough to start yet: silence while priming.
    REQUIRE(ring.write(ones.data(), ones.data(), 200));
    sync.process(ring, l.data(), r.data(), 480);
    CHECK(peak(l) == 0.0f);
    CHECK(sync.isPriming());

    // Enough now (target 240 + one block of 480): audio starts with a fade, not a jump.
    REQUIRE(ring.write(ones.data(), ones.data(), 480));
    REQUIRE(ring.write(ones.data(), ones.data(), 480));
    sync.process(ring, l.data(), r.data(), 480);
    CHECK_FALSE(sync.isPriming());
    CHECK(std::abs(l[0]) < 0.05f);
    CHECK(peak(l) > 0.5f);

    // Source stops: runs dry, one underrun, silence.
    for (int i = 0; i < 4; ++i)
        sync.process(ring, l.data(), r.data(), 480);
    CHECK(sync.underruns() == 1);
    CHECK(peak(l) == 0.0f);
    CHECK(sync.isPriming());
}

TEST_CASE("Sync drops the oldest audio when the ring overfills", "[unit][sync]")
{
    HeapRing heap(65536);
    auto& ring = heap.ring();
    Sync sync;
    sync.prepare(480);
    sync.reset(48000, 48000);
    sync.setTargetMs(5);

    std::vector<float> block(4800, 0.1f), l(480), r(480);
    for (int i = 0; i < 10; ++i) // one second piled up
        REQUIRE(ring.write(block.data(), block.data(), 4800));

    sync.process(ring, l.data(), r.data(), 480);

    CHECK(sync.overflows() == 1);
    CHECK(ring.available() < 48000 / 10);
}

TEST_CASE("Sync with no source outputs silence", "[unit][sync]")
{
    HeapRing heap(1024);
    Sync sync;
    sync.prepare(256);
    sync.reset(0, 48000);

    std::vector<float> ones(256, 1.0f), l(256, 9.0f), r(256, 9.0f);
    heap.ring().write(ones.data(), ones.data(), 256);
    sync.process(heap.ring(), l.data(), r.data(), 256);
    CHECK(peak(l) == 0.0f);
    CHECK(peak(r) == 0.0f);
}
