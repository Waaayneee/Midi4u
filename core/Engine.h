#pragma once

#include "AudioEndpoint.h"
#include "Mixer.h"
#include "SpscRing.h"
#include "Sync.h"
#include "TapReader.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace midi4u
{

enum class Latency
{
    low,
    normal,
    safe
};

struct LatencySettings
{
    double ringTargetMs; // Sync headroom for each ring
    double outputBufferMs;
};

LatencySettings latencySettings(Latency latency);

// The app's audio graph, independent of which output sink is in use:
//
//   Tap ring --Sync--\
//                     Mixer --> output (its callback is the master clock)
//   Mic ring --Sync--/
class Engine final : public OutputCallback, public MicCallback
{
public:
    static constexpr uint32_t kMicRingFrames = 32768;

    explicit Engine(Sync::Quality quality = Sync::Quality::sinc);

    // Message thread, once the shared memory is mapped.
    void setTapMemory(void* memory, size_t bytes) { tapReader_.setMemory(memory, bytes); }

    // Any thread.
    void setLatency(Latency latency) { ringTargetMs_.store(latencySettings(latency).ringTargetMs, std::memory_order_relaxed); }
    Mixer& mixer() { return mixer_; }
    TapReaderStatus tapStatus() const { return tapReader_.status(); }
    PeakMeter& micMeter() { return micMeter_; } // raw mic level, works even with no output

    struct Counters
    {
        uint64_t musicUnderruns, musicOverflows, micUnderruns, micOverflows;
    };
    Counters counters() const;

    // OutputCallback
    void outputStarting(double sampleRate, int maxFrames) override;
    void renderOutput(float* left, float* right, int numFrames) override;
    void outputStopped() override {}

    // MicCallback
    void micStarting(double sampleRate, int maxFrames) override;
    void micCaptured(const float* left, const float* right, int numFrames) override;
    void micStopped() override;

private:
    void renderChunk(float* left, float* right, int numFrames) MIDI4U_NONBLOCKING;

    TapReader tapReader_;
    Sync tapSync_, micSync_;
    HeapRing micRing_{kMicRingFrames};
    Mixer mixer_;
    PeakMeter micMeter_;

    std::atomic<double> ringTargetMs_;
    std::atomic<double> micRate_{0};
    std::atomic<uint32_t> micEpoch_{0};

    // Output thread only.
    double outputRate_ = 0;
    int maxChunk_ = 0;
    uint32_t seenMicEpoch_ = 0;
    bool micResetPending_ = true;
    std::vector<float> musicL_, musicR_, micL_, micR_;
};

} // namespace midi4u
