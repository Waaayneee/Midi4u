#pragma once

#include "Realtime.h"
#include "SpscRing.h"

#include <atomic>
#include <cstdint>
#include <vector>

struct SRC_STATE_tag;

namespace midi4u
{

// Pulls one source (music or mic) out of its ring at the output device's pace.
//
// A variable ratio resampler converts the source rate to the output rate. A small
// controller nudges the ratio, within +-0.5 percent, so the ring's low point stays
// near the target headroom. That absorbs clock drift between the two devices without
// the ring ever running dry or filling up.
//
// If the ring still runs dry, Sync outputs silence, waits for the ring to refill to the
// target, then fades back in. If it overfills, the oldest frames are dropped.
class Sync
{
public:
    enum class Quality
    {
        sinc,  // SRC_SINC_FASTEST, used by the app
        linear // SRC_LINEAR, cheap, for long simulations
    };

    static constexpr double kMaxCorrection = 0.005;

    explicit Sync(Quality quality = Quality::sinc);
    ~Sync();

    Sync(const Sync&) = delete;
    Sync& operator=(const Sync&) = delete;

    // Allocates everything. Call before audio starts, not on the audio thread.
    void prepare(int maxOutputFrames);

    // Starts over for a new source rate or output rate. A rate of 0 means "no source".
    void reset(double sourceRate, double outputRate) MIDI4U_NONBLOCKING;

    // Target headroom in milliseconds: how full the ring should be at its lowest point.
    void setTargetMs(double ms) MIDI4U_NONBLOCKING { targetMs_ = ms; }

    // Writes exactly numFrames (<= maxOutputFrames) to left and right.
    void process(SpscRing& ring, float* left, float* right, int numFrames) MIDI4U_NONBLOCKING;

    // Counters, safe to read from any thread.
    uint64_t underruns() const { return underruns_.load(std::memory_order_relaxed); }
    uint64_t overflows() const { return overflows_.load(std::memory_order_relaxed); }

    // For tests.
    double correction() const { return correction_; }
    bool isPriming() const { return priming_; }

private:
    double targetFrames() const { return targetMs_ * sourceRate_ / 1000.0; }
    void updateController(uint32_t fillAfter, int numFrames) MIDI4U_NONBLOCKING;

    const Quality quality_;
    SRC_STATE_tag* src_ = nullptr;
    std::vector<float> in_, out_;
    uint32_t inCapacity_ = 0;
    uint32_t inPos_ = 0, inCount_ = 0;
    int maxOutput_ = 0;

    double sourceRate_ = 0, outputRate_ = 0, nominalRatio_ = 1;
    double targetMs_ = 5.0;
    double correction_ = 0;
    double integral_ = 0;
    double smoothedLow_ = -1;
    uint32_t windowLow_ = UINT32_MAX;
    double windowFrames_ = 0;

    bool priming_ = true;
    float fade_ = 0, fadeStep_ = 1;

    std::atomic<uint64_t> underruns_{0}, overflows_{0};
};

} // namespace midi4u
