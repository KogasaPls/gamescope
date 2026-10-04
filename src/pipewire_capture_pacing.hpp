#pragma once

#include <cstdint>

namespace gamescope::pipewire_capture
{
inline uint64_t Interval(uint32_t numerator, uint32_t denominator)
{
    if (!numerator || !denominator)
        return 0;
    return (1'000'000'000ull * denominator + numerator - 1) / numerator;
}

struct Pacer
{
    uint64_t interval = 0;
    uint64_t next = 0;

    bool Due(uint64_t now, uint64_t negotiatedInterval, uint64_t refreshPeriod)
    {
        if (interval != negotiatedInterval)
        {
            interval = negotiatedInterval;
            next = now;
        }
        if (!interval)
            return true;

        // Vblanks are sampled with jitter around a phase fixed by the first
        // capture, so accept the vblank nearest the deadline rather than
        // skipping it for arriving a few microseconds early.
        const uint64_t slack = (refreshPeriod ? refreshPeriod : interval) / 2;
        if (now + slack < next)
            return false;

        // Advance by whole intervals to keep the long-run rate exact. After a
        // stall, schedule from now rather than trying to produce missed frames.
        next = now >= next + interval ? now + interval : next + interval;
        return true;
    }
};
}
