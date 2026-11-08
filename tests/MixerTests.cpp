#include "Mixer.h"
#include "TestUtils.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <random>
#include <vector>

using namespace midi4u;
using testutils::peak;

namespace
{
struct Rig
{
    Mixer mixer;
    std::vector<float> musicL, musicR, micL, micR, outL, outR;

    explicit Rig(int n = 480)
        : musicL(size_t(n)), musicR(size_t(n)), micL(size_t(n)), micR(size_t(n)), outL(size_t(n)), outR(size_t(n))
    {
        mixer.prepare(48000);
    }

    void run(int blocks = 1)
    {
        for (int i = 0; i < blocks; ++i)
            mixer.process(musicL.data(), musicR.data(), micL.data(), micR.data(), outL.data(), outR.data(), int(outL.size()));
    }
};
} // namespace

TEST_CASE("Mixer applies gains and sums the sources", "[unit][mixer]")
{
    Rig rig;
    std::fill(rig.musicL.begin(), rig.musicL.end(), 0.2f);
    std::fill(rig.musicR.begin(), rig.musicR.end(), 0.1f);
    std::fill(rig.micL.begin(), rig.micL.end(), 0.1f);
    std::fill(rig.micR.begin(), rig.micR.end(), 0.1f);

    rig.mixer.setMusicGainDb(-6.0f);
    rig.mixer.setMicGainDb(0.0f);
    rig.run(2); // first block ramps to the new gain

    const float half = std::pow(10.0f, -6.0f / 20.0f);
    CHECK(rig.outL.back() == Catch::Approx(0.2f * half + 0.1f).epsilon(1e-5));
    CHECK(rig.outR.back() == Catch::Approx(0.1f * half + 0.1f).epsilon(1e-5));
}

TEST_CASE("Mutes silence their source only", "[unit][mixer]")
{
    Rig rig;
    std::fill(rig.musicL.begin(), rig.musicL.end(), 0.3f);
    std::fill(rig.micL.begin(), rig.micL.end(), 0.2f);

    rig.mixer.setMusicMuted(true);
    rig.run(2);
    CHECK(rig.outL.back() == Catch::Approx(0.2f));

    rig.mixer.setMusicMuted(false);
    rig.mixer.setMicMuted(true);
    rig.run(2);
    CHECK(rig.outL.back() == Catch::Approx(0.3f));

    rig.mixer.setMusicMuted(true);
    rig.run(2);
    CHECK(peak(rig.outL) == 0.0f);
}

TEST_CASE("Gain changes ramp instead of jumping", "[unit][mixer]")
{
    Rig rig;
    std::fill(rig.musicL.begin(), rig.musicL.end(), 0.5f);
    rig.run();
    rig.mixer.setMusicMuted(true);
    rig.run();

    for (size_t i = 1; i < rig.outL.size(); ++i)
        CHECK(std::abs(rig.outL[i] - rig.outL[i - 1]) < 0.01f);
}

TEST_CASE("Limiter never exceeds its ceiling", "[unit][mixer]")
{
    Rig rig;
    rig.mixer.setMusicGainDb(12.0f);
    rig.mixer.setMicGainDb(12.0f);
    std::mt19937 random(7);
    std::uniform_real_distribution<float> loud(-10.0f, 10.0f);

    for (int block = 0; block < 2000; ++block)
    {
        for (size_t i = 0; i < rig.musicL.size(); ++i)
        {
            const float scale = (block / 100) % 2 == 0 ? 1.0f : 0.01f; // loud and quiet passages
            rig.musicL[i] = loud(random) * scale;
            rig.musicR[i] = loud(random) * scale;
            rig.micL[i] = loud(random) * scale;
            rig.micR[i] = loud(random) * scale;
        }
        rig.run();
        REQUIRE(peak(rig.outL) <= Mixer::kCeiling);
        REQUIRE(peak(rig.outR) <= Mixer::kCeiling);
    }
}

TEST_CASE("Limiter leaves quiet audio untouched", "[unit][mixer]")
{
    Rig rig;
    std::fill(rig.musicL.begin(), rig.musicL.end(), 0.5f);
    rig.run(3);
    CHECK(rig.outL.back() == Catch::Approx(0.5f));
}

TEST_CASE("Bad samples never reach the output", "[unit][mixer]")
{
    Rig rig;
    rig.musicL[3] = std::numeric_limits<float>::quiet_NaN();
    rig.micR[5] = std::numeric_limits<float>::infinity();
    rig.run();

    for (size_t i = 0; i < rig.outL.size(); ++i)
    {
        CHECK(std::isfinite(rig.outL[i]));
        CHECK(std::isfinite(rig.outR[i]));
    }
}

TEST_CASE("Test tone plays into the mix and stops", "[unit][mixer]")
{
    Rig rig;
    rig.mixer.playTestTone(0.1f); // 4800 frames
    rig.run(5);
    CHECK(peak(rig.outL) == Catch::Approx(Mixer::kToneLevel).margin(0.01));
    rig.run(10);
    CHECK(peak(rig.outL) == 0.0f);
}
