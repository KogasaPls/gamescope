#pragma once

#include <algorithm>
#include <cmath>

#include <pixman.h>

extern "C" {
#include <wlr/util/region.h>
}

namespace gamescope::wlserver_cursor
{
// Intersecting an input region with a 0x0 surface leaves a single box with
// no area that pixman_region32_empty() reports as non-empty.
static inline bool region_has_area( const pixman_region32_t *pRegion )
{
	const pixman_box32_t *pExtents = pixman_region32_extents( const_cast<pixman_region32_t *>( pRegion ) );
	return pExtents->x1 < pExtents->x2 && pExtents->y1 < pExtents->y2;
}

static inline bool clamp_point_into_region( const pixman_region32_t *pRegion, double *pX, double *pY )
{
	int nBoxes = 0;
	pixman_box32_t *pBoxes = pixman_region32_rectangles( const_cast<pixman_region32_t *>( pRegion ), &nBoxes );
	const double xOriginal = *pX;
	const double yOriginal = *pY;
	bool bFound = false;
	double flBestDistanceSquared = 0.0;

	for ( int i = 0; i < nBoxes; i++ )
	{
		const pixman_box32_t &box = pBoxes[i];
		if ( box.x2 <= box.x1 || box.y2 <= box.y1 )
			continue;

		// Pixman box bounds are half-open, so the last pixel inside is x2 - 1.
		const double xCandidate = std::clamp<double>( xOriginal, box.x1, box.x2 - 1 );
		const double yCandidate = std::clamp<double>( yOriginal, box.y1, box.y2 - 1 );
		const double flDx = xCandidate - xOriginal;
		const double flDy = yCandidate - yOriginal;
		const double flDistanceSquared = flDx * flDx + flDy * flDy;

		if ( !bFound || flDistanceSquared < flBestDistanceSquared )
		{
			bFound = true;
			flBestDistanceSquared = flDistanceSquared;
			*pX = xCandidate;
			*pY = yCandidate;
		}
	}

	return bFound;
}

static inline bool apply_confine_constraint( bool locked, const pixman_region32_t *pConfineRegion, double sx, double sy, double *pDx, double *pDy )
{
	if ( locked )
		return false;

	// An empty confine region here means we have nothing sensible to confine
	// to; let motion through rather than freezing the cursor.
	// wlserver_clampcursor() still bounds it.
	if ( pixman_region32_empty( pConfineRegion ) )
		return true;

	double sxConfined, syConfined;
	if ( !wlr_region_confine( pConfineRegion, sx, sy, sx + *pDx, sy + *pDy, &sxConfined, &syConfined ) )
	{
		// wlr_region_confine fails outright when the starting point is
		// outside the region -- reachable when the client warps the cursor
		// out of its own confine area, since warps bypass the constraint.
		// Without recovery every subsequent motion fails the same way and
		// the cursor is frozen until the constraint is recreated. Clamp the
		// intended destination into the region instead.
		if ( !pixman_region32_contains_point( const_cast<pixman_region32_t *>( pConfineRegion ),
				(int)std::floor( sx ), (int)std::floor( sy ), nullptr ) )
		{
			double xClamped = sx + *pDx;
			double yClamped = sy + *pDy;
			if ( clamp_point_into_region( pConfineRegion, &xClamped, &yClamped ) )
			{
				*pDx = xClamped - sx;
				*pDy = yClamped - sy;
				return true;
			}
		}
		return false;
	}

	*pDx = sxConfined - sx;
	*pDy = syConfined - sy;

	if ( *pDx == 0.0 && *pDy == 0.0 )
		return false;

	return true;
}
}
