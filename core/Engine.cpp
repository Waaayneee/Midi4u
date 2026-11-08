#include "Engine.h"

#include <algorithm>

namespace midi4u
{

LatencySettings latencySettings(Latency latency)
{
    switch (latency)
    {
        case Latency::low:
            return {2.5, 5.0};
        case Latency::safe:
            return {12.0, 20.0};
        case Latency::normal:
            break;
    }
    return {5.0, 10.0};
}

Engine::Engine(Sync::Quality quality)
    : tapSync_(quality), micSync_(quality), ringTargetMs_(latencySettings(Latency::normal).ringTargetMs)
{
}

Engine::Counters Engine::counters() const
{
    return {tapSync_.underruns(), tapSync_.overflows(), micSync_.underruns(), micSync_.overflows()};
}

void Engine::outputStarting(double sampleRate, int maxFrames)
{
    // Called before callbacks start, never during one, so allocating here is fine.
    outputRate_ = sampleRate;
    maxChunk_ = std::clamp(maxFrames, 64, 8192);

    for (auto* buffer : {&musicL_, &musicR_, &micL_, &micR_})
        buffer->assign(size_t(maxChunk_), 0.0f);

    tapSync_.prepare(maxChunk_);
    micSync_.prepare(maxChunk_);
    mixer_.prepare(sampleRate);
    tapReader_.forceReconnect();
    micResetPending_ = true;
}

void Engine::renderOutput(float* left, float* right, int numFrames)
{
    if (maxChunk_ == 0)
    {
        std::fill(left, left + numFrames, 0.0f);
        std::fill(right, right + numFrames, 0.0f);
        return;
    }

    for (int done = 0; done < numFrames;)
    {
        const int chunk = std::min(numFrames - done, maxChunk_);
        renderChunk(left + done, right + done, chunk);
        done += chunk;
    }
}

void Engine::renderChunk(float* left, float* right, int numFrames)
{
    const double targetMs = ringTargetMs_.load(std::memory_order_relaxed);
    tapSync_.setTargetMs(targetMs);
    micSync_.setTargetMs(targetMs);

    // Music from Ableton
    if (tapReader_.poll(uint32_t(numFrames), outputRate_))
    {
        if (tapReader_.takeReset())
            tapSync_.reset(tapReader_.sampleRate(), outputRate_);

        tapSync_.process(tapReader_.ring(), musicL_.data(), musicR_.data(), numFrames);
    }
    else
    {
        std::fill_n(musicL_.data(), numFrames, 0.0f);
        std::fill_n(musicR_.data(), numFrames, 0.0f);
    }

    // Mic
    const uint32_t epoch = micEpoch_.load(std::memory_order_acquire);

    if (epoch != seenMicEpoch_ || micResetPending_)
    {
        seenMicEpoch_ = epoch;
        micResetPending_ = false;
        micRing_.ring().discardAll();
        micSync_.reset(micRate_.load(std::memory_order_relaxed), outputRate_);
    }

    micSync_.process(micRing_.ring(), micL_.data(), micR_.data(), numFrames); // silence while no mic

    mixer_.process(musicL_.data(), musicR_.data(), micL_.data(), micR_.data(), left, right, numFrames);
}

void Engine::micStarting(double sampleRate, int)
{
    micRate_.store(sampleRate, std::memory_order_relaxed);
    micEpoch_.fetch_add(1, std::memory_order_release);
}

void Engine::micCaptured(const float* left, const float* right, int numFrames)
{
    micMeter_.add(left, numFrames);
    micMeter_.add(right, numFrames);
    micRing_.ring().write(left, right, uint32_t(numFrames)); // dropped if the output is not running
}

void Engine::micStopped()
{
    micRate_.store(0, std::memory_order_relaxed);
    micEpoch_.fetch_add(1, std::memory_order_release);
}

} // namespace midi4u
