#include "TapProtocol.h"

namespace midi4u
{

TapCheck attachTap(void* memory, size_t bytes)
{
    if (memory == nullptr || bytes < kTapMappingBytes)
        return TapCheck::badSize;

    auto* header = static_cast<TapHeader*>(memory);
    uint32_t expected = 0;

    if (header->magic.compare_exchange_strong(expected, kTapMagicInitialising, std::memory_order_acq_rel))
    {
        header->version = kTapVersion;
        header->numChannels = kTapChannels;
        header->capacityFrames = kTapCapacityFrames;
        header->magic.store(kTapMagic, std::memory_order_release);
    }

    return checkTap(memory, bytes);
}

TapCheck checkTap(const void* memory, size_t bytes)
{
    if (memory == nullptr || bytes < sizeof(TapHeader))
        return TapCheck::badSize;

    const auto* header = static_cast<const TapHeader*>(memory);
    const uint32_t magic = header->magic.load(std::memory_order_acquire);

    if (magic == kTapMagicInitialising)
        return TapCheck::notReady;

    if (magic != kTapMagic)
        return TapCheck::badMagic;

    if (header->version != kTapVersion)
        return TapCheck::versionMismatch;

    const uint32_t capacity = header->capacityFrames;
    const bool powerOfTwo = capacity != 0 && (capacity & (capacity - 1)) == 0;

    if (header->numChannels != kTapChannels || ! powerOfTwo
        || kTapDataOffset + size_t(capacity) * kTapChannels * sizeof(float) > bytes)
        return TapCheck::badSize;

    return TapCheck::ok;
}

SpscRing tapRing(void* memory)
{
    auto* header = static_cast<TapHeader*>(memory);
    auto* data = reinterpret_cast<float*>(static_cast<char*>(memory) + kTapDataOffset);
    return SpscRing(data, header->capacityFrames, &header->writeIndex, &header->readIndex);
}

bool TapOwnership::update(TapHeader& header, uint64_t nowMs)
{
    uint64_t owner = header.ownerId.load(std::memory_order_acquire);

    if (owner == myId_)
        return true;

    if (owner == 0)
    {
        if (header.ownerId.compare_exchange_strong(owner, myId_, std::memory_order_acq_rel))
        {
            watching_ = false;
            return true;
        }
        // Someone else won; owner now holds their id.
    }

    const uint64_t heartbeat = header.heartbeat.load(std::memory_order_acquire);

    if (! watching_ || owner != lastOwner_ || heartbeat != lastHeartbeat_)
    {
        watching_ = true;
        lastOwner_ = owner;
        lastHeartbeat_ = heartbeat;
        lastChangeMs_ = nowMs;
        return false;
    }

    if (nowMs - lastChangeMs_ >= kTakeoverMs
        && header.ownerId.compare_exchange_strong(owner, myId_, std::memory_order_acq_rel))
    {
        watching_ = false;
        return true;
    }

    return false;
}

void TapOwnership::release(TapHeader& header)
{
    uint64_t expected = myId_;
    header.ownerId.compare_exchange_strong(expected, 0, std::memory_order_acq_rel);
    watching_ = false;
}

} // namespace midi4u
