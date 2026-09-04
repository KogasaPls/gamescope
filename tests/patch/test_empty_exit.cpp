#include <catch2/catch_test_macros.hpp>

#include "empty_exit_helpers.hpp"

namespace
{
using gamescope::empty_exit::GraceNanosFromSeconds;
using gamescope::empty_exit::k_ulDefaultGraceNanos;
using gamescope::empty_exit::State;
using gamescope::empty_exit::UpdateEmpty;

constexpr uint64_t k_ulSecond = 1'000'000'000ul;
constexpr uint64_t k_ulGrace = 5 * k_ulSecond;
}

TEST_CASE( "A grace period in seconds becomes nanoseconds", "[empty_exit]" )
{
	REQUIRE( k_ulDefaultGraceNanos == 30 * k_ulSecond );
	REQUIRE( GraceNanosFromSeconds( 9 ) == 9 * k_ulSecond );
}

TEST_CASE( "A flag carrying no usable value takes the default grace period", "[empty_exit]" )
{
	REQUIRE( GraceNanosFromSeconds( 0 ) == k_ulDefaultGraceNanos );
	REQUIRE( GraceNanosFromSeconds( -1 ) == k_ulDefaultGraceNanos );
}

TEST_CASE( "A shown window latches the session and holds the clock at zero", "[empty_exit]" )
{
	State state;

	REQUIRE_FALSE( UpdateEmpty( state, true, 1 * k_ulSecond, k_ulGrace ) );
	REQUIRE( state.bHasShownWindow );
	REQUIRE( state.ulEmptySinceNanos == 0 );

	REQUIRE_FALSE( UpdateEmpty( state, true, 100 * k_ulSecond, k_ulGrace ) );
	REQUIRE( state.ulEmptySinceNanos == 0 );
}

TEST_CASE( "An empty session before the first window never expires", "[empty_exit]" )
{
	State state;

	for ( uint64_t ulNow = 0; ulNow < 100 * k_ulSecond; ulNow += k_ulSecond )
		REQUIRE_FALSE( UpdateEmpty( state, false, ulNow, k_ulGrace ) );

	REQUIRE_FALSE( state.bHasShownWindow );
	REQUIRE( state.ulEmptySinceNanos == 0 );
}

TEST_CASE( "The grace period is measured from the first empty iteration", "[empty_exit]" )
{
	State state;

	UpdateEmpty( state, true, 10 * k_ulSecond, k_ulGrace );

	REQUIRE_FALSE( UpdateEmpty( state, false, 11 * k_ulSecond, k_ulGrace ) );
	REQUIRE( state.ulEmptySinceNanos == 11 * k_ulSecond );

	REQUIRE_FALSE( UpdateEmpty( state, false, 16 * k_ulSecond - 1, k_ulGrace ) );
	REQUIRE( UpdateEmpty( state, false, 16 * k_ulSecond, k_ulGrace ) );
}

TEST_CASE( "A window appearing mid-grace restarts the clock", "[empty_exit]" )
{
	State state;

	UpdateEmpty( state, true, 0, k_ulGrace );
	UpdateEmpty( state, false, 1 * k_ulSecond, k_ulGrace );
	UpdateEmpty( state, true, 4 * k_ulSecond, k_ulGrace );

	REQUIRE_FALSE( UpdateEmpty( state, false, 5 * k_ulSecond, k_ulGrace ) );
	REQUIRE_FALSE( UpdateEmpty( state, false, 9 * k_ulSecond, k_ulGrace ) );
	REQUIRE( UpdateEmpty( state, false, 10 * k_ulSecond, k_ulGrace ) );
}

TEST_CASE( "An absent flag leaves no grace period and never expires", "[empty_exit]" )
{
	State state;

	UpdateEmpty( state, true, 0, 0 );

	for ( uint64_t ulNow = k_ulSecond; ulNow < 100 * k_ulSecond; ulNow += k_ulSecond )
		REQUIRE_FALSE( UpdateEmpty( state, false, ulNow, 0 ) );
}

TEST_CASE( "Expiry holds on every later iteration until a window returns", "[empty_exit]" )
{
	State state;

	UpdateEmpty( state, true, 0, k_ulGrace );
	UpdateEmpty( state, false, k_ulSecond, k_ulGrace );

	REQUIRE( UpdateEmpty( state, false, 6 * k_ulSecond, k_ulGrace ) );
	REQUIRE( UpdateEmpty( state, false, 7 * k_ulSecond, k_ulGrace ) );
	REQUIRE_FALSE( UpdateEmpty( state, true, 8 * k_ulSecond, k_ulGrace ) );
}
