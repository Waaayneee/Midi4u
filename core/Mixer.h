#pragma once

#include "Realtime.h"

#include <atomic>
#include <cstdint>

namespace midi4u
{

// Peak level shared between an audio thread (adds) and the UI (takes and resets).
class PeakMeter
{
public:
    void add(const float* samples, int numFrames) MIDI4U_NONBLOCKING;
    float take() { return peak_.exchange(0.0f, std::memory_order_relaxed); }

private:
    std::atomic<float> peak_{0.0f};
};

// Music gain and mute, mic gain and mute, an optional test tone, then a limiter
// that keeps the mix at or below -1 dBFS.
class Mixer
{
public:
    static constexpr float kCeiling = 0.89125094f; // -1 dBFS
    static constexpr float kToneHz = 440.0f;
    static constexpr float kToneLevel = 0.25f;     // -12 dBFS

    // Not on the audio thread.
    void prepare(double sampleRate);

    // Any thread.
    void setMusicGainDb(float db);
    void setMicGainDb(float db);
    void setMusicMuted(bool muted) { musicMuted_.store(muted, std::memory_order_relaxed); }
    void setMicMuted(bool muted) { micMuted_.store(muted, std::memory_order_relaxed); }
    void playTestTone(float seconds = 2.0f) { toneRequestSeconds_.store(seconds, std::memory_order_relaxed); }

    PeakMeter& musicMeter() { return musicMeter_; } // music before its fader
    PeakMeter& outputMeter() { return outputMeter_; }

    // Audio thread. Inputs may alias each other; outputs must not alias inputs.
    void process(const float* musicL, const float* musicR, const float* micL, const float* micR,
                 float* outL, float* outR, int numFrames) MIDI4U_NONBLOCKING;

private:
    std::atomic<float> musicGain_{1.0f}, micGain_{1.0f};
    std::atomic<bool> musicMuted_{false}, micMuted_{false};
    std::atomic<float> toneRequestSeconds_{0.0f};

    double sampleRate_ = 48000.0;
    float currentMusicGain_ = 1.0f, currentMicGain_ = 1.0f;
    float limiterGain_ = 1.0f, limiterRelease_ = 0.0f;

    int64_t toneTotal_ = 0, tonePosition_ = 0;
    double tonePhase_ = 0;

    PeakMeter musicMeter_, outputMeter_;
};

} // namespace midi4u
