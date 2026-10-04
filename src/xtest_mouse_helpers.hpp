#pragma once
#include <algorithm>
#include <array>
#include <cmath>
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
// Steam Input sends discrete integer XTEST motions at a lower rate than a
// high-refresh nested display while a held controller chord polls every frame.
// Subdivide bursts across 3 ms ticks and sustain integer steps over a short
// hold window so Wine/Proton and the game see continuous mouse movement.
class MotionPacer
{
public:
    static constexpr uint64_t kStepIntervalNs = 3'000'000;
    static constexpr uint64_t kEarlyWakeSlackNs = 500'000;
    static constexpr uint64_t kHoldDurationNs = 48'000'000;
    static constexpr unsigned kSmoothSteps = 4;
    static constexpr double kMaxDebt = 4.0;

    void Reset()
    {
        *this = {};
    }

    int PollTimeoutMs(uint64_t nowNs) const
    {
        if (!m_active)
            return -1;
        if (nowNs >= m_nextStepNs)
            return 0;
        return int((m_nextStepNs - nowNs + 999'999) / 1'000'000);
    }

    std::optional<std::array<double, 2>> Step(const MotionBatch &batch, uint64_t nowNs, bool focused)
    {
        if (!focused)
        {
            Reset();
            return std::nullopt;
        }

        double dx = 0.0, dy = 0.0;
        for (const auto &motion : batch.Motions())
        {
            dx += motion[0];
            dy += motion[1];
        }

        if (dx != 0.0 || dy != 0.0)
        {
            if (!m_active)
            {
                m_active = true;
                m_nextStepNs = nowNs;
            }
            m_lastInputNs = nowNs;

            m_x.Ingest(dx);
            m_y.Ingest(dy);
            if (const double maxVel = std::max(std::abs(m_x.vel), std::abs(m_y.vel)); maxVel > 0.0)
            {
                m_x.dir = m_x.vel / maxVel;
                m_y.dir = m_y.vel / maxVel;
            }
            m_smoothStepsRemaining = kSmoothSteps;
        }

        if (!m_active)
            return std::nullopt;

        if (nowNs > m_lastInputNs && nowNs - m_lastInputNs > kHoldDurationNs &&
            std::abs(m_x.pending) < 1.0 && std::abs(m_y.pending) < 1.0)
        {
            Reset();
            return std::nullopt;
        }

        if (nowNs + kEarlyWakeSlackNs < m_nextStepNs)
            return std::nullopt;

        m_nextStepNs = nowNs + kStepIntervalNs;

        if (std::abs(m_x.pending) >= 1.0 || std::abs(m_y.pending) >= 1.0)
        {
            const unsigned steps = std::max(1u, m_smoothStepsRemaining);
            double stepX = std::trunc(m_x.pending / double(steps));
            double stepY = std::trunc(m_y.pending / double(steps));
            if (stepX == 0.0 && stepY == 0.0)
            {
                if (std::abs(m_x.pending) >= std::abs(m_y.pending))
                    stepX = m_x.pending > 0.0 ? 1.0 : -1.0;
                else
                    stepY = m_y.pending > 0.0 ? 1.0 : -1.0;
            }
            m_x.pending -= stepX;
            m_y.pending -= stepY;
            if (m_smoothStepsRemaining > 1)
                --m_smoothStepsRemaining;
            return std::array<double, 2>{stepX, stepY};
        }

        return std::array<double, 2>{m_x.KeepAliveStep(), m_y.KeepAliveStep()};
    }

private:
    struct Axis
    {
        double pending = 0.0;
        double debt = 0.0;
        double vel = 0.0;
        double dir = 0.0;
        double keepAliveAccum = 0.0;

        void Ingest(double d)
        {
            if (d != 0.0 && (d * pending < 0.0 || d * vel < 0.0))
                pending = debt = keepAliveAccum = vel = 0.0;
            vel = vel * 0.5 + d;
            if (std::abs(vel) < 0.5)
                vel = keepAliveAccum = 0.0;
            double net = d;
            if (d * debt > 0.0)
                net = (d * (d - debt) > 0.0) ? (d - debt) : 0.0;
            debt = 0.0;
            pending += net;
        }

        double KeepAliveStep()
        {
            keepAliveAccum += dir;
            const double step = std::round(keepAliveAccum);
            keepAliveAccum -= step;
            debt = std::clamp(debt + step, -kMaxDebt, kMaxDebt);
            return step;
        }
    };

    Axis m_x{};
    Axis m_y{};
    uint64_t m_lastInputNs = 0;
    uint64_t m_nextStepNs = 0;
    unsigned m_smoothStepsRemaining = 0;
    bool m_active = false;
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
