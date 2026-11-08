#include "TapProtocol.h"
#include "TestUtils.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

using namespace midi4u;
using testutils::AlignedBlock;

namespace
{
TapHeader& headerOf(AlignedBlock& block)
{
    return *static_cast<TapHeader*>(block.data);
}
} // namespace

TEST_CASE("Fresh memory is initialised and accepted", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);

    auto& header = headerOf(block);
    CHECK(header.magic.load() == kTapMagic);
    CHECK(header.version == kTapVersion);
    CHECK(header.numChannels == 2);
    CHECK(header.capacityFrames == kTapCapacityFrames);

    // Attaching again (the other side) leaves it alone.
    header.writeIndex.store(123);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);
    CHECK(header.writeIndex.load() == 123);
}

TEST_CASE("Header check rejects bad magic", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    std::memset(block.data, 0xAB, sizeof(TapHeader));
    CHECK(checkTap(block.data, block.bytes) == TapCheck::badMagic);
    CHECK(attachTap(block.data, block.bytes) == TapCheck::badMagic);
}

TEST_CASE("Header check rejects a wrong version", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);
    headerOf(block).version = kTapVersion + 1;
    CHECK(checkTap(block.data, block.bytes) == TapCheck::versionMismatch);
}

TEST_CASE("Header check rejects impossible sizes", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);
    auto& header = headerOf(block);

    header.capacityFrames = 1000; // not a power of two
    CHECK(checkTap(block.data, block.bytes) == TapCheck::badSize);

    header.capacityFrames = kTapCapacityFrames * 2; // larger than the mapping
    CHECK(checkTap(block.data, block.bytes) == TapCheck::badSize);

    header.capacityFrames = 0;
    CHECK(checkTap(block.data, block.bytes) == TapCheck::badSize);

    header.capacityFrames = kTapCapacityFrames;
    header.numChannels = 3;
    CHECK(checkTap(block.data, block.bytes) == TapCheck::badSize);

    header.numChannels = 2;
    CHECK(checkTap(block.data, block.bytes) == TapCheck::ok);
    CHECK(checkTap(block.data, kTapMappingBytes - 1) == TapCheck::badSize);
    CHECK(checkTap(nullptr, kTapMappingBytes) == TapCheck::badSize);
}

TEST_CASE("Header check waits while the other side initialises", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    headerOf(block).magic.store(kTapMagicInitialising);
    CHECK(checkTap(block.data, block.bytes) == TapCheck::notReady);
}

TEST_CASE("First Tap owns the bridge, others wait for its heartbeat to stop", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);
    auto& header = headerOf(block);

    TapOwnership first(11), second(22);

    CHECK(first.update(header, 0));
    CHECK_FALSE(second.update(header, 0));
    CHECK(first.update(header, 10));

    // A live owner keeps its bridge no matter how long.
    for (uint64_t t = 100; t <= 5000; t += 100)
    {
        header.heartbeat.fetch_add(1);
        CHECK_FALSE(second.update(header, t));
    }

    // Heartbeat stops: no takeover before one second, takeover after.
    CHECK_FALSE(second.update(header, 5500));
    CHECK_FALSE(second.update(header, 5999));
    CHECK(second.update(header, 6000));
    CHECK(header.ownerId.load() == 22);
    CHECK_FALSE(first.update(header, 6000));
}

TEST_CASE("Releasing ownership lets another Tap claim at once", "[unit][protocol]")
{
    AlignedBlock block(kTapMappingBytes);
    REQUIRE(attachTap(block.data, block.bytes) == TapCheck::ok);
    auto& header = headerOf(block);

    TapOwnership first(1), second(2);
    REQUIRE(first.update(header, 0));
    REQUIRE_FALSE(second.update(header, 0));

    first.release(header);
    CHECK(header.ownerId.load() == 0);
    CHECK(second.update(header, 1));

    // Releasing when not the owner changes nothing.
    first.release(header);
    CHECK(header.ownerId.load() == 2);
}
