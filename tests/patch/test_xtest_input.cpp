#include <catch2/catch_test_macros.hpp>
#include "xtest_input_helpers.hpp"
#include <cstring>
#include <vector>
#include <linux/input-event-codes.h>
using gamescope::xtest_input::Motion;

namespace {
using Motions = std::vector<std::array<double, 2>>;
bool DispatchAll(uint8_t category, std::span<const uint8_t> data, uint8_t opcode,
    bool swapped, bool focused, Motions &motions, const gamescope::xtest_input::KeySink &keys = {})
{
    do {
        gamescope::xtest_input::DispatchBudget budget;
        if (!gamescope::xtest_input::DispatchRecordData(category, data, opcode, swapped, focused, budget,
                keys, [&](double x, double y) { motions.push_back({x, y}); })) return false;
    } while (!data.empty());
    return true;
}
}
namespace {
std::array<uint8_t, 36> Request(int16_t x = 7, int16_t y = -5, bool swapped = false)
{
    std::array<uint8_t, 36> request{};
    request[0] = 132; request[1] = 2; request[4] = 6; request[5] = 1;
    auto word = [&](size_t offset, uint16_t value) {
        if (swapped) value = uint16_t((value >> 8) | (value << 8));
        std::memcpy(request.data() + offset, &value, 2);
    };
    word(2, 9); word(24, uint16_t(x)); word(26, uint16_t(y));
    return request;
}
}
namespace {
struct KeyboardFixture {
    std::unique_ptr<xkb_context, decltype(&xkb_context_unref)> context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), xkb_context_unref};
    xkb_rule_names names;
    std::unique_ptr<xkb_keymap, decltype(&xkb_keymap_unref)> map{xkb_keymap_new_from_names(context.get(), &names, XKB_KEYMAP_COMPILE_NO_FLAGS), xkb_keymap_unref};
    gamescope::xtest_input::KeyboardState state;
    KeyboardFixture(const char *layout = "us") : names{.layout = layout} { REQUIRE(map); REQUIRE(state.SetKeymap(map.get())); }
    uint32_t Mask(const char *name) { return 1u << xkb_keymap_mod_get_index(map.get(), name); }
};
}
TEST_CASE("XTEST F presses and releases decode as focused immediate evdev keys", "[xtest_keyboard]")
{
    using gamescope::xtest_input::Key;
    for (bool swapped : {false, true}) {
        for (bool pressed : {false, true}) {
            auto request = Request(0, 0, swapped);
            request[4] = pressed ? 2 : 3;
            request[5] = KEY_F + 8;
            request[35] = 99; // unspecified padding in a core request
            auto key = Key(request, 132, swapped, true);
            REQUIRE(key);
            CHECK(key->scancode == KEY_F);
            CHECK(key->pressed == pressed);
            CHECK_FALSE(Key(request, 132, swapped, false));
            CHECK_FALSE(Key(request, 131, swapped, true));
            request[8] = 1;
            CHECK_FALSE(Key(request, 132, swapped, true));
            request[8] = 0; request[5] = 7;
            CHECK_FALSE(Key(request, 132, swapped, true));
        }
    }
    CHECK_FALSE(Key({}, 132, false, true));
    for (unsigned type : {4, 5, 6, 64}) {
        auto request = Request(); request[4] = type;
        CHECK_FALSE(Key(request, 132, false, true));
    }
}
TEST_CASE("XTEST mixed packets preserve keyboard transitions and discard physical markers", "[xtest_keyboard]")
{
    for (bool swapped : {false, true}) {
        auto press = Request(0, 0, swapped); press[4] = 2; press[5] = KEY_F + 8;
        auto release = press; release[4] = 3;
        auto move = Request(4, -3, swapped);
        std::vector<uint8_t> data(press.begin(), press.end());
        data.insert(data.end(), move.begin(), move.end());
        data.insert(data.end(), release.begin(), release.end());
        std::vector<std::pair<uint32_t, bool>> keys;
        auto sink = [&](uint32_t key, bool down) { keys.emplace_back(key, down); };
        Motions motions;
        REQUIRE(DispatchAll(1, data, 132, swapped, true, motions, sink));
        CHECK(keys == std::vector<std::pair<uint32_t, bool>>{{KEY_F, true}, {KEY_F, false}});
        REQUIRE(motions.size() == 1);
        CHECK(motions[0] == std::array<double, 2>{4, -3});
        REQUIRE(DispatchAll(0, data, 132, swapped, true, motions, sink));
        CHECK(keys.size() == 2);
        CHECK(motions.size() == 1);
        REQUIRE(DispatchAll(1, data, 132, swapped, false, motions, sink));
        CHECK(keys.size() == 2);
    }
}
TEST_CASE("Physical and XTEST F ownership survives either release order", "[xtest_keyboard]")
{
    using gamescope::xtest_input::KeySource;
    for (auto first : {KeySource::Wayland, KeySource::XTest}) {
        KeyboardFixture fixture;
        auto second = first == KeySource::Wayland ? KeySource::XTest : KeySource::Wayland;
        REQUIRE(fixture.state.Set(first, KEY_F, true));
        CHECK_FALSE(fixture.state.Set(first, KEY_F, true));
        CHECK_FALSE(fixture.state.Set(second, KEY_F, true));
        CHECK_FALSE(fixture.state.Set(first, KEY_F, false));
        REQUIRE(fixture.state.Set(second, KEY_F, false));
        CHECK(fixture.state.Empty());
        CHECK_FALSE(fixture.state.Set(first, KEY_F, false));
    }
}
TEST_CASE("XTEST cancellation releases only emulated ownership and permits a fresh press", "[xtest_keyboard]")
{
    using gamescope::xtest_input::KeySource;
    KeyboardFixture fixture;
    REQUIRE(fixture.state.Set(KeySource::Wayland, KEY_F, true));
    CHECK_FALSE(fixture.state.Set(KeySource::XTest, KEY_F, true));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_X, true));
    CHECK(fixture.state.ReleaseXTest() == std::vector<uint32_t>{KEY_X});
    CHECK_FALSE(fixture.state.Empty());
    CHECK_FALSE(fixture.state.Set(KeySource::XTest, KEY_F, false));
    REQUIRE(fixture.state.Set(KeySource::Wayland, KEY_F, false));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_F, true));
    CHECK(fixture.state.ReleaseAll() == std::vector<uint32_t>{KEY_F});
    CHECK(fixture.state.Empty());
    CHECK_FALSE(fixture.state.Set(KeySource::XTest, KEY_F, false));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_F, true));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_F, false));
}
TEST_CASE("Native modifier updates preserve emulated chords and shared modifiers", "[xtest_keyboard]")
{
    using gamescope::xtest_input::KeySource;
    KeyboardFixture fixture;
    auto control = fixture.Mask(XKB_MOD_NAME_CTRL), shift = fixture.Mask(XKB_MOD_NAME_SHIFT);
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_LEFTCTRL, true));
    CHECK_FALSE(fixture.state.Set(KeySource::XTest, KEY_LEFTCTRL, true));
    fixture.state.SetWaylandModifiers({.depressed = shift});
    CHECK(fixture.state.CombinedModifiers().depressed == (control | shift));
    CHECK_FALSE(fixture.state.Set(KeySource::Wayland, KEY_LEFTCTRL, true));
    fixture.state.SetWaylandModifiers({.depressed = control | shift});
    CHECK(fixture.state.ReleaseXTest().empty());
    CHECK(fixture.state.CombinedModifiers().depressed == (control | shift));
    fixture.state.ReleaseAll();
    CHECK(fixture.state.CombinedModifiers().depressed == 0);
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_LEFTCTRL, true));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_LEFTCTRL, false));
    CHECK(fixture.state.CombinedModifiers().depressed == 0);
}
TEST_CASE("Emulated lock keys toggle against host state and clear on cancellation", "[xtest_keyboard]")
{
    using gamescope::xtest_input::KeySource;
    KeyboardFixture fixture;
    auto caps = fixture.Mask(XKB_MOD_NAME_CAPS);
    fixture.state.SetWaylandModifiers({.locked = caps});
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_CAPSLOCK, true));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_CAPSLOCK, false));
    CHECK(fixture.state.CombinedModifiers().locked == 0);
    CHECK(fixture.state.ReleaseXTest().empty());
    CHECK(fixture.state.CombinedModifiers().locked == caps);
}
TEST_CASE("Emulated modifiers use the host layout without dropping held keys", "[xtest_keyboard]")
{
    using gamescope::xtest_input::KeySource;
    KeyboardFixture fixture("us,de");
    auto control = fixture.Mask(XKB_MOD_NAME_CTRL), altgr = fixture.Mask("Mod5");
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_LEFTCTRL, true));
    fixture.state.SetWaylandModifiers({.group = 1});
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_RIGHTALT, true));
    CHECK(fixture.state.CombinedModifiers().depressed == (control | altgr));
    CHECK(fixture.state.CombinedModifiers().group == 1);
    // Changing layout preserves the action of an already held modifier.
    fixture.state.SetWaylandModifiers({.group = 0});
    CHECK(fixture.state.CombinedModifiers().depressed == (control | altgr));
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_RIGHTALT, false));
    CHECK(fixture.state.CombinedModifiers().depressed == control);
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_RIGHTALT, true));
    CHECK(fixture.state.CombinedModifiers().depressed == (control | fixture.Mask("Mod1")));
    fixture.state.SetWaylandModifiers({.group = 1});
    fixture.state.ReleaseXTest();
    CHECK(fixture.state.CombinedModifiers().depressed == 0);
    CHECK(fixture.state.CombinedModifiers().group == 1);
    REQUIRE(fixture.state.Set(KeySource::XTest, KEY_RIGHTALT, true));
    CHECK(fixture.state.CombinedModifiers().depressed == altgr);
    fixture.state.ReleaseAll();
    CHECK(fixture.state.CombinedModifiers().group == 1);
    CHECK(fixture.state.CombinedModifiers().depressed == 0);
}
TEST_CASE("XTEST recording forwards immediate focused relative core motion", "[xtest_mouse]")
{
    auto request = Request();
    auto motion = Motion(request, 132, false, true);
    REQUIRE(motion.has_value());
    REQUIRE((*motion)[0] == 7);
    REQUIRE((*motion)[1] == -5);
    REQUIRE_FALSE(Motion(request, 132, false, false));
    REQUIRE_FALSE(Motion(request, 131, false, true));
    REQUIRE_FALSE(Motion(request, 0, false, true));
}
TEST_CASE("XTEST absolute positioning never accumulates as relative motion", "[xtest_mouse]")
{
    auto request = Request(400, 300);
    request[5] = 0;
    for (int i = 0; i < 3; ++i)
        REQUIRE_FALSE(Motion(request, 132, false, true));
    request[5] = 1;
    REQUIRE(Motion(request, 132, false, true).has_value());
}
TEST_CASE("XTEST recording excludes other input and delayed requests", "[xtest_mouse]")
{
    for (const unsigned type : {2, 3, 4, 5}) {
        auto request = Request(); request[4] = type;
        REQUIRE_FALSE(Motion(request, 132, false, true));
    }
    auto request = Request(); request[35] = 4;
    REQUIRE(Motion(request, 132, false, true).has_value()); // core padding is unspecified
    request = Request(); request[4] = 64; request[35] = 4;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    request = Request(); request[8] = 1;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    request = Request(); request[1] = 3;
    REQUIRE_FALSE(Motion(request, 132, false, true));
}
TEST_CASE("XTEST recording decodes signed coordinates in either sender byte order", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        auto request = Request(-32768, 32767, swapped);
        auto motion = Motion(request, 132, swapped, true);
        REQUIRE(motion.has_value());
        REQUIRE((*motion)[0] == -32768);
        REQUIRE((*motion)[1] == 32767);
        motion = Motion(Request(0, -1, swapped), 132, swapped, true);
        REQUIRE(motion.has_value());
        REQUIRE((*motion)[0] == 0);
        REQUIRE((*motion)[1] == -1);
    }
}
TEST_CASE("XTEST recording rejects malformed requests and stationary motion", "[xtest_mouse]")
{
    REQUIRE_FALSE(Motion({}, 132, false, true));
    auto request = Request();
    REQUIRE_FALSE(Motion(std::span(request).first(35), 132, false, true));
    request[2] = request[3] = 0;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    REQUIRE_FALSE(Motion(Request(0, 0), 132, false, true));
    request = Request(); request[5] = 2;
    REQUIRE_FALSE(Motion(request, 132, false, true));
}

TEST_CASE("XTEST RECORD batches skip variable XI requests without losing core motion", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        const auto request = Request(7, -5, swapped);
        std::vector<uint8_t> data(request.begin(), request.end());
        auto xi = Request(400, 300, swapped);
        xi[4] = 64; // XI DeviceMotionNotify
        uint16_t words = swapped ? 0x1100 : 17;
        std::memcpy(xi.data() + 2, &words, 2);
        data.insert(data.end(), xi.begin(), xi.end());
        data.resize(data.size() + 32); // appended DeviceValuator event
        data.insert(data.end(), request.begin(), request.end());
        Motions motions;
        REQUIRE(DispatchAll(1, data, 132, swapped, true, motions));
        REQUIRE(motions.size() == 2);
        REQUIRE(motions[0][0] + motions[1][0] == 14);
        REQUIRE(motions[0][1] + motions[1][1] == -10);
    }
}
TEST_CASE("XTEST RECORD framing handles big requests and rejects malformed data", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        auto request = Request(7, -5, swapped);
        std::vector<uint8_t> data(request.begin(), request.end());
        data[2] = data[3] = 0;
        uint32_t words = swapped ? 0x0a000000 : 10;
        data.insert(data.begin() + 4, 4, 0);
        std::memcpy(data.data() + 4, &words, 4);
        data.insert(data.end(), request.begin(), request.end());
        Motions motions;
        REQUIRE(DispatchAll(1, data, 132, swapped, true, motions));
        REQUIRE(motions.size() == 1); // unsupported BIG FakeInput skipped
    }
    Motions motions;
    const auto request = Request();
    REQUIRE_FALSE(DispatchAll(1, std::span(request).first(3), 132, false, true, motions));
    REQUIRE_FALSE(DispatchAll(1, std::span(request).first(35), 132, false, true, motions));
    REQUIRE(motions.empty());
}

TEST_CASE("XTEST RECORD markers add no input and EndOfData fails", "[xtest_mouse]")
{
    Motions motions;
    const auto request = Request();
    REQUIRE(DispatchAll(0, request, 132, false, true, motions));
    REQUIRE(motions.empty());
    REQUIRE(DispatchAll(1, request, 132, false, true, motions));
    REQUIRE(motions.size() == 1);
    REQUIRE_FALSE(DispatchAll(5, {}, 132, false, true, motions));
}

TEST_CASE("XTEST movement stays inside its F hold across bounded continuations", "[xtest_keyboard][xtest_regression]")
{
    using namespace gamescope::xtest_input;
    for (bool swapped : {false, true}) {
        auto press = Request(0, 0, swapped); press[4] = 2; press[5] = KEY_F + 8;
        auto release = press; release[4] = 3;
        const auto move = Request(12, -4, swapped);
        std::vector<uint8_t> bytes;
        std::vector<int> expected, delivered;
        for (unsigned i = 0; i < 512; ++i) {
            for (const auto &request : {press, move, release}) bytes.insert(bytes.end(), request.begin(), request.end());
            expected.insert(expected.end(), {1, 2, 3});
        }
        std::span<const uint8_t> data(bytes);
        bool held = false;
        unsigned passes = 0;
        while (!data.empty()) {
            DispatchBudget budget;
            const auto before = delivered.size();
            REQUIRE(DispatchRequests(data, 132, swapped, true, budget,
                [&](uint32_t key, bool down) {
                    CHECK(key == KEY_F); held = down; delivered.push_back(down ? 1 : 3);
                }, [&](double x, double y) {
                    CHECK(held); CHECK(x == 12); CHECK(y == -4); delivered.push_back(2);
                }));
            CHECK(delivered.size() - before <= DispatchBudget::kWorkLimit);
            REQUIRE(++passes < 1000);
        }
        CHECK(passes > 1);
        CHECK_FALSE(held);
        CHECK(delivered == expected);
    }
}

TEST_CASE("XTEST stops at the sender's exact displacement", "[xtest_mouse][xtest_regression]")
{
    for (unsigned count : {1u, 60u}) {
        Motions motions;
        for (unsigned ms = 0; ms < count * 16 + 60; ++ms) {
            if (ms % 16 == 0 && ms / 16 < count)
                REQUIRE(DispatchAll(1, Request(1, 0), 132, false, true, motions));
            else
                REQUIRE(DispatchAll(1, {}, 132, false, true, motions));
        }
        REQUIRE(motions.size() == count);
        for (const auto &motion : motions) CHECK(motion == std::array<double, 2>{1, 0});
    }
}

TEST_CASE("XTEST focus loss discards a partially consumed reply", "[xtest_mouse][xtest_regression]")
{
    using namespace gamescope::xtest_input;
    const auto request = Request();
    std::vector<uint8_t> bytes;
    for (unsigned i = 0; i < 1024; ++i) bytes.insert(bytes.end(), request.begin(), request.end());
    std::span<const uint8_t> data(bytes);
    unsigned received = 0;
    auto motion = [&](double, double) { ++received; };
    DispatchBudget first;
    REQUIRE(DispatchRequests(data, 132, false, true, first, {}, motion));
    REQUIRE(!data.empty());
    const auto before = received;
    while (!data.empty()) {
        DispatchBudget next;
        REQUIRE(DispatchRequests(data, 132, false, false, next, {}, motion));
    }
    CHECK(received == before);
}
