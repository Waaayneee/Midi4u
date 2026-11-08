#include "TapPluginSide.h"

#include <chrono>
#include <cmath>
#include <random>

namespace midi4u
{

namespace
{
constexpr uint64_t kOpenRetryMs = 1000;
constexpr uint64_t kAppSilentMs = 1000;
} // namespace

TapPluginSide::TapPluginSide(std::string memoryName, uint64_t instanceId)
    : memoryName_(std::move(memoryName)), instanceId_(instanceId), ownership_(instanceId)
{
}

TapPluginSide::~TapPluginSide()
{
    if (auto* header = header_.load(std::memory_order_acquire))
        ownership_.release(*header);
}

uint64_t TapPluginSide::makeInstanceId()
{
    std::random_device device;
    const uint64_t random = (uint64_t(device()) << 32) ^ uint64_t(device());
    const uint64_t time = uint64_t(std::chrono::steady_clock::now().time_since_epoch().count());
    const uint64_t id = random ^ (time * 0x9E3779B97F4A7C15ull);
    return id == 0 ? 1 : id;
}

void TapPluginSide::setSampleRate(double sampleRate)
{
    sampleRate_.store(static_cast<uint32_t>(std::lround(sampleRate)), std::memory_order_relaxed);
}

void TapPluginSide::tick(uint64_t nowMs)
{
    if (memory_ == nullptr)
    {
        if (nowMs < nextOpenMs_)
            return;

        nextOpenMs_ = nowMs + kOpenRetryMs;
        std::string error;
        memory_ = SharedMemory::openOrCreate(memoryName_, kTapMappingBytes, error);

        if (memory_ == nullptr)
        {
            status_.store(TapPluginStatus::noBridge, std::memory_order_relaxed);
            return;
        }

        attachTap(memory_->data(), memory_->size());
    }

    auto* header = header_.load(std::memory_order_relaxed);

    if (header == nullptr)
    {
        switch (checkTap(memory_->data(), memory_->size()))
        {
            case TapCheck::ok:
                break;
            case TapCheck::versionMismatch:
                status_.store(TapPluginStatus::versionMismatch, std::memory_order_relaxed);
                return;
            case TapCheck::notReady:
                return; // the app is filling in the header; try again next tick
            case TapCheck::badMagic:
            case TapCheck::badSize:
                status_.store(TapPluginStatus::noBridge, std::memory_order_relaxed);
                return;
        }

        header = static_cast<TapHeader*>(memory_->data());
        ring_ = tapRing(header);
        header_.store(header, std::memory_order_release); // publishes ring_ to the audio thread
    }

    if (! ownership_.update(*header, nowMs))
    {
        readSeen_ = false;
        status_.store(TapPluginStatus::anotherTapActive, std::memory_order_relaxed);
        return;
    }

    const uint64_t readIndex = header->readIndex.load(std::memory_order_relaxed);

    if (! readSeen_)
    {
        readSeen_ = true;
        lastReadIndex_ = readIndex;
        lastReadChangeMs_ = nowMs - kAppSilentMs; // "waiting" until the app is seen reading
    }
    else if (readIndex != lastReadIndex_)
    {
        lastReadIndex_ = readIndex;
        lastReadChangeMs_ = nowMs;
    }

    status_.store(nowMs - lastReadChangeMs_ < kAppSilentMs ? TapPluginStatus::connected
                                                           : TapPluginStatus::waitingForApp,
                  std::memory_order_relaxed);
}

void TapPluginSide::process(const float* left, const float* right, uint32_t numFrames)
{
    auto* header = header_.load(std::memory_order_acquire);

    if (header == nullptr || header->ownerId.load(std::memory_order_acquire) != instanceId_)
        return;

    const uint32_t rate = sampleRate_.load(std::memory_order_relaxed);

    if (rate == 0)
        return;

    if (header->sampleRate.load(std::memory_order_relaxed) != rate)
        header->sampleRate.store(rate, std::memory_order_release);

    ring_.write(left, right, numFrames); // dropped if the app is not running or the ring is full
    header->heartbeat.fetch_add(1, std::memory_order_release);
}

} // namespace midi4u
