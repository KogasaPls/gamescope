#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "capture_tonemap.hpp"

using gamescope::capture_tonemap::CaptureTonemap;
using gamescope::capture_tonemap::Resolve;

TEST_CASE( "The defaults reproduce the fixed 500 nit mapping without roll-off", "[capture_tonemap]" )
{
	const CaptureTonemap t = Resolve( 500.f, 0.f, 3 );

	REQUIRE( t.flSDRWhiteNits == 500.f );
	REQUIRE_FALSE( t.bRollOff );
}

TEST_CASE( "A peak above the SDR white level rolls highlights off into it", "[capture_tonemap]" )
{
	const CaptureTonemap t = Resolve( 200.f, 1000.f, 3 );

	REQUIRE( t.flSDRWhiteNits == 200.f );
	REQUIRE( t.bRollOff );
	REQUIRE( t.flSourcePeakNits == 1000.f );
	REQUIRE( t.nOperator == 3 );
}

TEST_CASE( "A peak at or below the SDR white level leaves nothing to roll off", "[capture_tonemap]" )
{
	REQUIRE_FALSE( Resolve( 200.f, 200.f, 3 ).bRollOff );
	REQUIRE_FALSE( Resolve( 200.f, 150.f, 3 ).bRollOff );
}

TEST_CASE( "An unusable SDR white level falls back to 500 nits", "[capture_tonemap]" )
{
	REQUIRE( Resolve( 0.f, 1000.f, 3 ).flSDRWhiteNits == 500.f );
	REQUIRE( Resolve( -10.f, 1000.f, 3 ).flSDRWhiteNits == 500.f );
	REQUIRE( Resolve( std::numeric_limits<float>::quiet_NaN(), 1000.f, 3 ).flSDRWhiteNits == 500.f );
}

TEST_CASE( "A non-finite peak disables roll-off", "[capture_tonemap]" )
{
	REQUIRE_FALSE( Resolve( 200.f, std::numeric_limits<float>::infinity(), 3 ).bRollOff );
	REQUIRE_FALSE( Resolve( 200.f, std::numeric_limits<float>::quiet_NaN(), 3 ).bRollOff );
}

TEST_CASE( "Levels beyond PQ's 10000 nit range are clamped to it", "[capture_tonemap]" )
{
	const CaptureTonemap t = Resolve( 20000.f, 40000.f, 3 );

	REQUIRE( t.flSDRWhiteNits == 10000.f );
	REQUIRE_FALSE( t.bRollOff );
}

TEST_CASE( "An operator outside the EETF 2390 variants selects max channel", "[capture_tonemap]" )
{
	REQUIRE( Resolve( 200.f, 1000.f, 1 ).nOperator == 1 );
	REQUIRE( Resolve( 200.f, 1000.f, 2 ).nOperator == 2 );
	REQUIRE( Resolve( 200.f, 1000.f, 0 ).nOperator == 3 );
	REQUIRE( Resolve( 200.f, 1000.f, 7 ).nOperator == 3 );
}
