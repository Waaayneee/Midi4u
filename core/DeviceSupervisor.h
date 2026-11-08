#pragma once

#include "AudioEndpoint.h"

#include <optional>
#include <string>

namespace midi4u
{

enum class DeviceState
{
    notSelected,
    starting, // opened, waiting for the first callbacks
    running,
    missing,  // the chosen device is not connected
    failed    // the device exists but would not open
};

// Keeps one endpoint (mic or output) on the device the user chose. Reopens it when it
// stops, retries every kRetryMs while it is missing, and never switches to another device.
// Runs on the message thread only.
class DeviceSupervisor
{
public:
    static constexpr uint64_t kRetryMs = 3000;
    static constexpr uint64_t kStallMs = 1500; // no callbacks for this long means the device is gone

    explicit DeviceSupervisor(AudioEndpoint& endpoint) : endpoint_(endpoint) {}
    ~DeviceSupervisor();

    DeviceSupervisor(const DeviceSupervisor&) = delete;
    DeviceSupervisor& operator=(const DeviceSupervisor&) = delete;

    // Chooses a device (or none) and tries to open it straight away.
    void select(const std::optional<DeviceInfo>& device, uint64_t nowMs);

    // Changes the buffer size, reopening the device if it is open.
    void setBufferMs(double bufferMs, uint64_t nowMs);

    // Call a few times per second.
    void tick(uint64_t nowMs);

    DeviceState state() const { return state_; }
    const std::string& lastError() const { return lastError_; }
    const std::optional<DeviceInfo>& selected() const { return wanted_; }

private:
    void closeIfOpen();

    AudioEndpoint& endpoint_;
    std::optional<DeviceInfo> wanted_;
    double bufferMs_ = 10.0;
    DeviceState state_ = DeviceState::notSelected;
    std::string lastError_;
    bool open_ = false;
    uint64_t lastCount_ = 0;
    uint64_t lastProgressMs_ = 0;
    uint64_t nextTryMs_ = 0;
};

} // namespace midi4u
