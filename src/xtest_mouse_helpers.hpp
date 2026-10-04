#pragma once
#include <array>
#include <cstring>
#include <optional>
#include <span>
#include <cstdint>
namespace gamescope::xtest_mouse {
inline constexpr size_t kRequestSize = 36;
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
// Forward at most kLimit motions per drain and sum any excess into the last
// one, so a flood neither loses displacement nor blocks the drain.
struct MotionBatch
{
    static constexpr size_t kLimit = 256;
    std::array<std::array<double, 2>, kLimit> motions{};
    size_t count = 0;

    void Add(double dx, double dy)
    {
        if (count < kLimit)
            motions[count++] = {dx, dy};
        else
        {
            motions[kLimit - 1][0] += dx;
            motions[kLimit - 1][1] += dy;
        }
    }
    std::span<const std::array<double, 2>> Motions() const { return std::span(motions).first(count); }
};
// RECORD may batch fixed-size core and variable-length XI FakeInput requests.
// Request lengths and payloads retain the recorded client's byte order.
inline bool DispatchRequests(std::span<const uint8_t> data, uint8_t opcode,
    bool swapped, bool focused, MotionBatch &batch)
{
    while (!data.empty())
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
        if (const auto decoded = Motion(data.first(bytes), opcode, swapped, focused))
            batch.Add((*decoded)[0], (*decoded)[1]);
        data = data.subspan(bytes);
    }
    return true;
}
// False means the recording stream is unusable. Every reply must be drained:
// leaving xcb-buffered replies for a later socket wake could strand pending
// requests when the socket has already been drained.
inline bool DispatchRecordData(uint8_t category, std::span<const uint8_t> data,
    uint8_t opcode, bool swapped, bool focused, MotionBatch &batch)
{
    if (category == 0) // FromServer motion flush marker, never input
        return true;
    if (category != 1) return false; // includes EndOfData
    return DispatchRequests(data, opcode, swapped, focused, batch);
}

}
