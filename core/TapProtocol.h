#pragma once

#include "SpscRing.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace midi4u
{

// Shared memory layout used between Midi4u Tap (writer) and the Midi4u App (reader).
//
//   [TapHeader, 256 bytes][capacityFrames * 2 interleaved float32]
//
// magic and version sit at offsets 0 and 4 forever, so any future version can still
// recognise this one and report a mismatch.

inline constexpr uint32_t kTapMagic = 0x5534444Du;             // "MD4U"
inline constexpr uint32_t kTapMagicInitialising = 0x54494E49u; // "INIT", set while a side fills in the header
inline constexpr uint32_t kTapVersion = 1;
inline constexpr uint32_t kTapChannels = 2;
inline constexpr uint32_t kTapCapacityFrames = 65536; // about 1.4 s at 48 kHz

#if defined(_WIN32)
inline constexpr const char* kTapName = "Local\\Midi4u.Tap.v1";
#else
inline constexpr const char* kTapName = "/Midi4u.Tap.v1";
#endif

struct TapHeader
{
    std::atomic<uint32_t> magic;
    uint32_t version;
    std::atomic<uint32_t> sampleRate; // Ableton's current rate, 0 until the plugin has played
    uint32_t numChannels;
    uint32_t capacityFrames;
    uint32_t reserved;

    alignas(64) std::atomic<uint64_t> writeIndex; // written only by the plugin
    alignas(64) std::atomic<uint64_t> readIndex;  // written only by the app
    alignas(64) std::atomic<uint64_t> heartbeat;  // bumped by the owning plugin every audio block
    std::atomic<uint64_t> ownerId;                // instance that owns the bridge, 0 when free
};

static_assert(sizeof(TapHeader) == 256, "TapHeader layout is part of the protocol");
static_assert(std::atomic<uint32_t>::is_always_lock_free, "Shared memory needs lock free atomics");

inline constexpr size_t kTapDataOffset = sizeof(TapHeader);
inline constexpr size_t kTapMappingBytes = kTapDataOffset + size_t(kTapCapacityFrames) * kTapChannels * sizeof(float);

enum class TapCheck
{
    ok,
    notReady,        // the other side is filling in the header right now
    badMagic,        // not Midi4u memory
    versionMismatch, // written by a different Midi4u version
    badSize          // impossible channel count or capacity
};

// Fills in a fresh (all zero) mapping, then validates it. Safe for both sides to call at once.
TapCheck attachTap(void* memory, size_t bytes);

// Checks magic, version and sizes before any read or write.
TapCheck checkTap(const void* memory, size_t bytes);

// Ring view over a mapping that passed checkTap.
SpscRing tapRing(void* memory);

// Decides which Tap instance owns the bridge. Every instance calls update() a few times
// per second from a non audio thread. The first instance claims ownership; others take
// over only after the owner's heartbeat has been still for kTakeoverMs.
class TapOwnership
{
public:
    static constexpr uint64_t kTakeoverMs = 1000;

    explicit TapOwnership(uint64_t myId) : myId_(myId) {}

    // Returns true when this instance owns the bridge.
    bool update(TapHeader& header, uint64_t nowMs);

    // Gives up ownership so another instance can claim it at once.
    void release(TapHeader& header);

    uint64_t id() const { return myId_; }

private:
    uint64_t myId_;
    bool watching_ = false;
    uint64_t lastOwner_ = 0;
    uint64_t lastHeartbeat_ = 0;
    uint64_t lastChangeMs_ = 0;
};

} // namespace midi4u
