#include <catch2/catch_test_macros.hpp>

#include "wsi_present_mode_helpers.hpp"

namespace
{
using gamescope::wsi::ComputeDriverPresentModes;
using gamescope::wsi::MapPassthroughPresentMode;
using gamescope::wsi::PresentModeSwapchain;

constexpr uint32_t k_uFifo = 2;
constexpr uint32_t k_uFifoRelaxed = 3;
constexpr uint32_t k_uMailbox = 1;
constexpr uint32_t k_uImmediate = 0;
constexpr uint32_t k_uFifoLatestReady = 1000361000;

std::optional<std::vector<uint32_t>> Compute(
	std::vector<PresentModeSwapchain> swapchains,
	std::optional<std::vector<uint32_t>> oAppModes )
{
	std::optional<std::span<const uint32_t>> oAppModeSpan;
	if ( oAppModes )
		oAppModeSpan = std::span<const uint32_t>{ *oAppModes };

	return ComputeDriverPresentModes( swapchains, oAppModeSpan, k_uFifo, k_uFifoRelaxed, k_uMailbox );
}

constexpr PresentModeSwapchain Hooked{ .bHooked = true, .bPassthrough = false };
constexpr PresentModeSwapchain Passthrough{ .bHooked = true, .bPassthrough = true };
constexpr PresentModeSwapchain Unhooked{ .bHooked = false, .bPassthrough = false };
}

TEST_CASE( "Every hooked swapchain is substituted with one mode each", "[wsi_present_mode]" )
{
	const auto oModes = Compute( { Hooked, Hooked, Hooked }, std::nullopt );

	REQUIRE( oModes );
	REQUIRE( *oModes == std::vector<uint32_t>{ k_uMailbox, k_uMailbox, k_uMailbox } );
}

TEST_CASE( "A passthrough swapchain keeps the mode the app asked for", "[wsi_present_mode]" )
{
	const auto oModes = Compute(
		{ Hooked, Passthrough },
		std::vector<uint32_t>{ k_uImmediate, k_uFifo } );

	REQUIRE( oModes );
	REQUIRE( *oModes == std::vector<uint32_t>{ k_uMailbox, k_uFifo } );
}

TEST_CASE( "A passthrough swapchain with no app modes substitutes nothing", "[wsi_present_mode]" )
{
	REQUIRE_FALSE( Compute( { Hooked, Passthrough }, std::nullopt ) );
}

TEST_CASE( "An unhooked swapchain with no app modes substitutes nothing", "[wsi_present_mode]" )
{
	REQUIRE_FALSE( Compute( { Hooked, Unhooked }, std::nullopt ) );
}

TEST_CASE( "An unhooked swapchain copies the app's own mode", "[wsi_present_mode]" )
{
	const auto oModes = Compute(
		{ Hooked, Unhooked },
		std::vector<uint32_t>{ k_uFifo, k_uImmediate } );

	REQUIRE( oModes );
	REQUIRE( *oModes == std::vector<uint32_t>{ k_uMailbox, k_uImmediate } );
}

TEST_CASE( "Nothing is substituted when no swapchain needs it", "[wsi_present_mode]" )
{
	REQUIRE_FALSE( Compute( { Passthrough, Unhooked },
		std::vector<uint32_t>{ k_uFifo, k_uFifo } ) );
	REQUIRE_FALSE( Compute( {}, std::nullopt ) );
}

TEST_CASE( "App modes too short for the present are not indexed", "[wsi_present_mode]" )
{
	REQUIRE_FALSE( Compute( { Hooked, Unhooked }, std::vector<uint32_t>{ k_uFifo } ) );

	const auto oModes = Compute( { Hooked, Hooked }, std::vector<uint32_t>{ k_uFifo } );

	REQUIRE( oModes );
	REQUIRE( *oModes == std::vector<uint32_t>{ k_uMailbox, k_uMailbox } );
}

TEST_CASE( "Passthrough maps relaxed FIFO onto FIFO and every other mode onto MAILBOX", "[wsi_present_mode]" )
{
	REQUIRE( MapPassthroughPresentMode( k_uFifo, k_uFifo, k_uFifoRelaxed, k_uMailbox ) == k_uFifo );
	REQUIRE( MapPassthroughPresentMode( k_uFifoRelaxed, k_uFifo, k_uFifoRelaxed, k_uMailbox ) == k_uFifo );
	REQUIRE( MapPassthroughPresentMode( k_uMailbox, k_uFifo, k_uFifoRelaxed, k_uMailbox ) == k_uMailbox );
	REQUIRE( MapPassthroughPresentMode( k_uImmediate, k_uFifo, k_uFifoRelaxed, k_uMailbox ) == k_uMailbox );
	REQUIRE( MapPassthroughPresentMode( k_uFifoLatestReady, k_uFifo, k_uFifoRelaxed, k_uMailbox ) == k_uMailbox );
}

TEST_CASE( "A passthrough swapchain never names a mode outside FIFO and MAILBOX per present", "[wsi_present_mode]" )
{
	const auto oRelaxed = Compute( { Passthrough }, std::vector<uint32_t>{ k_uFifoRelaxed } );
	REQUIRE( oRelaxed );
	REQUIRE( *oRelaxed == std::vector<uint32_t>{ k_uFifo } );

	const auto oLatest = Compute( { Passthrough, Unhooked }, std::vector<uint32_t>{ k_uFifoLatestReady, k_uImmediate } );
	REQUIRE( oLatest );
	REQUIRE( *oLatest == std::vector<uint32_t>{ k_uMailbox, k_uImmediate } );

	REQUIRE_FALSE( Compute( { Passthrough }, std::vector<uint32_t>{ k_uMailbox } ) );
}
