#pragma once

#include <cmath>
#include <cstddef>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#if defined(_WIN32)
    #include <process.h>
    #define MIDI4U_GETPID _getpid
#else
    #include <unistd.h>
    #define MIDI4U_GETPID getpid
#endif

namespace testutils
{

constexpr double kPi = 3.14159265358979323846;

// Zeroed memory aligned like a shared memory mapping.
struct AlignedBlock
{
    explicit AlignedBlock(size_t size) : bytes(size), data(::operator new(size, std::align_val_t(64)))
    {
        std::memset(data, 0, size);
    }
    ~AlignedBlock() { ::operator delete(data, std::align_val_t(64)); }
    AlignedBlock(const AlignedBlock&) = delete;
    AlignedBlock& operator=(const AlignedBlock&) = delete;

    size_t bytes;
    void* data;
};

// Sine generator that keeps its phase across blocks.
struct Sine
{
    double hz, rate, level = 0.5, phase = 0;

    void fill(float* out, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            out[i] = float(level * std::sin(phase));
            phase = std::fmod(phase + 2.0 * kPi * hz / rate, 2.0 * kPi);
        }
    }
};

// Frequency from upward zero crossings (linear interpolated). Returns 0 if there is no tone.
inline double measureFrequency(const std::vector<float>& x, double rate)
{
    double first = -1, last = -1;
    int crossings = 0;

    for (size_t i = 1; i < x.size(); ++i)
    {
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
        {
            const double t = (double(i - 1) + x[i - 1] / double(x[i - 1] - x[i])) / rate;
            if (first < 0)
                first = t;
            last = t;
            ++crossings;
        }
    }

    return crossings > 1 ? (crossings - 1) / (last - first) : 0.0;
}

inline float peak(const std::vector<float>& x)
{
    float p = 0;
    for (float v : x)
        p = std::max(p, std::abs(v));
    return p;
}

// A shared memory name unique to this test process.
inline std::string uniqueMemoryName(const char* suffix)
{
#if defined(_WIN32)
    return "Local\\Midi4uTest." + std::to_string(MIDI4U_GETPID()) + "." + suffix;
#else
    return "/M4uT." + std::to_string(MIDI4U_GETPID()) + "." + suffix; // macOS limit is 31 chars
#endif
}

} // namespace testutils
