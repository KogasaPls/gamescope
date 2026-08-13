#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>

#include "app_viewport_helpers.hpp"

namespace
{
using gamescope::app_viewport::ClipPlaneToViewport;
using gamescope::app_viewport::ComputeViewport;
using gamescope::app_viewport::k_uMinDim;
using gamescope::app_viewport::PlaneRect;
using gamescope::app_viewport::Rect;

constexpr uint32_t kOutW = 2560;
constexpr uint32_t kOutH = 1440;
constexpr Rect kOutput{ 0, 0, kOutW, kOutH };
}

TEST_CASE( "No app, or an app that fills the output, shows the whole output", "[app_viewport]" )
{
	REQUIRE( ComputeViewport( std::nullopt, kOutW, kOutH ) == kOutput );
	REQUIRE( ComputeViewport( Rect{ 0, 0, kOutW, kOutH }, kOutW, kOutH ) == kOutput );
	REQUIRE( ComputeViewport( Rect{ 0, 0, 0, 900 }, kOutW, kOutH ) == kOutput );
	REQUIRE( ComputeViewport( Rect{ 100, 100, 800, 600 }, 0, 0 ) == Rect{ 0, 0, 0, 0 } );
}

TEST_CASE( "A smaller app is shown where it sits on the output", "[app_viewport]" )
{
	REQUIRE( ComputeViewport( Rect{ 480, 270, 1600, 900 }, kOutW, kOutH ) == Rect{ 480, 270, 1600, 900 } );
}

TEST_CASE( "A transient tiny buffer is floored, not followed", "[app_viewport]" )
{
	const Rect viewport = ComputeViewport( Rect{ 1279, 719, 1, 1 }, kOutW, kOutH );
	REQUIRE( viewport.uWidth == k_uMinDim );
	REQUIRE( viewport.uHeight == k_uMinDim );
	REQUIRE( viewport.nX == 1279 );
	REQUIRE( viewport.nY == 719 );
}

TEST_CASE( "An app larger than the output, or hanging off its edge, is kept inside", "[app_viewport]" )
{
	REQUIRE( ComputeViewport( Rect{ -100, -100, 4000, 3000 }, kOutW, kOutH ) == kOutput );
	REQUIRE( ComputeViewport( Rect{ 2000, 1000, 800, 600 }, kOutW, kOutH ) == Rect{ 1760, 840, 800, 600 } );
	REQUIRE( ComputeViewport( Rect{ -50, 10, 800, 600 }, kOutW, kOutH ) == Rect{ 0, 10, 800, 600 } );
}

TEST_CASE( "An output below the floor is used whole", "[app_viewport]" )
{
	REQUIRE( ComputeViewport( Rect{ 0, 0, 10, 10 }, 32, 32 ) == Rect{ 0, 0, 32, 32 } );
}

TEST_CASE( "A plane inside the viewport is only translated", "[app_viewport]" )
{
	const Rect viewport{ 480, 270, 1600, 900 };
	const PlaneRect plane{ .nDestX = 480, .nDestY = 270, .flSrcX = 0, .flSrcY = 0, .flSrcWidth = 1600, .flSrcHeight = 900, .nDstWidth = 1600, .nDstHeight = 900 };

	const auto oClipped = ClipPlaneToViewport( plane, viewport );
	REQUIRE( oClipped );
	REQUIRE( *oClipped == PlaneRect{ .nDestX = 0, .nDestY = 0, .flSrcX = 0, .flSrcY = 0, .flSrcWidth = 1600, .flSrcHeight = 900, .nDstWidth = 1600, .nDstHeight = 900 } );
}

TEST_CASE( "The composite is cropped to the viewport", "[app_viewport]" )
{
	const Rect viewport{ 480, 270, 1600, 900 };
	const PlaneRect composite{ .nDestX = 0, .nDestY = 0, .flSrcX = 0, .flSrcY = 0, .flSrcWidth = kOutW, .flSrcHeight = kOutH, .nDstWidth = int32_t( kOutW ), .nDstHeight = int32_t( kOutH ) };

	const auto oClipped = ClipPlaneToViewport( composite, viewport );
	REQUIRE( oClipped );
	REQUIRE( *oClipped == PlaneRect{ .nDestX = 0, .nDestY = 0, .flSrcX = 480, .flSrcY = 270, .flSrcWidth = 1600, .flSrcHeight = 900, .nDstWidth = 1600, .nDstHeight = 900 } );
}

TEST_CASE( "A scaled plane crops its source in proportion", "[app_viewport]" )
{
	const Rect viewport{ 400, 0, 800, 600 };
	const PlaneRect plane{ .nDestX = 200, .nDestY = 100, .flSrcX = 0, .flSrcY = 0, .flSrcWidth = 800, .flSrcHeight = 450, .nDstWidth = 1600, .nDstHeight = 900 };

	const auto oClipped = ClipPlaneToViewport( plane, viewport );
	REQUIRE( oClipped );
	REQUIRE( oClipped->nDestX == 0 );
	REQUIRE( oClipped->nDestY == 100 );
	REQUIRE( oClipped->flSrcX == 100.0 );
	REQUIRE( oClipped->flSrcY == 0.0 );
	REQUIRE( oClipped->nDstWidth == 800 );
	REQUIRE( oClipped->nDstHeight == 500 );
	REQUIRE( oClipped->flSrcWidth == 400.0 );
	REQUIRE( oClipped->flSrcHeight == 250.0 );
}

TEST_CASE( "A plane entirely outside the viewport presents nothing", "[app_viewport]" )
{
	const Rect viewport{ 480, 270, 1600, 900 };
	REQUIRE_FALSE( ClipPlaneToViewport( PlaneRect{ .nDestX = 0, .nDestY = 0, .flSrcWidth = 100, .flSrcHeight = 100, .nDstWidth = 100, .nDstHeight = 100 }, viewport ) );
	REQUIRE_FALSE( ClipPlaneToViewport( PlaneRect{ .nDestX = 2100, .nDestY = 300, .flSrcWidth = 100, .flSrcHeight = 100, .nDstWidth = 100, .nDstHeight = 100 }, viewport ) );
	REQUIRE_FALSE( ClipPlaneToViewport( PlaneRect{ .nDestX = 500, .nDestY = 300, .flSrcWidth = 100, .flSrcHeight = 100, .nDstWidth = 0, .nDstHeight = 100 }, viewport ) );
}

TEST_CASE( "A plane touching the far edge keeps its last pixel", "[app_viewport]" )
{
	const Rect viewport{ 0, 0, 100, 100 };
	const auto oClipped = ClipPlaneToViewport( PlaneRect{ .nDestX = 99, .nDestY = 99, .flSrcWidth = 10, .flSrcHeight = 10, .nDstWidth = 10, .nDstHeight = 10 }, viewport );
	REQUIRE( oClipped );
	REQUIRE( oClipped->nDstWidth == 1 );
	REQUIRE( oClipped->nDstHeight == 1 );
	REQUIRE( oClipped->flSrcWidth == 1.0 );
}


TEST_CASE( "A crop below the viewport's fixed-point resolution is dropped", "[app_viewport]" )
{
	using gamescope::app_viewport::ClipPlaneToViewport;
	using gamescope::app_viewport::PlaneRect;
	using gamescope::app_viewport::Rect;

	// One source pixel stretched across a thousand, clipped to its last column:
	// the surviving source width rounds to zero in 24.8 fixed point.
	const PlaneRect plane{ .nDestX = -999, .nDestY = 0, .flSrcX = 0.0, .flSrcY = 0.0,
		.flSrcWidth = 1.0, .flSrcHeight = 1.0, .nDstWidth = 1000, .nDstHeight = 1000 };

	REQUIRE( !ClipPlaneToViewport( plane, Rect{ 0, 0, 1000, 1000 } ).has_value() );
}

TEST_CASE( "A left crop moves the source origin by the source-per-destination ratio", "[app_viewport]" )
{
	using gamescope::app_viewport::ClipPlaneToViewport;
	using gamescope::app_viewport::PlaneRect;
	using gamescope::app_viewport::Rect;

	// Two source pixels per destination pixel horizontally, four vertically, so
	// swapping the two ratios or dropping either changes the answer.
	const PlaneRect plane{ .nDestX = -10, .nDestY = -10, .flSrcX = 0.0, .flSrcY = 0.0,
		.flSrcWidth = 200.0, .flSrcHeight = 400.0, .nDstWidth = 100, .nDstHeight = 100 };

	const auto oClipped = ClipPlaneToViewport( plane, Rect{ 0, 0, 1000, 1000 } );

	REQUIRE( oClipped.has_value() );
	REQUIRE( oClipped->nDestX == 0 );
	REQUIRE( oClipped->nDestY == 0 );
	REQUIRE( oClipped->flSrcX == 20.0 );
	REQUIRE( oClipped->flSrcY == 40.0 );
	REQUIRE( oClipped->flSrcWidth == 180.0 );
	REQUIRE( oClipped->flSrcHeight == 360.0 );
	REQUIRE( oClipped->nDstWidth == 90 );
	REQUIRE( oClipped->nDstHeight == 90 );
}

namespace
{
using gamescope::app_viewport::DecideHostConfigure;
using gamescope::app_viewport::HostConfigureAction;
using gamescope::app_viewport::HostConfigureInput;
using gamescope::app_viewport::HostConfigureState;
using gamescope::app_viewport::Rect;

constexpr Rect k_Output{ 0, 0, 1280, 720 };
constexpr Rect k_Viewport{ 0, 0, 800, 600 };
}

TEST_CASE( "The first configure is adopted before any Present", "[app_viewport][configure]" )
{
	const auto result = DecideHostConfigure( HostConfigureState{}, HostConfigureInput{ false, Rect{ 0, 0, 1280, 720 }, k_Output, true } );
	REQUIRE( result.eAction == HostConfigureAction::AdoptConfigured );
	REQUIRE( result.outputToAdopt == Rect{ 0, 0, 1280, 720 } );
	REQUIRE( result.state == HostConfigureState{} );
}

TEST_CASE( "A later floating configure is not adopted while tracking", "[app_viewport][configure]" )
{
	const HostConfigureState state{ false, {}, {}, k_Viewport };
	const auto result = DecideHostConfigure( state, HostConfigureInput{ false, Rect{ 0, 0, 1000, 1000 }, k_Output, true } );
	REQUIRE( result.eAction == HostConfigureAction::KeepOutput );
	REQUIRE( result.state == state );
}

TEST_CASE( "A dictated configure sizes the output and remembers the floating geometry", "[app_viewport][configure]" )
{
	const HostConfigureState state{ false, {}, {}, k_Viewport };
	const auto result = DecideHostConfigure( state, HostConfigureInput{ true, Rect{ 0, 0, 1920, 1080 }, k_Output, true } );
	REQUIRE( result.eAction == HostConfigureAction::AdoptConfigured );
	REQUIRE( result.outputToAdopt == Rect{ 0, 0, 1920, 1080 } );
	REQUIRE( result.state.bHostDictated );
	REQUIRE( result.state.floatingOutput == k_Output );
	REQUIRE( result.state.floatingViewport == k_Viewport );
}

TEST_CASE( "Entering a dictated state twice keeps the first floating geometry", "[app_viewport][configure]" )
{
	const HostConfigureState state{ true, k_Output, k_Viewport, Rect{ 0, 0, 1920, 1080 } };
	const auto result = DecideHostConfigure( state, HostConfigureInput{ true, Rect{ 0, 0, 2560, 1440 }, Rect{ 0, 0, 1920, 1080 }, true } );
	REQUIRE( result.eAction == HostConfigureAction::AdoptConfigured );
	REQUIRE( result.state.floatingOutput == k_Output );
	REQUIRE( result.state.floatingViewport == k_Viewport );
}

TEST_CASE( "Leaving a dictated state restores the floating output and viewport", "[app_viewport][configure]" )
{
	const HostConfigureState state{ true, k_Output, k_Viewport, Rect{ 0, 0, 1920, 1080 } };
	const auto result = DecideHostConfigure( state, HostConfigureInput{ false, Rect{ 0, 0, 800, 600 }, Rect{ 0, 0, 1920, 1080 }, true } );
	REQUIRE( result.eAction == HostConfigureAction::RestoreFloating );
	REQUIRE( result.outputToAdopt == k_Output );
	REQUIRE( result.state.viewport == k_Viewport );
	REQUIRE_FALSE( result.state.bHostDictated );
	REQUIRE( result.bCroppingOutput );
}

TEST_CASE( "A window dictated from map keeps the dictated size when it leaves", "[app_viewport][configure]" )
{
	const HostConfigureState state{ true, k_Output, {}, Rect{ 0, 0, 1920, 1080 } };
	const auto result = DecideHostConfigure( state, HostConfigureInput{ false, Rect{ 0, 0, 800, 600 }, Rect{ 0, 0, 1920, 1080 }, true } );
	REQUIRE( result.eAction == HostConfigureAction::KeepOutput );
	REQUIRE_FALSE( result.state.bHostDictated );
}

TEST_CASE( "Tracking off adopts every configure", "[app_viewport][configure]" )
{
	const HostConfigureState state{ true, k_Output, k_Viewport, Rect{ 0, 0, 1920, 1080 } };
	const auto leaving = DecideHostConfigure( state, HostConfigureInput{ false, Rect{ 0, 0, 800, 600 }, Rect{ 0, 0, 1920, 1080 }, false } );
	REQUIRE( leaving.eAction == HostConfigureAction::AdoptConfigured );
	REQUIRE( leaving.outputToAdopt == Rect{ 0, 0, 800, 600 } );
	const auto floating = DecideHostConfigure( HostConfigureState{ false, {}, {}, k_Viewport }, HostConfigureInput{ false, Rect{ 0, 0, 1000, 1000 }, k_Output, false } );
	REQUIRE( floating.eAction == HostConfigureAction::AdoptConfigured );
}
