#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "wayland_scale_helpers.hpp"

namespace
{
using gamescope::wayland::WaylandScalePositionToLogical;
using gamescope::wayland::WaylandScaleToLogical;
using gamescope::wayland::WaylandScaleToPhysical;
}

TEST_CASE( "A scale factor of 120 is the identity", "[wayland_scale]" )
{
	REQUIRE( WaylandScaleToPhysical( 1920u, 120u ) == 1920u );
	REQUIRE( WaylandScaleToLogical( 1920u, 120u ) == 1920u );
	REQUIRE( WaylandScalePositionToLogical( 1920, 120u ) == 1920 );
	REQUIRE( WaylandScalePositionToLogical( -1920, 120u ) == -1920 );
}

TEST_CASE( "Sizes convert between physical and logical pixels", "[wayland_scale]" )
{
	REQUIRE( WaylandScaleToPhysical( 100u, 180u ) == 150u );
	REQUIRE( WaylandScaleToLogical( 150u, 180u ) == 100u );

	REQUIRE( WaylandScaleToLogical( 1u, 180u ) == 1u );
	REQUIRE( WaylandScaleToLogical( 100u, 180u ) == 67u );
}

TEST_CASE( "Large sizes do not wrap a 32 bit multiply", "[wayland_scale]" )
{
	// 40000000 * 120 exceeds UINT32_MAX.
	REQUIRE( WaylandScaleToLogical( 40000000u, 240u ) == 20000000u );
	REQUIRE( WaylandScaleToPhysical( 40000000u, 240u ) == 80000000u );
}

TEST_CASE( "A zero position stays at the origin", "[wayland_scale]" )
{
	REQUIRE( WaylandScalePositionToLogical( 0, 180u ) == 0 );
	REQUIRE( WaylandScalePositionToLogical( 0, 240u ) == 0 );
}

TEST_CASE( "Negative positions mirror positive ones", "[wayland_scale]" )
{
	REQUIRE( WaylandScalePositionToLogical( 100, 180u ) == 67 );
	REQUIRE( WaylandScalePositionToLogical( -100, 180u ) == -67 );

	REQUIRE( WaylandScalePositionToLogical( 10, 180u ) == 7 );
	REQUIRE( WaylandScalePositionToLogical( -10, 180u ) == -7 );

	REQUIRE( WaylandScalePositionToLogical( 1, 240u ) == 1 );
	REQUIRE( WaylandScalePositionToLogical( -1, 240u ) == -1 );
}

TEST_CASE( "Positions round to nearest, not up", "[wayland_scale]" )
{
	// 20 * 120 / 180 = 13.33: the unsigned size helper rounds up, positions do not.
	REQUIRE( WaylandScaleToLogical( 20u, 180u ) == 14u );
	REQUIRE( WaylandScalePositionToLogical( 20, 180u ) == 13 );
	REQUIRE( WaylandScalePositionToLogical( -20, 180u ) == -13 );
}

TEST_CASE( "Large positions do not wrap", "[wayland_scale]" )
{
	REQUIRE( WaylandScalePositionToLogical( 2000000, 240u ) == 1000000 );
	REQUIRE( WaylandScalePositionToLogical( -2000000, 240u ) == -1000000 );

	REQUIRE( WaylandScalePositionToLogical( INT32_MIN, 120u ) == INT32_MIN );
	REQUIRE( WaylandScalePositionToLogical( INT32_MAX, 120u ) == INT32_MAX );
}

TEST_CASE( "Scaling to physical truncates rather than rounding", "[wayland_scale]" )
{
	using gamescope::wayland::WaylandScaleToPhysical;

	// 1281 * 90 / 120 is 960.75: the physical direction floors where its logical
	// counterpart rounds up, so a size scaled down and back never grows.
	REQUIRE( WaylandScaleToPhysical( 1281, 90 ) == 960 );
	REQUIRE( WaylandScaleToPhysical( 1, 180 ) == 1 );
	REQUIRE( WaylandScaleToPhysical( 1, 60 ) == 0 );
}
