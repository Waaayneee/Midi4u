#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace midi4u
{

// A device as the system names it. id is the system's stable ID (empty if unknown),
// name is what the user sees.
struct DeviceInfo
{
    std::string id;
    std::string name;

    bool operator==(const DeviceInfo&) const = default;
};

// Receives the output device's audio callbacks. The output device is the master clock.
class OutputCallback
{
public:
    virtual ~OutputCallback() = default;
    virtual void outputStarting(double sampleRate, int maxFrames) = 0; // not on the audio thread; may allocate
    virtual void renderOutput(float* left, float* right, int numFrames) = 0;
    virtual void outputStopped() = 0;
};

// Receives the mic device's audio callbacks. Mono mics arrive with left == right.
class MicCallback
{
public:
    virtual ~MicCallback() = default;
    virtual void micStarting(double sampleRate, int maxFrames) = 0;
    virtual void micCaptured(const float* left, const float* right, int numFrames) = 0;
    virtual void micStopped() = 0;
};

// One audio device slot (the mic, or the output). The real versions wrap JUCE devices;
// tests use fakes to simulate unplugging and sample rate changes.
class AudioEndpoint
{
public:
    virtual ~AudioEndpoint() = default;

    // Rescans and lists the devices that exist right now.
    virtual std::vector<DeviceInfo> listDevices() = 0;

    // The system default device, if any.
    virtual std::optional<DeviceInfo> defaultDevice() = 0;

    // Opens and starts the device. Returns an empty string on success, otherwise the error.
    virtual std::string open(const DeviceInfo& device, double bufferMs) = 0;

    virtual void close() = 0;

    // Grows by one per audio callback. Used to notice a device that stopped silently.
    virtual uint64_t callbackCount() const = 0;
};

} // namespace midi4u
