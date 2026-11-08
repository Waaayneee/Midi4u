#pragma once

#include "AudioEndpoint.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <memory>

// A mic or output device opened through JUCE: WASAPI shared mode on Windows (never ASIO,
// so the Scarlett stays Ableton's), CoreAudio on macOS.
//
// Used as the output, this is the v1 Output Sink (WindowsDeviceSink on Windows,
// MacDeviceSink on macOS). v2 swaps in a sink for our own driver behind the same
// AudioEndpoint interface.
//
// Devices are opened directly, not through AudioDeviceManager, because that class falls
// back to the default device when the chosen one disappears. Midi4u must not.
class JuceDevice final : public midi4u::AudioEndpoint, private juce::AudioIODeviceCallback
{
public:
    explicit JuceDevice(midi4u::OutputCallback& output);
    explicit JuceDevice(midi4u::MicCallback& mic);
    ~JuceDevice() override;

    std::vector<midi4u::DeviceInfo> listDevices() override;
    std::optional<midi4u::DeviceInfo> defaultDevice() override;
    std::string open(const midi4u::DeviceInfo& device, double bufferMs) override;
    void close() override;
    uint64_t callbackCount() const override { return callbacks_.load(std::memory_order_relaxed); }

private:
    bool isInput() const { return mic_ != nullptr; }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceIOCallbackWithContext(const float* const* inputs, int numInputs, float* const* outputs, int numOutputs,
                                          int numFrames, const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceStopped() override;

    midi4u::OutputCallback* output_ = nullptr;
    midi4u::MicCallback* mic_ = nullptr;
    std::unique_ptr<juce::AudioIODeviceType> type_;
    std::unique_ptr<juce::AudioIODevice> device_;
    std::vector<float> scratch_;
    std::atomic<uint64_t> callbacks_{0};
};
