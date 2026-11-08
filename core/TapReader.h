#pragma once

#include "Realtime.h"
#include "TapProtocol.h"

#include <atomic>
#include <cstddef>

namespace midi4u
{

enum class TapReaderStatus
{
    noBridge,         // shared memory not open yet
    waitingForPlugin, // no heartbeat: Ableton closed or the plugin was removed
    connected,
    versionMismatch   // "Please update Midi4u"; the data is ignored
};

// App side of the bridge. Validates the header, watches the heartbeat and exposes the
// music ring. poll() runs on the output device's audio thread.
class TapReader
{
public:
    static constexpr double kHeartbeatTimeoutSeconds = 1.0;

    // Message thread, once. The memory must stay mapped for as long as the reader is used.
    void setMemory(void* memory, size_t bytes);

    // Audio thread, once per output block. Returns true while music should be read.
    bool poll(uint32_t elapsedFrames, double outputRate) MIDI4U_NONBLOCKING;

    // True once after (re)connecting or after Ableton changed sample rate. The ring has
    // already been emptied; the caller should reset its resampler.
    bool takeReset() MIDI4U_NONBLOCKING { const bool r = resetPending_; resetPending_ = false; return r; }

    // Forget the connection, for example when the output device restarts. Not concurrent with poll().
    void forceReconnect() { connected_ = false; }

    SpscRing& ring() { return ring_; }
    uint32_t sampleRate() const { return rate_; }
    TapReaderStatus status() const { return status_.load(std::memory_order_relaxed); }

private:
    bool fail(TapReaderStatus status) MIDI4U_NONBLOCKING;

    std::atomic<void*> memory_{nullptr};
    size_t bytes_ = 0;
    std::atomic<TapReaderStatus> status_{TapReaderStatus::noBridge};

    // Audio thread only.
    SpscRing ring_;
    bool ringReady_ = false;
    bool heartbeatSeen_ = false;
    uint64_t lastHeartbeat_ = 0;
    double stillFrames_ = 0;
    bool connected_ = false;
    uint32_t rate_ = 0;
    bool resetPending_ = false;
};

} // namespace midi4u
