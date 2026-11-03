#include <catch2/catch_test_macros.hpp>

#include "ordered_shutdown_helpers.hpp"

namespace
{
using gamescope::ordered_shutdown::GraceNanosFromMs;
using gamescope::ordered_shutdown::k_ulChildPollNanos;
using gamescope::ordered_shutdown::OnChildrenChecked;
using gamescope::ordered_shutdown::OnTerminatingSignal;
using gamescope::ordered_shutdown::Phase;
using gamescope::ordered_shutdown::PollTimeoutMs;
using gamescope::ordered_shutdown::State;
using gamescope::ordered_shutdown::Step;

constexpr uint64_t k_ulMs = 1'000'000ul;
constexpr uint64_t k_ulGrace = 10'000 * k_ulMs;
constexpr uint64_t k_ulStart = 5'000 * k_ulMs;
}

TEST_CASE( "A grace period in milliseconds becomes nanoseconds", "[ordered_shutdown]" )
{
	REQUIRE( GraceNanosFromMs( 10'000 ) == k_ulGrace );
	REQUIRE( GraceNanosFromMs( 0 ) == 0 );
	REQUIRE( GraceNanosFromMs( -5 ) == 0 );
}

TEST_CASE( "Without children a terminating signal shuts down at once", "[ordered_shutdown]" )
{
	State state;
	REQUIRE( OnTerminatingSignal( state, false, k_ulStart, k_ulGrace ) == Step::Shutdown );
	REQUIRE( state.ePhase == Phase::Done );
	REQUIRE( PollTimeoutMs( state, k_ulStart ) == -1 );
}

TEST_CASE( "Without a grace period a terminating signal shuts down at once", "[ordered_shutdown]" )
{
	State state;
	REQUIRE( OnTerminatingSignal( state, true, k_ulStart, 0 ) == Step::Shutdown );
	REQUIRE( state.ePhase == Phase::Done );
}

TEST_CASE( "With children the first signal asks them to exit and waits", "[ordered_shutdown]" )
{
	State state;
	REQUIRE( OnTerminatingSignal( state, true, k_ulStart, k_ulGrace ) == Step::SignalChildren );
	REQUIRE( state.ePhase == Phase::WaitingForChildren );
	REQUIRE( state.ulDeadlineNanos == k_ulStart + k_ulGrace );

	REQUIRE( OnChildrenChecked( state, true, k_ulStart + 1'000 * k_ulMs ) == Step::Wait );
	REQUIRE( state.ePhase == Phase::WaitingForChildren );
}

TEST_CASE( "Shutdown follows once the children have exited", "[ordered_shutdown]" )
{
	State state;
	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );
	REQUIRE( OnChildrenChecked( state, false, k_ulStart + 300 * k_ulMs ) == Step::Shutdown );
	REQUIRE( state.ePhase == Phase::Done );
}

TEST_CASE( "Children still running at the deadline are killed", "[ordered_shutdown]" )
{
	State state;
	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );
	REQUIRE( OnChildrenChecked( state, true, k_ulStart + k_ulGrace - 1 ) == Step::Wait );
	REQUIRE( OnChildrenChecked( state, true, k_ulStart + k_ulGrace ) == Step::GraceExpired );
	REQUIRE( state.ePhase == Phase::Done );
}

TEST_CASE( "A second signal while waiting forces the shutdown", "[ordered_shutdown]" )
{
	State state;
	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );
	REQUIRE( OnTerminatingSignal( state, true, k_ulStart + 100 * k_ulMs, k_ulGrace ) == Step::ForceShutdown );
	REQUIRE( state.ePhase == Phase::Done );
}

TEST_CASE( "Nothing more happens once the shutdown is decided", "[ordered_shutdown]" )
{
	State state;
	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );
	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );

	REQUIRE( OnTerminatingSignal( state, true, k_ulStart, k_ulGrace ) == Step::Wait );
	REQUIRE( OnChildrenChecked( state, true, k_ulStart + 2 * k_ulGrace ) == Step::Wait );
	REQUIRE( OnChildrenChecked( state, false, k_ulStart + 2 * k_ulGrace ) == Step::Wait );
}

TEST_CASE( "Checks for children never sleep past the next poll or the deadline", "[ordered_shutdown]" )
{
	State state;
	REQUIRE( PollTimeoutMs( state, k_ulStart ) == -1 );

	OnTerminatingSignal( state, true, k_ulStart, k_ulGrace );
	REQUIRE( PollTimeoutMs( state, k_ulStart ) == int( k_ulChildPollNanos / k_ulMs ) );
	REQUIRE( PollTimeoutMs( state, k_ulStart + k_ulGrace - 1'500'000ul ) == 2 );
	REQUIRE( PollTimeoutMs( state, k_ulStart + k_ulGrace ) == 0 );
	REQUIRE( PollTimeoutMs( state, k_ulStart + 2 * k_ulGrace ) == 0 );
}
