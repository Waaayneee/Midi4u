#include "AppController.h"

#include "DeviceRules.h"
#include "Platform.h"
#include "TapProtocol.h"

using midi4u::DeviceInfo;
using midi4u::DeviceState;
using midi4u::Latency;

namespace
{
constexpr int kLogFilesToKeep = 5;
constexpr uint64_t kBridgeRetryMs = 3000;

juce::var deviceToVar(const std::optional<DeviceInfo>& device)
{
    if (! device)
        return {};

    auto* object = new juce::DynamicObject();
    object->setProperty("id", juce::String(device->id));
    object->setProperty("name", juce::String(device->name));
    return juce::var(object);
}

std::optional<DeviceInfo> deviceFromVar(const juce::var& value)
{
    if (! value.isObject())
        return std::nullopt;

    DeviceInfo device{value["id"].toString().toStdString(), value["name"].toString().toStdString()};

    if (device.id.empty() && device.name.empty())
        return std::nullopt;

    return device;
}

juce::String latencyName(Latency latency)
{
    switch (latency)
    {
        case Latency::low: return "low";
        case Latency::safe: return "safe";
        case Latency::normal: break;
    }
    return "normal";
}

Latency latencyFromName(const juce::String& name)
{
    if (name == "low")
        return Latency::low;
    if (name == "safe")
        return Latency::safe;
    return Latency::normal;
}

juce::String stateName(DeviceState state)
{
    switch (state)
    {
        case DeviceState::notSelected: return "not selected";
        case DeviceState::starting: return "starting";
        case DeviceState::running: return "running";
        case DeviceState::missing: return "missing";
        case DeviceState::failed: return "failed";
    }
    return {};
}

juce::String tapStatusName(midi4u::TapReaderStatus status)
{
    switch (status)
    {
        case midi4u::TapReaderStatus::noBridge: return "bridge not open";
        case midi4u::TapReaderStatus::waitingForPlugin: return "no heartbeat from Midi4u Tap";
        case midi4u::TapReaderStatus::connected: return "connected";
        case midi4u::TapReaderStatus::versionMismatch: return "version mismatch, please update Midi4u";
    }
    return {};
}

float clampDb(const juce::var& value)
{
    return juce::jlimit(-60.0f, 12.0f, static_cast<float>(static_cast<double>(value)));
}

std::string nameOf(const std::optional<DeviceInfo>& device)
{
    return device ? device->name : std::string();
}

std::unique_ptr<juce::FileLogger> createLogger()
{
    const auto folder = platform::appDataFolder().getChildFile("logs");
    folder.createDirectory();

    // Rotate: keep the newest few logs, one per launch.
    auto old = folder.findChildFiles(juce::File::findFiles, false, "midi4u-*.log");
    std::sort(old.begin(), old.end(), [](const juce::File& a, const juce::File& b) {
        return a.getLastModificationTime() > b.getLastModificationTime();
    });
    for (int i = kLogFilesToKeep - 1; i < old.size(); ++i)
        old.getReference(i).deleteFile();

    const auto name = "midi4u-" + juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S") + ".log";
    return std::make_unique<juce::FileLogger>(folder.getChildFile(name), "Midi4u " + juce::String(JUCE_APPLICATION_VERSION_STRING), 0);
}
} // namespace

//==============================================================================
juce::File Settings::file()
{
    return platform::appDataFolder().getChildFile("settings.json");
}

Settings Settings::load()
{
    Settings settings;
    const auto json = juce::JSON::parse(file());

    if (! json.isObject())
        return settings;

    settings.mic = deviceFromVar(json["mic"]);
    settings.output = deviceFromVar(json["output"]);
    settings.musicDb = clampDb(json.getProperty("musicDb", 0.0));
    settings.micDb = clampDb(json.getProperty("micDb", 0.0));
    settings.musicMuted = json.getProperty("musicMuted", false);
    settings.micMuted = json.getProperty("micMuted", false);
    settings.latency = latencyFromName(json.getProperty("latency", "normal").toString());
    settings.startWithSystem = json.getProperty("startWithWindows", false);

    return settings;
}

void Settings::save() const
{
    auto* object = new juce::DynamicObject();
    object->setProperty("mic", deviceToVar(mic));
    object->setProperty("output", deviceToVar(output));
    object->setProperty("musicDb", musicDb);
    object->setProperty("micDb", micDb);
    object->setProperty("musicMuted", musicMuted);
    object->setProperty("micMuted", micMuted);
    object->setProperty("latency", latencyName(latency));
    object->setProperty("startWithWindows", startWithSystem);

    file().getParentDirectory().createDirectory();
    file().replaceWithText(juce::JSON::toString(juce::var(object)));
}

//==============================================================================
AppController::AppController() : logger_(createLogger()), settings_(Settings::load())
{
    const auto now = nowMs();

    engine_.mixer().setMusicGainDb(settings_.musicDb);
    engine_.mixer().setMicGainDb(settings_.micDb);
    engine_.mixer().setMusicMuted(settings_.musicMuted);
    engine_.mixer().setMicMuted(settings_.micMuted);
    engine_.setLatency(settings_.latency);
    outputSupervisor_.setBufferMs(midi4u::latencySettings(settings_.latency).outputBufferMs, now);

    if (settings_.startWithSystem)
        platform::setStartWithSystem(true); // keeps the path current if the app moved

    openBridge();

    if (settings_.output)
    {
        log("Output: " + juce::String(settings_.output->name));
        outputSupervisor_.select(settings_.output, now);
    }

    if (settings_.mic && ! midi4u::wouldFeedBack(settings_.mic->name, nameOf(settings_.output)))
    {
        log("Mic: " + juce::String(settings_.mic->name));
        micSupervisor_.select(settings_.mic, now);
    }

    startTimerHz(4);
}

AppController::~AppController()
{
    stopTimer();
    log("Midi4u closed");
}

uint64_t AppController::nowMs()
{
    return static_cast<uint64_t>(juce::Time::getMillisecondCounterHiRes());
}

void AppController::log(const juce::String& message)
{
    if (logger_ != nullptr)
        logger_->logMessage(message);
}

std::vector<DeviceInfo> AppController::micChoices()
{
    return midi4u::usableMics(mic_.listDevices(), nameOf(settings_.output));
}

std::vector<DeviceInfo> AppController::outputChoices()
{
    return output_.listDevices();
}

void AppController::selectMic(const std::optional<DeviceInfo>& mic)
{
    if (mic && midi4u::wouldFeedBack(mic->name, nameOf(settings_.output)))
        return; // blocked: it would feed the mix back into itself

    settings_.mic = mic;
    settings_.save();
    log("Mic selected: " + juce::String(mic ? mic->name : "none"));
    micSupervisor_.select(mic, nowMs());
}

void AppController::selectOutput(const std::optional<DeviceInfo>& output)
{
    settings_.output = output;
    settings_.save();
    log("Output selected: " + juce::String(output ? output->name : "none"));
    outputSupervisor_.select(output, nowMs());

    if (settings_.mic && midi4u::wouldFeedBack(settings_.mic->name, nameOf(output)))
    {
        log("Mic cleared: it would feed the new output back into itself");
        selectMic(std::nullopt);
    }
}

void AppController::setMusicDb(float db)
{
    settings_.musicDb = db;
    engine_.mixer().setMusicGainDb(db);
}

void AppController::setMicDb(float db)
{
    settings_.micDb = db;
    engine_.mixer().setMicGainDb(db);
}

void AppController::setMusicMuted(bool muted)
{
    settings_.musicMuted = muted;
    engine_.mixer().setMusicMuted(muted);
    settings_.save();
}

void AppController::setMicMuted(bool muted)
{
    settings_.micMuted = muted;
    engine_.mixer().setMicMuted(muted);
    settings_.save();
}

void AppController::setLatency(Latency latency)
{
    settings_.latency = latency;
    settings_.save();
    engine_.setLatency(latency);
    outputSupervisor_.setBufferMs(midi4u::latencySettings(latency).outputBufferMs, nowMs());
    log("Latency: " + latencyName(latency));
}

void AppController::setStartWithSystem(bool enabled)
{
    settings_.startWithSystem = enabled;
    settings_.save();
    platform::setStartWithSystem(enabled);
}

std::optional<DeviceInfo> AppController::cableDevice()
{
    return midi4u::findByPrefix(output_.listDevices(), platform::cableOutputPrefix().toStdString());
}

bool AppController::outputIsCable() const
{
    return settings_.output
        && juce::String(settings_.output->name).startsWithIgnoreCase(platform::cableOutputPrefix());
}

void AppController::useDefaultMic()
{
    if (const auto mic = mic_.defaultDevice(); mic && ! midi4u::wouldFeedBack(mic->name, nameOf(settings_.output)))
        selectMic(mic);
}

void AppController::useCableOutput()
{
    if (const auto cable = cableDevice())
        selectOutput(cable);
}

void AppController::timerCallback()
{
    openBridge();
    const auto now = nowMs();
    updatePluginStatus(now);
    outputSupervisor_.tick(now);
    micSupervisor_.tick(now);
    logChanges();
}

void AppController::openBridge()
{
    if (tapMemory_ != nullptr || nowMs() < nextBridgeTryMs_)
        return;

    nextBridgeTryMs_ = nowMs() + kBridgeRetryMs;
    std::string error;
    auto memory = midi4u::SharedMemory::openOrCreate(midi4u::kTapName, midi4u::kTapMappingBytes, error);

    if (memory == nullptr)
    {
        if (bridgeError_ != juce::String(error))
            log("Bridge could not open: " + juce::String(error));
        bridgeError_ = error;
        return;
    }

    midi4u::attachTap(memory->data(), memory->size()); // a version mismatch is reported by the engine
    engine_.setTapMemory(memory->data(), memory->size());
    tapMemory_ = std::move(memory);
    log("Bridge open");
}

void AppController::updatePluginStatus(uint64_t now)
{
    using midi4u::TapReaderStatus;

    if (tapMemory_ == nullptr)
    {
        pluginStatus_ = TapReaderStatus::noBridge;
        return;
    }

    switch (midi4u::checkTap(tapMemory_->data(), tapMemory_->size()))
    {
        case midi4u::TapCheck::ok: break;
        case midi4u::TapCheck::versionMismatch: pluginStatus_ = TapReaderStatus::versionMismatch; return;
        case midi4u::TapCheck::notReady:
        case midi4u::TapCheck::badMagic:
        case midi4u::TapCheck::badSize: pluginStatus_ = TapReaderStatus::noBridge; return;
    }

    const auto heartbeat = static_cast<midi4u::TapHeader*>(tapMemory_->data())->heartbeat.load(std::memory_order_acquire);

    if (! heartbeatSeen_ || heartbeat != lastHeartbeat_)
    {
        // The first reading may be left over from an old session, so it does not count as alive.
        lastHeartbeatChangeMs_ = heartbeatSeen_ ? now : now - 1000;
        heartbeatSeen_ = true;
        lastHeartbeat_ = heartbeat;
    }

    pluginStatus_ = now - lastHeartbeatChangeMs_ < 1000 ? TapReaderStatus::connected : TapReaderStatus::waitingForPlugin;
}

void AppController::logChanges()
{
    if (outputSupervisor_.state() != lastOutputState_)
    {
        lastOutputState_ = outputSupervisor_.state();
        log("Output " + juce::String(nameOf(settings_.output)) + ": " + stateName(lastOutputState_) + " "
            + juce::String(outputSupervisor_.lastError()));
    }

    if (micSupervisor_.state() != lastMicState_)
    {
        lastMicState_ = micSupervisor_.state();
        log("Mic " + juce::String(nameOf(settings_.mic)) + ": " + stateName(lastMicState_) + " "
            + juce::String(micSupervisor_.lastError()));
    }

    if (pluginStatus_ != lastTapStatus_)
    {
        lastTapStatus_ = pluginStatus_;
        log("Plugin: " + tapStatusName(lastTapStatus_));
    }

    // Audio threads only bump counters; the log is written here.
    const auto counters = engine_.counters();

    if (counters.musicUnderruns > lastCounters_.musicUnderruns && lastTapStatus_ == midi4u::TapReaderStatus::connected)
        log("Music buffer underrun (total " + juce::String(counters.musicUnderruns) + ")");
    if (counters.musicOverflows > lastCounters_.musicOverflows)
        log("Music buffer overflow (total " + juce::String(counters.musicOverflows) + ")");
    if (counters.micUnderruns > lastCounters_.micUnderruns && lastMicState_ == DeviceState::running)
        log("Mic buffer underrun (total " + juce::String(counters.micUnderruns) + ")");
    if (counters.micOverflows > lastCounters_.micOverflows)
        log("Mic buffer overflow (total " + juce::String(counters.micOverflows) + ")");

    lastCounters_ = counters;
}
