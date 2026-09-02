#include <catch2/catch_test_macros.hpp>

#include "wlserver_cursor_helpers.hpp"

#include <cstdint>
#include <limits>

namespace
{
struct ScopedRegion
{
	pixman_region32_t region;

	ScopedRegion()
	{
		pixman_region32_init( &region );
	}

	ScopedRegion( int32_t x, int32_t y, uint32_t width, uint32_t height )
	{
		pixman_region32_init_rect( &region, x, y, width, height );
	}

	~ScopedRegion()
	{
		pixman_region32_fini( &region );
	}
};

bool region_contains( pixman_region32_t *pRegion, int x, int y )
{
	return pixman_region32_contains_point( pRegion, x, y, nullptr );
}

// The default surface input region intersected with a 0x0 surface: extents
// with no area, but a data pointer that keeps pixman_region32_empty() false.
void make_degenerate_region( ScopedRegion &degenerate )
{
	ScopedRegion defaultInput{
		std::numeric_limits<int32_t>::min(),
		std::numeric_limits<int32_t>::min(),
		std::numeric_limits<uint32_t>::max(),
		std::numeric_limits<uint32_t>::max(),
	};

	REQUIRE( pixman_region32_intersect_rect(
		&degenerate.region, &defaultInput.region, 0, 0, 0, 0 ) );
}
}

TEST_CASE( "wlserver cursor constraint region area", "[wlserver][cursor]" )
{
	SECTION( "a region whose extents have no area has no area" )
	{
		ScopedRegion degenerate;
		make_degenerate_region( degenerate );

		REQUIRE_FALSE( gamescope::wlserver_cursor::region_has_area( &degenerate.region ) );
	}

	SECTION( "an empty region has no area" )
	{
		ScopedRegion emptyConstraint;

		REQUIRE_FALSE( gamescope::wlserver_cursor::region_has_area( &emptyConstraint.region ) );
	}

	SECTION( "a region with extent has area" )
	{
		ScopedRegion constraint{ 10, 20, 30, 40 };

		REQUIRE( gamescope::wlserver_cursor::region_has_area( &constraint.region ) );
	}
}

TEST_CASE( "wlserver cursor confinement motion", "[wlserver][cursor]" )
{
	SECTION( "a default-initialized confine region safely allows boundary motion" )
	{
		ScopedRegion confine;
		double dx = -1.0;
		double dy = 0.0;

		REQUIRE( pixman_region32_empty( &confine.region ) );
		REQUIRE( gamescope::wlserver_cursor::apply_confine_constraint( false, &confine.region, 0.0, 0.0, &dx, &dy ) );
		REQUIRE( dx == -1.0 );
		REQUIRE( dy == 0.0 );
	}

	SECTION( "a degenerate confine region does not clamp with inverted bounds" )
	{
		ScopedRegion degenerate;
		make_degenerate_region( degenerate );

		double dx = -1.0;
		double dy = 0.0;

		REQUIRE_FALSE( gamescope::wlserver_cursor::apply_confine_constraint(
			false, &degenerate.region, 0.0, 0.0, &dx, &dy ) );
		REQUIRE( dx == -1.0 );
		REQUIRE( dy == 0.0 );
	}

	SECTION( "clamping into a region lands on a point the region contains" )
	{
		ScopedRegion confine{ 0, 0, 100, 100 };
		double x = 150.0;
		double y = -20.0;

		REQUIRE( gamescope::wlserver_cursor::clamp_point_into_region( &confine.region, &x, &y ) );
		REQUIRE( x == 99.0 );
		REQUIRE( y == 0.0 );
		REQUIRE( region_contains( &confine.region, (int)x, (int)y ) );
	}

	SECTION( "clamping into a disjoint region chooses the nearest rectangle" )
	{
		ScopedRegion confine{ 0, 0, 10, 10 };
		REQUIRE( pixman_region32_union_rect( &confine.region, &confine.region, 100, 100, 10, 10 ) );

		double x = 111.0;
		double y = 105.0;

		REQUIRE( gamescope::wlserver_cursor::clamp_point_into_region( &confine.region, &x, &y ) );
		REQUIRE( x == 109.0 );
		REQUIRE( y == 105.0 );
		REQUIRE( region_contains( &confine.region, (int)x, (int)y ) );

		// In the hole between the two rectangles, where clamping to the
		// extents would leave the point outside the region.
		x = 50.0;
		y = 50.0;

		REQUIRE( gamescope::wlserver_cursor::clamp_point_into_region( &confine.region, &x, &y ) );
		REQUIRE( x == 9.0 );
		REQUIRE( y == 9.0 );
		REQUIRE( region_contains( &confine.region, (int)x, (int)y ) );
	}

	SECTION( "clamping into a region with no area is refused" )
	{
		ScopedRegion degenerate;
		make_degenerate_region( degenerate );

		double x = 5.0;
		double y = 5.0;

		REQUIRE_FALSE( gamescope::wlserver_cursor::clamp_point_into_region( &degenerate.region, &x, &y ) );
		REQUIRE( x == 5.0 );
		REQUIRE( y == 5.0 );
	}

	SECTION( "locked constraints reject pointer motion" )
	{
		ScopedRegion confine{ 0, 0, 100, 100 };
		double dx = 1.0;
		double dy = 1.0;

		REQUIRE_FALSE( gamescope::wlserver_cursor::apply_confine_constraint( true, &confine.region, 10.0, 10.0, &dx, &dy ) );
	}

	SECTION( "empty confine regions allow unconstrained pointer motion" )
	{
		ScopedRegion emptyConfine;
		double dx = 1.0;
		double dy = 1.0;

		REQUIRE( gamescope::wlserver_cursor::apply_confine_constraint( false, &emptyConfine.region, 10.0, 10.0, &dx, &dy ) );
		REQUIRE( dx == 1.0 );
		REQUIRE( dy == 1.0 );
	}

	SECTION( "motion inside the confine region is preserved" )
	{
		ScopedRegion confine{ 0, 0, 100, 100 };
		double dx = 5.0;
		double dy = 7.0;

		REQUIRE( gamescope::wlserver_cursor::apply_confine_constraint( false, &confine.region, 10.0, 10.0, &dx, &dy ) );
		REQUIRE( dx == 5.0 );
		REQUIRE( dy == 7.0 );
	}

	SECTION( "cursor outside the confine region is clamped back inside" )
	{
		ScopedRegion confine{ 0, 0, 100, 100 };
		double dx = -1.0;
		double dy = 2.0;

		// Client warped the cursor to (150, 50), outside its own confine
		// region; motion must not freeze -- it clamps back into the region.
		REQUIRE( gamescope::wlserver_cursor::apply_confine_constraint( false, &confine.region, 150.0, 50.0, &dx, &dy ) );
		REQUIRE( 150.0 + dx == 99.0 );
		REQUIRE( 50.0 + dy == 52.0 );
	}

	SECTION( "motion clamped to zero is treated as blocked" )
	{
		ScopedRegion confine{ 0, 0, 100, 100 };
		double dx = -1.0;
		double dy = 0.0;

		REQUIRE_FALSE( gamescope::wlserver_cursor::apply_confine_constraint( false, &confine.region, 0.0, 10.0, &dx, &dy ) );
		REQUIRE( dx == 0.0 );
		REQUIRE( dy == 0.0 );
	}
}
