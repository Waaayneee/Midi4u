#include "Sync.h"

#include <samplerate.h>

#include <algorithm>
#include <cmath>

namespace midi4u
{

namespace
{
constexpr uint32_t kInputChunkFrames = 1024;
constexpr double kFadeInMs = 5.0;
constexpr double kOverflowExtraMs = 100.0; // fill this far above target counts as overflow

// Controller. The ring's low point is measured over each window, smoothed, and compared
// with the target. An error of kErrorScaleMs gives the full proportional correction.
constexpr double kWindowSeconds = 0.25;
constexpr double kSmoothing = 0.25;
constexpr double kErrorScaleMs = 10.0;
constexpr double kProportionalGain = Sync::kMaxCorrection;
constexpr double kIntegralGainPerSecond = 0.0005;

void clear(float* left, float* right, int numFrames)
{
    std::fill(left, left + numFrames, 0.0f);
    std::fill(right, right + numFrames, 0.0f);
}
} // namespace

Sync::Sync(Quality quality) : quality_(quality) {}

Sync::~Sync()
{
    if (src_ != nullptr)
        src_delete(src_);
}

void Sync::prepare(int maxOutputFrames)
{
    maxOutput_ = std::max(1, maxOutputFrames);

    if (src_ == nullptr)
    {
        int error = 0;
        src_ = src_new(quality_ == Quality::sinc ? SRC_SINC_FASTEST : SRC_LINEAR, int(SpscRing::kChannels), &error);
    }

    inCapacity_ = kInputChunkFrames;
    in_.assign(size_t(inCapacity_) * SpscRing::kChannels, 0.0f);
    out_.assign(size_t(maxOutput_) * SpscRing::kChannels, 0.0f);
    reset(sourceRate_, outputRate_);
}

void Sync::reset(double sourceRate, double outputRate)
{
    sourceRate_ = sourceRate;
    outputRate_ = outputRate;
    nominalRatio_ = (sourceRate > 0 && outputRate > 0) ? outputRate / sourceRate : 1.0;

    if (src_ != nullptr)
        src_reset(src_);

    inPos_ = inCount_ = 0;
    correction_ = integral_ = 0;
    smoothedLow_ = -1;
    windowLow_ = UINT32_MAX;
    windowFrames_ = 0;
    priming_ = true;
    fade_ = 0;
    fadeStep_ = outputRate > 0 ? float(1000.0 / (kFadeInMs * outputRate)) : 1.0f;
}

void Sync::process(SpscRing& ring, float* left, float* right, int numFrames)
{
    if (numFrames <= 0)
        return;

    if (src_ == nullptr || numFrames > maxOutput_ || sourceRate_ <= 0 || outputRate_ <= 0 || ! ring.isValid())
    {
        clear(left, right, numFrames);
        return;
    }

    const double target = targetFrames();
    const double ratio = nominalRatio_ * (1.0 + correction_);

    if (priming_)
    {
        // Wait until there is the target headroom plus one block to read.
        if (ring.available() + inCount_ < target + numFrames / ratio)
        {
            clear(left, right, numFrames);
            return;
        }

        priming_ = false;
        fade_ = 0;
    }

    int produced = 0;

    while (produced < numFrames)
    {
        if (inCount_ == 0)
        {
            const double needed = std::ceil((numFrames - produced) / ratio) + 2.0;
            inPos_ = 0;
            inCount_ = ring.readInterleaved(in_.data(), uint32_t(std::min<double>(needed, inCapacity_)));

            if (inCount_ == 0)
                break;
        }

        SRC_DATA data{};
        data.data_in = in_.data() + size_t(inPos_) * SpscRing::kChannels;
        data.input_frames = long(inCount_);
        data.data_out = out_.data() + size_t(produced) * SpscRing::kChannels;
        data.output_frames = long(numFrames - produced);
        data.src_ratio = ratio;
        data.end_of_input = 0;

        if (src_process(src_, &data) != 0)
            break;

        inPos_ += uint32_t(data.input_frames_used);
        inCount_ -= uint32_t(data.input_frames_used);
        produced += int(data.output_frames_gen);

        if (data.input_frames_used == 0 && data.output_frames_gen == 0)
            break;
    }

    for (int i = 0; i < produced; ++i)
    {
        float gain = 1.0f;

        if (fade_ < 1.0f)
        {
            fade_ = std::min(1.0f, fade_ + fadeStep_);
            gain = fade_;
        }

        left[i] = out_[size_t(i) * 2] * gain;
        right[i] = out_[size_t(i) * 2 + 1] * gain;
    }

    if (produced < numFrames)
    {
        // Ran dry: silence now, refill to the target, then fade back in.
        clear(left + produced, right + produced, numFrames - produced);
        underruns_.fetch_add(1, std::memory_order_relaxed);
        priming_ = true;
        return;
    }

    const uint32_t fillAfter = ring.available() + inCount_;

    if (fillAfter > target + kOverflowExtraMs * sourceRate_ / 1000.0)
    {
        // Far too full (the reader stalled, or a source runs much faster): drop the oldest audio.
        ring.discard(uint32_t(fillAfter - target));
        overflows_.fetch_add(1, std::memory_order_relaxed);
        smoothedLow_ = -1;
        windowLow_ = UINT32_MAX;
        windowFrames_ = 0;
        return;
    }

    updateController(fillAfter, numFrames);
}

void Sync::updateController(uint32_t fillAfter, int numFrames)
{
    windowLow_ = std::min(windowLow_, fillAfter);
    windowFrames_ += numFrames;

    if (windowFrames_ < kWindowSeconds * outputRate_)
        return;

    const double windowSeconds = windowFrames_ / outputRate_;
    smoothedLow_ = smoothedLow_ < 0 ? double(windowLow_) : smoothedLow_ + kSmoothing * (windowLow_ - smoothedLow_);
    windowLow_ = UINT32_MAX;
    windowFrames_ = 0;

    // Positive error: too full, so read faster (a lower ratio). Negative: read slower.
    const double error = (smoothedLow_ - targetFrames()) / (kErrorScaleMs * sourceRate_ / 1000.0);
    integral_ = std::clamp(integral_ + kIntegralGainPerSecond * error * windowSeconds, -kMaxCorrection, kMaxCorrection);
    correction_ = std::clamp(-(kProportionalGain * error + integral_), -kMaxCorrection, kMaxCorrection);
}

} // namespace midi4u
