#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace gamescope::window_icon
{
// EWMH defines _NET_WM_ICON as a sequence of records, each [width, height,
// width*height ARGB pixels], with no bound on the record count or on the
// dimensions, and the dimensions are client-supplied independently of how
// many words the client actually sent.

inline constexpr uint32_t k_uMaxIconDimension = 1024;

struct WindowIcon
{
	uint32_t uWidth;
	uint32_t uHeight;
	std::span<const uint32_t> pixels;
};

inline std::optional<WindowIcon> ParseWindowIcon( std::span<const uint32_t> words )
{
	std::optional<WindowIcon> oBest;
	uint64_t ulBestArea = 0;

	size_t uOffset = 0;
	while ( words.size() - uOffset >= 2 )
	{
		const uint32_t uWidth = words[uOffset];
		const uint32_t uHeight = words[uOffset + 1];

		const uint64_t ulArea = uint64_t( uWidth ) * uint64_t( uHeight );
		const uint64_t ulRecordWords = 2 + ulArea;

		if ( ulRecordWords > uint64_t( words.size() - uOffset ) )
			break;

		const bool bUsable =
			uWidth != 0 && uHeight != 0 &&
			uWidth <= k_uMaxIconDimension && uHeight <= k_uMaxIconDimension;

		if ( bUsable && ulArea > ulBestArea )
		{
			ulBestArea = ulArea;
			oBest = WindowIcon
			{
				.uWidth = uWidth,
				.uHeight = uHeight,
				.pixels = words.subspan( uOffset + 2, size_t( ulArea ) ),
			};
		}

		uOffset += size_t( ulRecordWords );
	}

	return oBest;
}
}
