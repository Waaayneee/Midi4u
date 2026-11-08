#include "TapReader.h"

namespace midi4u
{

void TapReader::setMemory(void* memory, size_t bytes)
{
    bytes_ = bytes;
    memory_.store(memory, std::memory_order_release); // publishes bytes_
}

bool TapReader::fail(TapReaderStatus status)
{
    connected_ = false;
    status_.store(status, std::memory_order_relaxed);
    return false;
}

bool TapReader::poll(uint32_t elapsedFrames, double outputRate)
{
    void* memory = memory_.load(std::memory_order_acquire);

    if (memory == nullptr)
        return fail(TapReaderStatus::noBridge);

    switch (checkTap(memory, bytes_))
    {
        case TapCheck::ok:
            break;
        case TapCheck::versionMismatch:
            return fail(TapReaderStatus::versionMismatch);
        case TapCheck::notReady:
            return fail(TapReaderStatus::waitingForPlugin);
        case TapCheck::badMagic:
        case TapCheck::badSize:
            return fail(TapReaderStatus::noBridge);
    }

    if (! ringReady_)
    {
        ring_ = tapRing(memory);
        ringReady_ = true;
    }

    auto* header = static_cast<TapHeader*>(memory);
    const uint64_t heartbeat = header->heartbeat.load(std::memory_order_acquire);
    const double timeoutFrames = kHeartbeatTimeoutSeconds * outputRate;

    if (! heartbeatSeen_)
    {
        // A heartbeat left over from an old session proves nothing: wait for it to move.
        heartbeatSeen_ = true;
        lastHeartbeat_ = heartbeat;
        stillFrames_ = timeoutFrames;
    }
    else if (heartbeat != lastHeartbeat_)
    {
        lastHeartbeat_ = heartbeat;
        stillFrames_ = 0;
    }
    else if (stillFrames_ < timeoutFrames)
    {
        stillFrames_ += elapsedFrames;
    }

    const uint32_t rate = header->sampleRate.load(std::memory_order_acquire);

    if (stillFrames_ >= timeoutFrames || rate == 0)
        return fail(TapReaderStatus::waitingForPlugin);

    if (! connected_ || rate != rate_ || ring_.isCorrupt())
    {
        // Fresh start: skip whatever piled up while nobody was reading.
        ring_.discardAll();
        rate_ = rate;
        connected_ = true;
        resetPending_ = true;
    }

    status_.store(TapReaderStatus::connected, std::memory_order_relaxed);
    return true;
}

} // namespace midi4u
