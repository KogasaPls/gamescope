#include <catch2/catch_test_macros.hpp>
#include "pipewire_capture_pacing.hpp"

#include <random>
#include <vector>

using gamescope::pipewire_capture::Pacer;
using gamescope::pipewire_capture::Interval;

namespace
{
constexpr uint64_t RefreshPeriod(uint64_t hz)
{
    return 1'000'000'000 / hz;
}

std::vector<uint64_t> CapturedTicks(uint64_t hz, uint64_t interval, uint64_t ticks, int64_t jitter, uint64_t refreshPeriod)
{
    std::mt19937_64 rng(hz * 7919 + ticks);
    std::uniform_int_distribution<int64_t> offset(-jitter, jitter);
    Pacer pacer;
    std::vector<uint64_t> captured;
    for (uint64_t tick = 0; tick < ticks; ++tick)
    {
        const uint64_t now = 1'000'000'000 + tick * 1'000'000'000 / hz + offset(rng);
        if (pacer.Due(now, interval, refreshPeriod))
            captured.push_back(tick);
    }
    return captured;
}

void RequireUniformGaps(const std::vector<uint64_t> &captured, uint64_t gap)
{
    REQUIRE(captured.size() > 1);
    REQUIRE(captured.front() == 0);
    for (size_t i = 1; i < captured.size(); ++i)
        REQUIRE(captured[i] - captured[i - 1] == gap);
}
}

TEST_CASE("Capture honors a 60 FPS consumer on a 240 Hz output", "[pipewire_capture]")
{
    Pacer pacer;
    unsigned captures = 0;
    for (uint64_t tick = 0; tick < 240; ++tick)
        captures += pacer.Due(tick * 1'000'000'000 / 240, Interval(60, 1), RefreshPeriod(240));
    REQUIRE(captures == 60);
}

TEST_CASE("Divisor rates capture every Nth vblank despite timestamp jitter", "[pipewire_capture]")
{
    for (int64_t jitter : { 0, 50'000 })
    {
        RequireUniformGaps(CapturedTicks(240, Interval(60, 1), 240 * 10, jitter, RefreshPeriod(240)), 4);
        RequireUniformGaps(CapturedTicks(240, Interval(240, 1), 240 * 10, jitter, RefreshPeriod(240)), 1);
        RequireUniformGaps(CapturedTicks(240, Interval(240, 1), 240 * 10, jitter, 0), 1);
        RequireUniformGaps(CapturedTicks(144, Interval(72, 1), 144 * 10, jitter, RefreshPeriod(144)), 2);
    }
}

TEST_CASE("Non-divisor rates keep the negotiated long-run rate", "[pipewire_capture]")
{
    for (int64_t jitter : { 0, 50'000 })
    {
        const auto captured = CapturedTicks(144, Interval(50, 1), 144 * 10, jitter, RefreshPeriod(144));
        REQUIRE(captured.size() >= 499);
        REQUIRE(captured.size() <= 501);
        for (size_t i = 1; i < captured.size(); ++i)
        {
            REQUIRE(captured[i] - captured[i - 1] >= 2);
            REQUIRE(captured[i] - captured[i - 1] <= 3);
        }
        const auto unknownRefresh = CapturedTicks(240, Interval(60, 1), 240 * 10, jitter, 0);
        REQUIRE(unknownRefresh.size() >= 599);
        REQUIRE(unknownRefresh.size() <= 601);
    }
}

TEST_CASE("Capture handles fractional rates and lower output refresh", "[pipewire_capture]")
{
    Pacer fractional;
    unsigned captures = 0;
    for (uint64_t tick = 0; tick < 240 * 10; ++tick)
        captures += fractional.Due(tick * 1'000'000'000 / 240, Interval(60'000, 1001), RefreshPeriod(240));
    REQUIRE(captures == 600);
    Pacer slow;
    for (uint64_t tick = 0; tick < 30; ++tick)
        REQUIRE(slow.Due(tick * 1'000'000'000 / 30, Interval(60, 1), RefreshPeriod(30)));
}

TEST_CASE("Stalls do not cause catch-up capture bursts", "[pipewire_capture]")
{
    Pacer pacer;
    const auto interval = Interval(60, 1);
    const auto refresh = RefreshPeriod(240);
    REQUIRE(pacer.Due(0, interval, refresh));
    REQUIRE(pacer.Due(10'000'000'000, interval, refresh));
    REQUIRE_FALSE(pacer.Due(10'001'000'000, interval, refresh));
    REQUIRE_FALSE(pacer.Due(10'004'166'666, interval, refresh));
    REQUIRE_FALSE(pacer.Due(10'012'500'000, interval, refresh));
    REQUIRE(pacer.Due(10'016'666'666, interval, refresh));
}

TEST_CASE("Negotiation changes reset pacing; absent rates stay unrestricted", "[pipewire_capture]")
{
    Pacer pacer;
    REQUIRE(pacer.Due(0, Interval(30, 1), 0));
    REQUIRE_FALSE(pacer.Due(1, Interval(30, 1), 0));
    REQUIRE(pacer.Due(2, Interval(60, 1), 0));
    REQUIRE(pacer.Due(3, 0, 0));
    REQUIRE(pacer.Due(4, 0, 0));
    REQUIRE(Interval(0, 1) == 0);
    REQUIRE(Interval(60, 0) == 0);
}
