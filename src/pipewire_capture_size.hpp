#pragma once

#include <algorithm>
#include <cstdint>

namespace gamescope::pipewire_capture
{
struct Size
{
	uint32_t width, height;
};

constexpr Size FitSize(Size source, Size limit)
{
	if (!source.width || !source.height)
		return source;

	Size size = {std::min(limit.width, source.width), std::min(limit.height, source.height)};
	const uint64_t widthLimited = uint64_t(size.width) * source.height;
	const uint64_t heightLimited = uint64_t(size.height) * source.width;
	if (widthLimited <= heightLimited)
		size.height = (widthLimited + source.width - 1) / source.width;
	else
		size.width = (heightLimited + source.height - 1) / source.height;

	// Round up for chroma where possible, down at the bound. A one-pixel
	// bound only permits the RGB offer.
	auto align = [](uint32_t value, uint32_t bound) {
		if (value % 2 == 0)
			return value;
		return value < bound ? value + 1 : std::max(1u, value - 1);
	};
	return {align(size.width, limit.width), align(size.height, limit.height)};
}

constexpr bool SupportsNV12(Size size)
{
	return size.width && size.height && size.width % 2 == 0 && size.height % 2 == 0;
}
}
