#pragma once

#include <cstdint>
#include <optional>

namespace gamescope
{
// With adaptive sync a compositor pass counts as a vblank as soon as the next
// flip can be queued, which is before the previous one is presented and can
// happen several times per refresh. FIFO still means one latch per refresh, so
// a window's FIFO commit waits until one refresh cycle after the window's
// previous latch. Returns the time to wake at when the commit has to wait.
inline constexpr std::optional<uint64_t> VRRFifoLatchWait( uint64_t ulNow, uint64_t ulLastLatch, uint64_t ulRefreshCycle, uint64_t ulFudge )
{
	const uint64_t ulSchedule = ulLastLatch + ulRefreshCycle;
	if ( ulNow + ulFudge >= ulSchedule )
		return std::nullopt;

	return ulSchedule;
}

// The time the window's next FIFO latch is counted from. A latch within the
// fudge of its slot keeps the slot's time, so wakeup latency does not lower
// the rate. A later latch counts from its own time, so the next commit cannot
// follow it early.
inline constexpr uint64_t VRRFifoLatchTime( uint64_t ulNow, uint64_t ulLastLatch, uint64_t ulRefreshCycle, uint64_t ulFudge )
{
	const uint64_t ulSchedule = ulLastLatch + ulRefreshCycle;
	if ( ulNow + ulFudge >= ulSchedule && ulNow <= ulSchedule + ulFudge )
		return ulSchedule;

	return ulNow;
}
}
