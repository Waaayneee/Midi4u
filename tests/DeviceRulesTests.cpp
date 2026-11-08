#include "DeviceRules.h"

#include <catch2/catch_test_macros.hpp>

using namespace midi4u;

TEST_CASE("Feedback loops are blocked", "[unit][devices]")
{
    CHECK(wouldFeedBack("CABLE Output (VB-Audio Virtual Cable)", "CABLE Input (VB-Audio Virtual Cable)"));
    CHECK(wouldFeedBack("CABLE Output (VB-Audio Virtual Cable)", "Speakers (Realtek)")); // always hidden
    CHECK(wouldFeedBack("CABLE-A Output (VB-Audio Cable A)", ""));
    CHECK(wouldFeedBack("Stereo Mix (Realtek(R) Audio)", "CABLE Input (VB-Audio Virtual Cable)"));
    CHECK(wouldFeedBack("BlackHole 2ch", "BlackHole 2ch"));

    CHECK_FALSE(wouldFeedBack("Headset Microphone (Arctis 7)", "CABLE Input (VB-Audio Virtual Cable)"));
    CHECK_FALSE(wouldFeedBack("Microphone (Scarlett 2i2 USB)", "CABLE Input (VB-Audio Virtual Cable)"));
    CHECK_FALSE(wouldFeedBack("BlackHole 2ch", "CABLE Input (VB-Audio Virtual Cable)"));
}

TEST_CASE("Mic dropdown hides feedback devices", "[unit][devices]")
{
    const std::vector<DeviceInfo> mics{{"1", "Headset Microphone (Arctis 7)"},
                                       {"2", "CABLE Output (VB-Audio Virtual Cable)"},
                                       {"3", "Stereo Mix (Realtek(R) Audio)"},
                                       {"4", "Microphone Array (Intel)"}};

    const auto usable = usableMics(mics, "CABLE Input (VB-Audio Virtual Cable)");
    REQUIRE(usable.size() == 2);
    CHECK(usable[0].id == "1");
    CHECK(usable[1].id == "4");
}

TEST_CASE("Reconnect finds the same device and never another one", "[unit][devices]")
{
    const DeviceInfo saved{"{id-headset}", "Headset Microphone (Arctis 7)"};

    SECTION("same ID, even if renamed")
    {
        const auto found = findSavedDevice(saved, {{"{id-laptop}", "Microphone Array"}, {"{id-headset}", "Headset Mic (2- Arctis 7)"}});
        REQUIRE(found);
        CHECK(found->id == "{id-headset}");
    }

    SECTION("same name with a new ID, for example another USB port")
    {
        const auto found = findSavedDevice(saved, {{"{id-new}", "Headset Microphone (Arctis 7)"}});
        REQUIRE(found);
        CHECK(found->id == "{id-new}");
    }

    SECTION("missing: no fallback to another mic")
    {
        CHECK_FALSE(findSavedDevice(saved, {{"{id-laptop}", "Microphone Array"}}));
        CHECK_FALSE(findSavedDevice(saved, {}));
    }

    SECTION("empty saved device matches nothing")
    {
        CHECK_FALSE(findSavedDevice({}, {{"", ""}, {"x", "Microphone Array"}}));
    }
}

TEST_CASE("Cable output is found by prefix", "[unit][devices]")
{
    const std::vector<DeviceInfo> outputs{{"1", "Speakers (Realtek)"}, {"2", "CABLE Input (VB-Audio Virtual Cable)"}};
    const auto cable = findByPrefix(outputs, "CABLE Input");
    REQUIRE(cable);
    CHECK(cable->id == "2");
    CHECK_FALSE(findByPrefix(outputs, "BlackHole"));
}
