#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <optional>
#include <span>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <xkbcommon/xkbcommon.h>
namespace gamescope::xtest_input {
inline constexpr size_t kRequestSize = 36;
using KeySink = std::function<void(uint32_t, bool)>;
using MotionSink = std::function<void(double, double)>;
struct KeyEvent { uint32_t scancode; bool pressed; };

inline std::optional<KeyEvent> Key(std::span<const uint8_t> request,
    uint8_t opcode, bool swapped, bool focused)
{
    if (!focused || !opcode || request.size() != kRequestSize ||
        request[0] != opcode || request[1] != 2 ||
        (request[4] != 2 && request[4] != 3) || request[5] < 8 ||
        request[8] || request[9] || request[10] || request[11])
        return std::nullopt;
    uint16_t words;
    std::memcpy(&words, request.data() + 2, 2);
    if (swapped) words = uint16_t((words >> 8) | (words << 8));
    if (words != kRequestSize / 4)
        return std::nullopt;
    // Host Xwayland's core keycodes are evdev scancodes plus eight, as
    // are the keycodes in the host's wl_keyboard keymap. Ignore core padding.
    return KeyEvent{uint32_t(request[5] - 8), request[4] == 2};
}

enum class KeySource : uint8_t { Wayland = 1, XTest = 2 };
struct Modifiers {
    uint32_t depressed = 0, latched = 0, locked = 0, group = 0;
    bool operator==(const Modifiers &) const = default;
};

// One ownership mask per key. A key reaches the seat on its first press
// and last release, even when physical and emulated keyboards overlap.
class KeyboardState
{
public:
    bool SetKeymap(xkb_keymap *keymap)
    {
        m_xkb.reset(xkb_state_new(keymap));
        ResetXTestModifiers();
        return bool(m_xkb);
    }
    bool Set(KeySource source, uint32_t key, bool pressed)
    {
        if (source == KeySource::XTest && !m_xkb)
            return false;
        auto it = m_held.find(key);
        const uint8_t before = it == m_held.end() ? 0 : it->second;
        const uint8_t bit = uint8_t(source);
        const uint8_t after = pressed ? before | bit : before & ~bit;
        if (before == after)
            return false;
        if (after) m_held[key] = after;
        else m_held.erase(it);
        if (source == KeySource::XTest)
            xkb_state_update_key(m_xkb.get(), key + 8, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
        return bool(before) != bool(after);
    }
    bool Empty() const { return m_held.empty(); }
    bool Held(KeySource source, uint32_t key) const
    {
        auto it = m_held.find(key);
        return it != m_held.end() && (it->second & uint8_t(source));
    }
    void SetWaylandModifiers(Modifiers modifiers)
    {
        if (m_xkb && m_wayland.group != modifiers.group)
            // Change layout without reconstructing (and losing) held key actions.
            xkb_state_update_latched_locked(m_xkb.get(), 0, 0, true, 0, 0, 0, true, modifiers.group);
        m_wayland = modifiers;
    }
    uint32_t WaylandModifiers() const { return m_wayland.depressed | m_wayland.latched | m_wayland.locked; }
    Modifiers CombinedModifiers() const
    {
        if (!m_xkb) return m_wayland;
        return {
            m_wayland.depressed | xkb_state_serialize_mods(m_xkb.get(), XKB_STATE_MODS_DEPRESSED),
            m_wayland.latched | xkb_state_serialize_mods(m_xkb.get(), XKB_STATE_MODS_LATCHED),
            // Emulated lock keys toggle relative to the host's lock state.
            m_wayland.locked ^ xkb_state_serialize_mods(m_xkb.get(), XKB_STATE_MODS_LOCKED),
            xkb_state_serialize_layout(m_xkb.get(), XKB_STATE_LAYOUT_EFFECTIVE),
        };
    }
    std::vector<uint32_t> ReleaseXTest()
    {
        std::vector<uint32_t> released;
        for (auto it = m_held.begin(); it != m_held.end();)
        {
            if ((it->second & uint8_t(KeySource::XTest)) && m_xkb)
                xkb_state_update_key(m_xkb.get(), it->first + 8, XKB_KEY_UP);
            it->second &= ~uint8_t(KeySource::XTest);
            if (!it->second) { released.push_back(it->first); it = m_held.erase(it); }
            else ++it;
        }
        ResetXTestModifiers();
        return released;
    }
    std::vector<uint32_t> ReleaseAll()
    {
        std::vector<uint32_t> released;
        for (const auto &[key, owners] : m_held) {
            released.push_back(key);
            if ((owners & uint8_t(KeySource::XTest)) && m_xkb)
                xkb_state_update_key(m_xkb.get(), key + 8, XKB_KEY_UP);
        }
        m_held.clear();
        m_wayland.depressed = m_wayland.latched = 0;
        ResetXTestModifiers();
        return released;
    }
private:
    void ResetXTestModifiers()
    {
        // Called after releasing synthetic keys: only latch/lock state remains.
        if (m_xkb)
            xkb_state_update_latched_locked(m_xkb.get(), UINT32_MAX, 0, true, 0,
                UINT32_MAX, 0, true, m_wayland.group);
    }
    std::unordered_map<uint32_t, uint8_t> m_held;
    Modifiers m_wayland;
    std::unique_ptr<xkb_state, decltype(&xkb_state_unref)> m_xkb{nullptr, xkb_state_unref};
};
// XTEST FakeInput wire layout. RECORD preserves the sender's byte order;
// client_swapped tells us whether it differs from this connection's order.
inline std::optional<std::array<double, 2>> Motion(std::span<const uint8_t> request,
    uint8_t opcode, bool swapped, bool focused)
{
    if (!focused || !opcode || request.size() != kRequestSize ||
        request[0] != opcode || request[1] != 2 || request[4] != 6 ||
        request[5] != 1)
        return std::nullopt; // only core relative MotionNotify, never absolute/XI input; byte 35 is padding
    auto word = [&](size_t offset) {
        uint16_t value;
        std::memcpy(&value, request.data() + offset, sizeof(value));
        return swapped ? uint16_t((value >> 8) | (value << 8)) : value;
    };
    // Delay requests are observed before execution; forwarding them early is
    // incorrect. Steam's immediate joystick events use CurrentTime (zero).
    if (word(2) != kRequestSize / 4 || request[8] || request[9] || request[10] || request[11])
        return std::nullopt;
    const auto signedWord = [&](size_t offset) {
        const uint16_t value = word(offset);
        return value < 0x8000 ? int(value) : int(value) - 0x10000;
    };
    const std::array<double, 2> delta{double(signedWord(24)), double(signedWord(26))};
    if (delta[0] == 0 && delta[1] == 0)
        return std::nullopt;
    return delta;
}
// A dispatch must yield to Wayland focus/shutdown even if a sender keeps
// filling XCB's buffers. Bound cheap work and slow input callbacks separately.
class DispatchBudget
{
public:
    static constexpr size_t kWorkLimit = 256;
    bool Take()
    {
        if (!m_remaining || std::chrono::steady_clock::now() >= m_deadline)
            return false;
        --m_remaining;
        return true;
    }
private:
    size_t m_remaining = kWorkLimit;
    std::chrono::steady_clock::time_point m_deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
};
// RECORD may batch fixed-size core and variable-length XI FakeInput requests.
// Request lengths and payloads retain the recorded client's byte order.
// Consume only complete requests. The owner retains the unconsumed span
// across dispatches, including when one RECORD reply exceeds the budget.
inline bool DispatchRequests(std::span<const uint8_t> &data, uint8_t opcode,
    bool swapped, bool focused, DispatchBudget &budget,
    const KeySink &onKey = {}, const MotionSink &onMotion = {})
{
    while (!data.empty() && budget.Take())
    {
        if (data.size() < 4)
            return false;
        uint16_t shortLength;
        std::memcpy(&shortLength, data.data() + 2, 2);
        if (swapped) shortLength = uint16_t((shortLength >> 8) | (shortLength << 8));
        uint32_t words = shortLength;
        if (!words) // BIG-REQUESTS use an extra 32-bit length field
        {
            if (data.size() < 8) return false;
            std::memcpy(&words, data.data() + 4, 4);
            if (swapped)
                words = ((words & 0xff) << 24) | ((words & 0xff00) << 8) |
                        ((words & 0xff0000) >> 8) | (words >> 24);
            if (words < 2) return false;
        }
        if (words > data.size() / 4)
            return false;
        const size_t bytes = size_t(words) * 4;
        if (const auto decoded = Motion(data.first(bytes), opcode, swapped, focused); decoded && onMotion)
            onMotion((*decoded)[0], (*decoded)[1]);
        else if (const auto key = Key(data.first(bytes), opcode, swapped, focused); key && onKey)
            onKey(key->scancode, key->pressed);
        data = data.subspan(bytes);
    }
    return true;
}
// False means the recording stream is unusable. A nonempty remainder needs
// another dispatch even if XCB already read it out of the socket.
inline bool DispatchRecordData(uint8_t category, std::span<const uint8_t> &data,
    uint8_t opcode, bool swapped, bool focused, DispatchBudget &budget,
    const KeySink &onKey = {}, const MotionSink &onMotion = {})
{
    if (category == 0) // FromServer device flush markers, never input
    {
        data = {};
        return true;
    }
    if (category != 1) return false; // includes EndOfData
    return DispatchRequests(data, opcode, swapped, focused, budget, onKey, onMotion);
}

}
