#pragma once

#include "SharedMemory.h"
#include "TapProtocol.h"

#include <atomic>
#include <memory>
#include <string>

namespace midi4u
{

enum class TapPluginStatus
{
    noBridge,         // shared memory could not be opened
    versionMismatch,  // "Please update Midi4u"
    anotherTapActive, // a different Tap instance owns the bridge
    waitingForApp,    // we own the bridge but the app is not reading
    connected         // we own the bridge and the app is reading
};

// Everything Midi4u Tap does with the bridge. The plugin only forwards calls here,
// so tests run exactly the code the plugin runs.
class TapPluginSide
{
public:
    explicit TapPluginSide(std::string memoryName = kTapName, uint64_t instanceId = makeInstanceId());
    ~TapPluginSide(); // gives up ownership

    TapPluginSide(const TapPluginSide&) = delete;
    TapPluginSide& operator=(const TapPluginSide&) = delete;

    // Message thread, a few times per second: opens the bridge, handles ownership, updates status.
    void tick(uint64_t nowMs);

    TapPluginStatus status() const { return status_.load(std::memory_order_relaxed); }

    // Any thread, before or between audio blocks.
    void setSampleRate(double sampleRate);

    // Audio thread. Copies the block into the bridge if this instance owns it.
    // Never waits, allocates or locks; drops the copy when the ring is full.
    void process(const float* left, const float* right, uint32_t numFrames) MIDI4U_NONBLOCKING;

    static uint64_t makeInstanceId();

private:
    const std::string memoryName_;
    const uint64_t instanceId_;
    TapOwnership ownership_;

    std::unique_ptr<SharedMemory> memory_;
    SpscRing ring_;
    std::atomic<TapHeader*> header_{nullptr};
    std::atomic<uint32_t> sampleRate_{0};
    std::atomic<TapPluginStatus> status_{TapPluginStatus::noBridge};

    uint64_t nextOpenMs_ = 0;
    bool readSeen_ = false;
    uint64_t lastReadIndex_ = 0;
    uint64_t lastReadChangeMs_ = 0;
};

} // namespace midi4u
