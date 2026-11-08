#pragma once

#include "Realtime.h"

#include <algorithm>
#include <atomic>
#include <cstdint>

namespace midi4u
{

// Lock free single producer, single consumer ring of interleaved stereo float frames.
// It works over memory it does not own, so the same code runs in shared memory
// (plugin to app) and on the heap (mic to output). Indices are 64 bit frame counters
// that only grow, so they never wrap in practice. Capacity must be a power of two.
class SpscRing
{
public:
    static constexpr uint32_t kChannels = 2;

    static_assert(std::atomic<uint64_t>::is_always_lock_free, "Shared memory needs lock free 64 bit atomics");

    SpscRing() = default;

    SpscRing(float* data, uint32_t capacityFrames, std::atomic<uint64_t>* writeIndex, std::atomic<uint64_t>* readIndex)
        : data_(data), capacity_(capacityFrames), mask_(capacityFrames - 1), write_(writeIndex), read_(readIndex)
    {
    }

    bool isValid() const { return data_ != nullptr && capacity_ != 0 && (capacity_ & mask_) == 0; }
    uint32_t capacity() const { return capacity_; }

    // ---- Producer side ----

    // Writes all n frames, or nothing if they do not fit. Returns false when dropped.
    bool write(const float* left, const float* right, uint32_t n) MIDI4U_NONBLOCKING
    {
        const uint64_t w = write_->load(std::memory_order_relaxed);
        const uint64_t r = read_->load(std::memory_order_acquire);
        const uint64_t used = w - r;

        if (used > capacity_ || capacity_ - used < n)
            return false;

        for (uint32_t i = 0; i < n; ++i)
        {
            float* frame = data_ + ((w + i) & mask_) * kChannels;
            frame[0] = left[i];
            frame[1] = right[i];
        }

        write_->store(w + n, std::memory_order_release);
        return true;
    }

    // ---- Consumer side ----

    // Frames ready to read. A corrupt index pair (more than capacity) reads as empty;
    // call isCorrupt() and discardAll() to recover.
    uint32_t available() const MIDI4U_NONBLOCKING
    {
        const uint64_t used = write_->load(std::memory_order_acquire) - read_->load(std::memory_order_relaxed);
        return used > capacity_ ? 0 : static_cast<uint32_t>(used);
    }

    bool isCorrupt() const MIDI4U_NONBLOCKING
    {
        return write_->load(std::memory_order_acquire) - read_->load(std::memory_order_relaxed) > capacity_;
    }

    // Reads up to n frames into an interleaved buffer. Returns frames read.
    uint32_t readInterleaved(float* dest, uint32_t n) MIDI4U_NONBLOCKING
    {
        const uint64_t r = read_->load(std::memory_order_relaxed);
        const uint32_t count = std::min(n, available());

        for (uint32_t i = 0; i < count; ++i)
        {
            const float* frame = data_ + ((r + i) & mask_) * kChannels;
            dest[i * 2] = frame[0];
            dest[i * 2 + 1] = frame[1];
        }

        read_->store(r + count, std::memory_order_release);
        return count;
    }

    // Reads up to n frames into separate channels. Returns frames read.
    uint32_t read(float* left, float* right, uint32_t n) MIDI4U_NONBLOCKING
    {
        const uint64_t r = read_->load(std::memory_order_relaxed);
        const uint32_t count = std::min(n, available());

        for (uint32_t i = 0; i < count; ++i)
        {
            const float* frame = data_ + ((r + i) & mask_) * kChannels;
            left[i] = frame[0];
            right[i] = frame[1];
        }

        read_->store(r + count, std::memory_order_release);
        return count;
    }

    // Drops the oldest n frames (or everything there is).
    void discard(uint32_t n) MIDI4U_NONBLOCKING
    {
        const uint32_t count = std::min(n, available());
        read_->store(read_->load(std::memory_order_relaxed) + count, std::memory_order_release);
    }

    // Drops everything, including recovering from corrupt indices.
    void discardAll() MIDI4U_NONBLOCKING
    {
        read_->store(write_->load(std::memory_order_acquire), std::memory_order_release);
    }

private:
    float* data_ = nullptr;
    uint32_t capacity_ = 0;
    uint32_t mask_ = 0;
    std::atomic<uint64_t>* write_ = nullptr;
    std::atomic<uint64_t>* read_ = nullptr;
};

// A ring that owns its memory, for rings inside the app. Allocates once, in the constructor.
class HeapRing
{
public:
    explicit HeapRing(uint32_t capacityFrames)
        : storage_(new float[static_cast<size_t>(capacityFrames) * SpscRing::kChannels]()),
          ring_(storage_, capacityFrames, &writeIndex_, &readIndex_)
    {
    }

    ~HeapRing() { delete[] storage_; }

    HeapRing(const HeapRing&) = delete;
    HeapRing& operator=(const HeapRing&) = delete;

    SpscRing& ring() { return ring_; }

private:
    std::atomic<uint64_t> writeIndex_{0};
    std::atomic<uint64_t> readIndex_{0};
    float* storage_;
    SpscRing ring_;
};

} // namespace midi4u
