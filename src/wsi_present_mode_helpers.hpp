#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace gamescope::wsi
{
struct PresentModeSwapchain
{
	bool bHooked = false;
	// Created under the passthrough opt-in: its driver modes are the app's
	// requests mapped by MapPassthroughPresentMode, not the MAILBOX we
	// substitute otherwise.
	bool bPassthrough = false;
};

// gamescope advertises neither wp_fifo_v1 nor tearing control, so Mesa
// enumerates only FIFO and MAILBOX for its surfaces. Relaxed FIFO means FIFO
// without tearing; everything else becomes MAILBOX.
inline constexpr uint32_t MapPassthroughPresentMode( uint32_t uMode, uint32_t uFifoMode, uint32_t uFifoRelaxedMode, uint32_t uMailboxMode )
{
	return ( uMode == uFifoMode || uMode == uFifoRelaxedMode ) ? uFifoMode : uMailboxMode;
}

// VkSwapchainPresentModeInfoEXT is one array covering every swapchain of the
// present, in the same order, and a mode named for a swapchain must be in
// that swapchain's compatibility set. So this is all or nothing: a swapchain
// we did not create, or one whose compatibility set we left alone, can only
// be named if the app supplied its own modes for us to copy.
inline std::optional<std::vector<uint32_t>> ComputeDriverPresentModes(
	std::span<const PresentModeSwapchain> swapchains,
	std::optional<std::span<const uint32_t>> oAppModes,
	uint32_t uFifoMode,
	uint32_t uFifoRelaxedMode,
	uint32_t uMailboxMode )
{
	if ( oAppModes && oAppModes->size() < swapchains.size() )
		oAppModes = std::nullopt;

	std::vector<uint32_t> driverModes;
	driverModes.reserve( swapchains.size() );

	bool bSubstituted = false;
	for ( size_t i = 0; i < swapchains.size(); i++ )
	{
		if ( !swapchains[i].bHooked )
		{
			if ( !oAppModes )
				return std::nullopt;

			driverModes.emplace_back( ( *oAppModes )[i] );
		}
		else if ( swapchains[i].bPassthrough )
		{
			if ( !oAppModes )
				return std::nullopt;

			const uint32_t uMode = MapPassthroughPresentMode( ( *oAppModes )[i], uFifoMode, uFifoRelaxedMode, uMailboxMode );
			bSubstituted |= uMode != ( *oAppModes )[i];
			driverModes.emplace_back( uMode );
		}
		else
		{
			bSubstituted = true;
			driverModes.emplace_back( uMailboxMode );
		}
	}

	if ( !bSubstituted )
		return std::nullopt;

	return driverModes;
}
}
