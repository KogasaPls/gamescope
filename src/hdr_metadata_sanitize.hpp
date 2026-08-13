#pragma once

#include <cstdint>
#include <optional>
#include <utility>

namespace gamescope::hdr_metadata
{
// wp_image_description_creator_params_v1 raises the fatal protocol error
// invalid_luminance when max L is not greater than min L, when max_cll or
// max_fall falls outside (min L, max L], or when max_fall exceeds max_cll.
// A fatal protocol error takes the host display down with it, so the app's
// HDR static metadata has to be checked before any of it is forwarded.
//
// Units follow the two definitions: hdr_metadata_infoframe carries min
// mastering luminance in 0.0001 cd/m2 and everything else in 1 cd/m2, and
// the protocol's min_lum argument is likewise cd/m2 * 10000 while max_lum,
// max_cll and max_fall are unscaled. Comparisons therefore scale the
// unscaled values by 10000 rather than dividing the minimum.

struct SanitizedHdrLuminance
{
	std::optional<std::pair<uint32_t, uint32_t>> masteringLuminance;
	std::optional<uint32_t> maxCll;
	std::optional<uint32_t> maxFall;
};

inline SanitizedHdrLuminance SanitizeHdrLuminance(
	uint32_t uMinMasteringLuminance,
	uint32_t uMaxMasteringLuminance,
	uint32_t uMaxCll,
	uint32_t uMaxFall )
{
	SanitizedHdrLuminance sanitized;

	if ( uint64_t( uMaxMasteringLuminance ) * 10000u <= uint64_t( uMinMasteringLuminance ) )
		return sanitized;

	sanitized.masteringLuminance = std::make_pair( uMinMasteringLuminance, uMaxMasteringLuminance );

	const auto IsWithinMasteringRange = [&]( uint32_t uLuminance )
	{
		return uint64_t( uLuminance ) * 10000u > uint64_t( uMinMasteringLuminance ) &&
			uLuminance <= uMaxMasteringLuminance;
	};

	if ( IsWithinMasteringRange( uMaxCll ) )
		sanitized.maxCll = uMaxCll;

	if ( IsWithinMasteringRange( uMaxFall ) && ( !sanitized.maxCll || uMaxFall <= *sanitized.maxCll ) )
		sanitized.maxFall = uMaxFall;

	return sanitized;
}
}
