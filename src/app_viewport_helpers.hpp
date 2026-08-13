#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

namespace gamescope::app_viewport
{
struct Rect
{
	int32_t nX = 0;
	int32_t nY = 0;
	uint32_t uWidth = 0;
	uint32_t uHeight = 0;

	constexpr bool operator==( const Rect & ) const = default;
	constexpr bool IsValid() const { return uWidth != 0 && uHeight != 0; }
};

// A window this small is a teardown artefact, not a size to follow.
inline constexpr uint32_t k_uMinDim = 64;

inline constexpr Rect ComputeViewport( std::optional<Rect> oApp, uint32_t uOutputWidth, uint32_t uOutputHeight )
{
	const Rect output{ 0, 0, uOutputWidth, uOutputHeight };
	if ( !oApp || !oApp->IsValid() || !output.IsValid() )
		return output;

	Rect app = *oApp;
	app.uWidth  = std::min( std::max( app.uWidth,  k_uMinDim ), uOutputWidth );
	app.uHeight = std::min( std::max( app.uHeight, k_uMinDim ), uOutputHeight );
	app.nX = std::clamp<int32_t>( app.nX, 0, int32_t( uOutputWidth  - app.uWidth ) );
	app.nY = std::clamp<int32_t>( app.nY, 0, int32_t( uOutputHeight - app.uHeight ) );
	return app;
}

struct HostConfigureState
{
	bool bHostDictated = false;
	Rect floatingOutput;
	Rect floatingViewport;
	Rect viewport;

	constexpr bool operator==( const HostConfigureState & ) const = default;
};

struct HostConfigureInput
{
	bool bHostDictated = false;
	Rect configured;
	Rect output;
	bool bTrackAppSize = false;
};

enum class HostConfigureAction { AdoptConfigured, RestoreFloating, KeepOutput };

struct HostConfigureResult
{
	HostConfigureState state;
	HostConfigureAction eAction = HostConfigureAction::KeepOutput;
	Rect outputToAdopt;
	bool bCroppingOutput = false;
};

// A host-dictated state sizes the output. Leaving it, the host restores the
// floating geometry it last saw, the app-sized window, so the output goes
// back to the floating size it had instead. A window the host dictated from
// the start, as sway does by tiling at map, has no floating size to go back
// to and keeps the dictated one. While --track-app-size floats the window its
// size is gamescope's, so only the first configure, before any Present, is
// adopted.
constexpr HostConfigureResult DecideHostConfigure( const HostConfigureState &state, const HostConfigureInput &input )
{
	HostConfigureResult result{ .state = state };
	const bool bLeavingDictated = state.bHostDictated && !input.bHostDictated;
	if ( input.bHostDictated && !state.bHostDictated )
	{
		result.state.floatingOutput = input.output;
		result.state.floatingViewport = state.viewport;
	}
	result.state.bHostDictated = input.bHostDictated;

	if ( input.bHostDictated )
	{
		result.eAction = HostConfigureAction::AdoptConfigured;
		result.outputToAdopt = input.configured;
		return result;
	}

	if ( bLeavingDictated && input.bTrackAppSize && state.floatingOutput.IsValid() && state.floatingViewport.IsValid() )
	{
		result.eAction = HostConfigureAction::RestoreFloating;
		result.outputToAdopt = state.floatingOutput;
		result.state.viewport = state.floatingViewport;
		result.bCroppingOutput = state.floatingViewport.IsValid() && state.floatingViewport != state.floatingOutput;
		return result;
	}

	if ( !input.bTrackAppSize || !state.viewport.IsValid() )
	{
		result.eAction = HostConfigureAction::AdoptConfigured;
		result.outputToAdopt = input.configured;
	}
	return result;
}

struct PlaneRect
{
	int32_t nDestX = 0;
	int32_t nDestY = 0;
	double flSrcX = 0.0;
	double flSrcY = 0.0;
	double flSrcWidth = 0.0;
	double flSrcHeight = 0.0;
	int32_t nDstWidth = 0;
	int32_t nDstHeight = 0;

	constexpr bool operator==( const PlaneRect & ) const = default;
};

// wp_viewport carries the source rectangle in 24.8 fixed point, so anything
// below one 256th of a pixel is not representable.
inline constexpr double k_flMinSourceExtent = 1.0 / 256.0;

inline std::optional<PlaneRect> ClipPlaneToViewport( PlaneRect plane, const Rect &viewport )
{
	// wp_viewport takes the source rectangle in 24.8 fixed point and rejects a
	// non-positive one, so a crop that rounds to zero there has to go.
	if ( plane.nDstWidth <= 0 || plane.nDstHeight <= 0 ||
	     plane.flSrcWidth < k_flMinSourceExtent || plane.flSrcHeight < k_flMinSourceExtent )
		return std::nullopt;

	const double flSrcPerDstX = plane.flSrcWidth  / plane.nDstWidth;
	const double flSrcPerDstY = plane.flSrcHeight / plane.nDstHeight;

	plane.nDestX -= viewport.nX;
	plane.nDestY -= viewport.nY;

	if ( plane.nDestX < 0 )
	{
		const int32_t nCut = -plane.nDestX;
		plane.flSrcX     += nCut * flSrcPerDstX;
		plane.flSrcWidth -= nCut * flSrcPerDstX;
		plane.nDstWidth  -= nCut;
		plane.nDestX = 0;
	}
	if ( plane.nDestY < 0 )
	{
		const int32_t nCut = -plane.nDestY;
		plane.flSrcY      += nCut * flSrcPerDstY;
		plane.flSrcHeight -= nCut * flSrcPerDstY;
		plane.nDstHeight  -= nCut;
		plane.nDestY = 0;
	}

	const int32_t nMaxWidth  = int32_t( viewport.uWidth )  - plane.nDestX;
	const int32_t nMaxHeight = int32_t( viewport.uHeight ) - plane.nDestY;
	if ( plane.nDstWidth > nMaxWidth )
	{
		plane.flSrcWidth = nMaxWidth * flSrcPerDstX;
		plane.nDstWidth  = nMaxWidth;
	}
	if ( plane.nDstHeight > nMaxHeight )
	{
		plane.flSrcHeight = nMaxHeight * flSrcPerDstY;
		plane.nDstHeight  = nMaxHeight;
	}

	if ( plane.nDstWidth <= 0 || plane.nDstHeight <= 0 ||
	     plane.flSrcWidth < k_flMinSourceExtent || plane.flSrcHeight < k_flMinSourceExtent )
		return std::nullopt;

	return plane;
}
}
