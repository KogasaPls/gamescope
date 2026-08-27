#pragma once

#include <cstdint>

namespace gamescope::wayland
{
// wp_fractional_scale_v1 expresses a scale as a fraction of 120.
inline constexpr uint32_t k_uFractionalScaleDenominator = 120;

inline uint32_t WaylandScaleToPhysical( uint32_t uValue, uint32_t uFactor )
{
	return uint32_t( uint64_t( uValue ) * uFactor / k_uFractionalScaleDenominator );
}

inline uint32_t WaylandScaleToLogical( uint32_t uValue, uint32_t uFactor )
{
	const uint64_t ulNumerator = uint64_t( uValue ) * k_uFractionalScaleDenominator;
	return uint32_t( ( ulNumerator + uFactor - 1 ) / uFactor );
}

inline int32_t WaylandScalePositionToLogical( int32_t nValue, uint32_t uFactor )
{
	const int64_t lNumerator = int64_t( nValue ) * k_uFractionalScaleDenominator;
	const int64_t lFactor = int64_t( uFactor );

	const bool bNegative = lNumerator < 0;
	const int64_t lMagnitude = bNegative ? -lNumerator : lNumerator;
	const int64_t lRounded = ( lMagnitude + lFactor / 2 ) / lFactor;

	return int32_t( bNegative ? -lRounded : lRounded );
}
}
