#include <catch2/catch_test_macros.hpp>

#include "vrr_fifo_latch.hpp"

namespace
{
using gamescope::VRRFifoLatchTime;
using gamescope::VRRFifoLatchWait;

constexpr uint64_t k_ulCycle = 4'166'667; // 240 Hz
constexpr uint64_t k_ulFudge = 200'000;
constexpr uint64_t k_ulLatch = 1'000'000'000;
}

TEST_CASE( "A window's first FIFO commit latches at once", "[vrr_fifo_latch]" )
{
	REQUIRE_FALSE( VRRFifoLatchWait( k_ulLatch, 0, k_ulCycle, k_ulFudge ) );
}

TEST_CASE( "A FIFO commit inside the refresh cycle of the previous latch waits for the cycle's end", "[vrr_fifo_latch]" )
{
	REQUIRE( VRRFifoLatchWait( k_ulLatch, k_ulLatch, k_ulCycle, k_ulFudge ) == k_ulLatch + k_ulCycle );
	REQUIRE( VRRFifoLatchWait( k_ulLatch + 2'500'000, k_ulLatch, k_ulCycle, k_ulFudge ) == k_ulLatch + k_ulCycle );
	REQUIRE( VRRFifoLatchWait( k_ulLatch + k_ulCycle - k_ulFudge - 1, k_ulLatch, k_ulCycle, k_ulFudge ) == k_ulLatch + k_ulCycle );
}

TEST_CASE( "A FIFO commit latches from the fudge before the cycle's end onwards", "[vrr_fifo_latch]" )
{
	REQUIRE_FALSE( VRRFifoLatchWait( k_ulLatch + k_ulCycle - k_ulFudge, k_ulLatch, k_ulCycle, k_ulFudge ) );
	REQUIRE_FALSE( VRRFifoLatchWait( k_ulLatch + k_ulCycle, k_ulLatch, k_ulCycle, k_ulFudge ) );
}

TEST_CASE( "A client slower than the refresh rate is never held", "[vrr_fifo_latch]" )
{
	REQUIRE_FALSE( VRRFifoLatchWait( k_ulLatch + 3 * k_ulCycle + 123, k_ulLatch, k_ulCycle, k_ulFudge ) );
}

TEST_CASE( "A window's first FIFO latch counts from its own time", "[vrr_fifo_latch]" )
{
	REQUIRE( VRRFifoLatchTime( k_ulLatch, 0, k_ulCycle, k_ulFudge ) == k_ulLatch );
}

TEST_CASE( "A FIFO latch within the fudge of its slot keeps the slot's time", "[vrr_fifo_latch]" )
{
	const uint64_t ulSlot = k_ulLatch + k_ulCycle;
	REQUIRE( VRRFifoLatchTime( ulSlot - k_ulFudge, k_ulLatch, k_ulCycle, k_ulFudge ) == ulSlot );
	REQUIRE( VRRFifoLatchTime( ulSlot, k_ulLatch, k_ulCycle, k_ulFudge ) == ulSlot );
	REQUIRE( VRRFifoLatchTime( ulSlot + k_ulFudge, k_ulLatch, k_ulCycle, k_ulFudge ) == ulSlot );
}

TEST_CASE( "A FIFO latch later than the fudge counts from its own time", "[vrr_fifo_latch]" )
{
	const uint64_t ulLate = k_ulLatch + k_ulCycle + k_ulFudge + 1;
	REQUIRE( VRRFifoLatchTime( ulLate, k_ulLatch, k_ulCycle, k_ulFudge ) == ulLate );
	REQUIRE( VRRFifoLatchTime( k_ulLatch + 3 * k_ulCycle, k_ulLatch, k_ulCycle, k_ulFudge ) == k_ulLatch + 3 * k_ulCycle );
}

TEST_CASE( "A queued FIFO commit does not follow a late latch early", "[vrr_fifo_latch]" )
{
	// 240 Hz: a commit latches 7.5 ms after the previous one while the next is already queued.
	const uint64_t ulLate = k_ulLatch + 7'500'000;
	REQUIRE_FALSE( VRRFifoLatchWait( ulLate, k_ulLatch, k_ulCycle, k_ulFudge ) );

	const uint64_t ulLast = VRRFifoLatchTime( ulLate, k_ulLatch, k_ulCycle, k_ulFudge );
	REQUIRE( VRRFifoLatchWait( ulLate + 833'000, ulLast, k_ulCycle, k_ulFudge ) == ulLate + k_ulCycle );
}

TEST_CASE( "Wakeup latency inside the fudge does not lower the FIFO latch rate", "[vrr_fifo_latch]" )
{
	uint64_t ulLast = k_ulLatch;
	for ( int i = 0; i < 240; i++ )
	{
		const std::optional<uint64_t> oWake = VRRFifoLatchWait( ulLast, ulLast, k_ulCycle, k_ulFudge );
		REQUIRE( oWake );

		const uint64_t ulPass = *oWake + 150'000;
		REQUIRE_FALSE( VRRFifoLatchWait( ulPass, ulLast, k_ulCycle, k_ulFudge ) );
		ulLast = VRRFifoLatchTime( ulPass, ulLast, k_ulCycle, k_ulFudge );
	}

	REQUIRE( ulLast == k_ulLatch + 240 * k_ulCycle );
}
