#pragma once

#include "DeviceSupervisor.h"
#include "Engine.h"
#include "JuceDevice.h"
#include "SharedMemory.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <memory>
#include <optional>

// Saved in settings.json in the app data folder and restored on launch.
struct Settings
{
    std::optional<midi4u::DeviceInfo> mic, output; // system ID plus display name
    float musicDb = 0.0f, micDb = 0.0f;
    bool musicMuted = false, micMuted = false;
    midi4u::Latency latency = midi4u::Latency::normal;
    bool startWithSystem = false;
    bool discordChecklist[4] = {false, false, false, false};

    static juce::File file();
    static Settings load();
    void save() const;
};

// Owns the engine, both devices and the bridge, and keeps them running. Message thread only.
class AppController final : private juce::Timer
{
public:
    AppController();
    ~AppController() override;

    midi4u::Engine& engine() { return engine_; }
    Settings& settings() { return settings_; }
    void saveSettings() { settings_.save(); }

    // Devices
    std::vector<midi4u::DeviceInfo> micChoices();    // feedback devices already hidden
    std::vector<midi4u::DeviceInfo> outputChoices();
    void selectMic(const std::optional<midi4u::DeviceInfo>& mic);
    void selectOutput(const std::optional<midi4u::DeviceInfo>& output);
    const midi4u::DeviceSupervisor& micSupervisor() const { return micSupervisor_; }
    const midi4u::DeviceSupervisor& outputSupervisor() const { return outputSupervisor_; }

    // Plugin state from the heartbeat, checked here so it does not depend on the output running.
    midi4u::TapReaderStatus pluginStatus() const { return pluginStatus_; }

    // Mix
    void setMusicDb(float db);
    void setMicDb(float db);
    void setMusicMuted(bool muted);
    void setMicMuted(bool muted);
    void setLatency(midi4u::Latency latency);
    void setStartWithSystem(bool enabled);
    void playTestTone() { engine_.mixer().playTestTone(); }

    // Setup helpers
    std::optional<midi4u::DeviceInfo> cableDevice();          // rescans
    bool outputIsCable() const;
    void useDefaultMic();       // the system default recording device, if it is usable
    void useCableOutput();      // "CABLE Input" when installed

private:
    void timerCallback() override;
    void openBridge();
    void updatePluginStatus(uint64_t now);
    void logChanges();
    void log(const juce::String& message);

    static uint64_t nowMs();

    // Declared first so it outlives the engine and devices that read it.
    std::unique_ptr<midi4u::SharedMemory> tapMemory_;
    std::unique_ptr<juce::FileLogger> logger_;

    Settings settings_;
    midi4u::Engine engine_;
    JuceDevice output_{static_cast<midi4u::OutputCallback&>(engine_)};
    JuceDevice mic_{static_cast<midi4u::MicCallback&>(engine_)};
    midi4u::DeviceSupervisor outputSupervisor_{output_};
    midi4u::DeviceSupervisor micSupervisor_{mic_};

    uint64_t nextBridgeTryMs_ = 0;
    juce::String bridgeError_;
    midi4u::DeviceState lastOutputState_ = midi4u::DeviceState::notSelected;
    midi4u::DeviceState lastMicState_ = midi4u::DeviceState::notSelected;
    midi4u::TapReaderStatus pluginStatus_ = midi4u::TapReaderStatus::noBridge;
    uint64_t lastHeartbeat_ = 0, lastHeartbeatChangeMs_ = 0;
    bool heartbeatSeen_ = false;
    midi4u::TapReaderStatus lastTapStatus_ = midi4u::TapReaderStatus::noBridge;
    midi4u::Engine::Counters lastCounters_{};
};
