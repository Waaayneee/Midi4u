#include "DeviceSupervisor.h"

#include "DeviceRules.h"

namespace midi4u
{

DeviceSupervisor::~DeviceSupervisor()
{
    closeIfOpen();
}

void DeviceSupervisor::closeIfOpen()
{
    if (open_)
    {
        endpoint_.close();
        open_ = false;
    }
}

void DeviceSupervisor::select(const std::optional<DeviceInfo>& device, uint64_t nowMs)
{
    closeIfOpen();
    wanted_ = device;
    lastError_.clear();
    nextTryMs_ = nowMs;
    tick(nowMs);
}

void DeviceSupervisor::setBufferMs(double bufferMs, uint64_t nowMs)
{
    if (bufferMs == bufferMs_)
        return;

    bufferMs_ = bufferMs;

    if (open_)
    {
        closeIfOpen();
        nextTryMs_ = nowMs;
        tick(nowMs);
    }
}

void DeviceSupervisor::tick(uint64_t nowMs)
{
    if (! wanted_)
    {
        state_ = DeviceState::notSelected;
        return;
    }

    if (open_)
    {
        const uint64_t count = endpoint_.callbackCount();

        if (count != lastCount_)
        {
            lastCount_ = count;
            lastProgressMs_ = nowMs;
            state_ = DeviceState::running;
            return;
        }

        if (nowMs - lastProgressMs_ < kStallMs)
            return;

        // Callbacks stopped: the device was unplugged or its driver gave up.
        closeIfOpen();
        lastError_ = "Device stopped responding";
        nextTryMs_ = nowMs;
    }

    if (nowMs < nextTryMs_)
        return;

    nextTryMs_ = nowMs + kRetryMs;

    const auto found = findSavedDevice(*wanted_, endpoint_.listDevices());

    if (! found)
    {
        state_ = DeviceState::missing;
        lastError_ = "Device not connected";
        return;
    }

    const std::string error = endpoint_.open(*found, bufferMs_);

    if (! error.empty())
    {
        state_ = DeviceState::failed;
        lastError_ = error;
        return;
    }

    open_ = true;
    lastCount_ = endpoint_.callbackCount();
    lastProgressMs_ = nowMs;
    state_ = DeviceState::starting;
    lastError_.clear();
}

} // namespace midi4u
