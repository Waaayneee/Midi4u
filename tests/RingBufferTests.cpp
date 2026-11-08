#include "SpscRing.h"

#include <catch2/catch_test_macros.hpp>

#include <random>
#include <thread>
#include <vector>

using midi4u::HeapRing;

TEST_CASE("Ring wraps around and keeps frames in order", "[unit][ring]")
{
    HeapRing heap(8);
    auto& ring = heap.ring();
    std::vector<float> l(8), r(8);

    for (int round = 0; round < 20; ++round) // indices pass the end of the buffer many times
    {
        const uint32_t n = 1 + uint32_t(round % 7);
        std::vector<float> inL(n), inR(n);
        for (uint32_t i = 0; i < n; ++i)
        {
            inL[i] = float(round * 10 + int(i));
            inR[i] = -inL[i];
        }

        REQUIRE(ring.write(inL.data(), inR.data(), n));
        REQUIRE(ring.available() == n);
        REQUIRE(ring.read(l.data(), r.data(), 8) == n);

        for (uint32_t i = 0; i < n; ++i)
        {
            CHECK(l[i] == inL[i]);
            CHECK(r[i] == inR[i]);
        }
    }
}

TEST_CASE("Ring overflow drops the whole write", "[unit][ring]")
{
    HeapRing heap(8);
    auto& ring = heap.ring();
    std::vector<float> block(8, 1.0f);

    REQUIRE(ring.write(block.data(), block.data(), 6));
    CHECK_FALSE(ring.write(block.data(), block.data(), 3));
    CHECK(ring.available() == 6);
    CHECK(ring.write(block.data(), block.data(), 2));
    CHECK(ring.available() == 8);
    CHECK_FALSE(ring.write(block.data(), block.data(), 1));
}

TEST_CASE("Ring underflow reads only what is there", "[unit][ring]")
{
    HeapRing heap(16);
    auto& ring = heap.ring();
    std::vector<float> l(16), r(16), in(16, 0.5f);

    CHECK(ring.read(l.data(), r.data(), 4) == 0);
    REQUIRE(ring.write(in.data(), in.data(), 3));
    CHECK(ring.read(l.data(), r.data(), 10) == 3);
    CHECK(ring.available() == 0);
    CHECK(ring.readInterleaved(l.data(), 2) == 0);
}

TEST_CASE("Ring discard drops the oldest frames", "[unit][ring]")
{
    HeapRing heap(16);
    auto& ring = heap.ring();
    std::vector<float> in{1, 2, 3, 4, 5}, l(5), r(5);

    REQUIRE(ring.write(in.data(), in.data(), 5));
    ring.discard(2);
    REQUIRE(ring.read(l.data(), r.data(), 5) == 3);
    CHECK(l[0] == 3);

    REQUIRE(ring.write(in.data(), in.data(), 5));
    ring.discardAll();
    CHECK(ring.available() == 0);
}

TEST_CASE("Ring treats impossible indices as corrupt and recovers", "[unit][ring]")
{
    std::atomic<uint64_t> w{5}, r{100}; // reader ahead of writer: impossible
    std::vector<float> data(32);
    midi4u::SpscRing ring(data.data(), 16, &w, &r);

    CHECK(ring.isCorrupt());
    CHECK(ring.available() == 0);
    float x = 0;
    CHECK_FALSE(ring.write(&x, &x, 1));
    ring.discardAll();
    CHECK_FALSE(ring.isCorrupt());
    CHECK(ring.write(&x, &x, 1));
}

TEST_CASE("Ring survives a two thread stress test", "[unit][ring]")
{
    constexpr uint64_t kTotalFrames = 8'000'000;
    HeapRing heap(1024);
    auto& ring = heap.ring();

    auto valueOf = [](uint64_t i) { return float(i % 1'000'003); }; // exact in float

    std::thread producer([&] {
        std::mt19937 random(1);
        std::vector<float> l(64), r(64);
        uint64_t next = 0;

        while (next < kTotalFrames)
        {
            const uint32_t n = uint32_t(std::min<uint64_t>(1 + random() % 64, kTotalFrames - next));
            for (uint32_t i = 0; i < n; ++i)
            {
                l[i] = valueOf(next + i);
                r[i] = -l[i];
            }
            if (ring.write(l.data(), r.data(), n))
                next += n;
            else
                std::this_thread::yield();
        }
    });

    std::mt19937 random(2);
    std::vector<float> l(64), r(64);
    uint64_t expected = 0;
    uint64_t mismatches = 0;

    while (expected < kTotalFrames)
    {
        const uint32_t got = ring.read(l.data(), r.data(), 1 + random() % 64);
        for (uint32_t i = 0; i < got; ++i, ++expected)
            if (l[i] != valueOf(expected) || r[i] != -valueOf(expected))
                ++mismatches;
        if (got == 0)
            std::this_thread::yield();
    }

    producer.join();
    CHECK(mismatches == 0);
    CHECK(ring.available() == 0);
}
