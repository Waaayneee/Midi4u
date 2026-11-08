#include "Mixer.h"

#include <algorithm>
#include <cmath>

namespace midi4u
{

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kLimiterReleaseSeconds = 0.1;
constexpr double kToneFadeSeconds = 0.01;

float dbToGain(float db)
{
    return db <= -60.0f ? 0.0f : std::pow(10.0f, db / 20.0f);
}

float finite(float x)
{
    return std::isfinite(x) ? x : 0.0f;
}
} // namespace

void PeakMeter::add(const float* samples, int numFrames)
{
    float peak = 0.0f;

    for (int i = 0; i < numFrames; ++i)
        peak = std::max(peak, std::abs(finite(samples[i])));

    float old = peak_.load(std::memory_order_relaxed);

    while (peak > old && ! peak_.compare_exchange_weak(old, peak, std::memory_order_relaxed))
    {
    }
}

void Mixer::prepare(double sampleRate)
{
    sampleRate_ = sampleRate > 0 ? sampleRate : 48000.0;
    limiterRelease_ = float(1.0 - std::exp(-1.0 / (kLimiterReleaseSeconds * sampleRate_)));
    limiterGain_ = 1.0f;
}

void Mixer::setMusicGainDb(float db)
{
    musicGain_.store(dbToGain(db), std::memory_order_relaxed);
}

void Mixer::setMicGainDb(float db)
{
    micGain_.store(dbToGain(db), std::memory_order_relaxed);
}

void Mixer::process(const float* musicL, const float* musicR, const float* micL, const float* micR,
                    float* outL, float* outR, int numFrames)
{
    if (numFrames <= 0)
        return;

    musicMeter_.add(musicL, numFrames);
    musicMeter_.add(musicR, numFrames);

    // Ramp gains across the block so fader moves and mutes never click.
    const float musicTarget = musicMuted_.load(std::memory_order_relaxed) ? 0.0f : musicGain_.load(std::memory_order_relaxed);
    const float micTarget = micMuted_.load(std::memory_order_relaxed) ? 0.0f : micGain_.load(std::memory_order_relaxed);
    const float musicStep = (musicTarget - currentMusicGain_) / float(numFrames);
    const float micStep = (micTarget - currentMicGain_) / float(numFrames);

    const float toneSeconds = toneRequestSeconds_.exchange(0.0f, std::memory_order_relaxed);

    if (toneSeconds > 0.0f)
    {
        toneTotal_ = int64_t(toneSeconds * sampleRate_);
        tonePosition_ = 0;
    }

    const double toneStep = 2.0 * kPi * kToneHz / sampleRate_;
    const double toneFade = kToneFadeSeconds * sampleRate_;

    for (int i = 0; i < numFrames; ++i)
    {
        currentMusicGain_ += musicStep;
        currentMicGain_ += micStep;

        float left = finite(musicL[i]) * currentMusicGain_ + finite(micL[i]) * currentMicGain_;
        float right = finite(musicR[i]) * currentMusicGain_ + finite(micR[i]) * currentMicGain_;

        if (tonePosition_ < toneTotal_)
        {
            const double envelope = std::min({1.0, tonePosition_ / toneFade, (toneTotal_ - tonePosition_) / toneFade});
            const float tone = float(kToneLevel * envelope * std::sin(tonePhase_));
            tonePhase_ = std::fmod(tonePhase_ + toneStep, 2.0 * kPi);
            ++tonePosition_;
            left += tone;
            right += tone;
        }

        // Peak limiter: instant attack, smooth release, linked stereo. Never exceeds the ceiling.
        limiterGain_ += (1.0f - limiterGain_) * limiterRelease_;
        const float peak = std::max(std::abs(left), std::abs(right));

        if (peak * limiterGain_ > kCeiling)
            limiterGain_ = kCeiling / peak;

        outL[i] = std::clamp(left * limiterGain_, -kCeiling, kCeiling);
        outR[i] = std::clamp(right * limiterGain_, -kCeiling, kCeiling);
    }

    currentMusicGain_ = musicTarget;
    currentMicGain_ = micTarget;

    outputMeter_.add(outL, numFrames);
    outputMeter_.add(outR, numFrames);
}

} // namespace midi4u
