#pragma once

#include <algorithm>
#include <cstdint>

// Whether we hold an X selection and the server time we took it at, as one
// word per selection per Xwayland server: X timestamps are CARD32, so both fit
// in one atomic and a reader cannot see the flag of one acquisition against the
// time of another. Bits 0-31 are the time, zero while unknown; bit 32 says the
// selection is ours; bit 33 asks for a release wanted while the time was
// unknown; the bits above count our acquisitions XFixes has yet to report. The
// time comes from the last of those reports: an earlier one names an
// acquisition the server has since replaced, and a release stamped with it
// would be ignored. Zero is "not ours".
namespace gamescope::x11_selection::ownership
{
inline constexpr uint64_t k_ulTimeMask = 0xffffffffull;
inline constexpr uint64_t k_ulOwned = 1ull << 32;
inline constexpr uint64_t k_ulReleasePending = 1ull << 33;
inline constexpr unsigned k_uUnreportedShift = 34;
inline constexpr uint64_t k_ulUnreportedMax = ~0ull >> k_uUnreportedShift;

inline constexpr bool IsOwned( uint64_t ulWord )
{
	return ulWord & k_ulOwned;
}

// Zero until the last outstanding report has arrived.
inline constexpr uint32_t AcquisitionTime( uint64_t ulWord )
{
	return uint32_t( ulWord & k_ulTimeMask );
}

inline constexpr uint64_t UnreportedAcquisitions( uint64_t ulWord )
{
	return ulWord >> k_uUnreportedShift;
}

// A SetSelectionOwner to our window is about to go out: ours, with the time
// unknown until this acquisition is reported. A release wanted before it is
// superseded.
inline constexpr uint64_t Acquire( uint64_t ulWord )
{
	const uint64_t ulUnreported = std::min( UnreportedAcquisitions( ulWord ) + 1, k_ulUnreportedMax );
	return k_ulOwned | ( ulUnreported << k_uUnreportedShift );
}

enum class ReleaseAction
{
	NotOurs,
	// The time is unknown, so the report that brings it makes the release.
	Deferred,
	Release,
};

struct ReleaseStep
{
	uint64_t ulWord = 0;
	ReleaseAction eAction = ReleaseAction::NotOurs;
	uint32_t uTime = 0;

	bool operator==( const ReleaseStep & ) const = default;
};

inline constexpr ReleaseStep Release( uint64_t ulWord )
{
	if ( !IsOwned( ulWord ) )
		return ReleaseStep{};

	if ( AcquisitionTime( ulWord ) == 0 )
		return ReleaseStep{ .ulWord = ulWord | k_ulReleasePending, .eAction = ReleaseAction::Deferred };

	return ReleaseStep{ .eAction = ReleaseAction::Release, .uTime = AcquisitionTime( ulWord ) };
}

struct ReportStep
{
	uint64_t ulWord = 0;
	// Release with the reported time: the report brought the time a release
	// waited for.
	bool bRelease = false;

	bool operator==( const ReportStep & ) const = default;
};

// XFixes reported an acquisition of ours at uTime. With later acquisitions
// still unreported it is not the one the server holds, so only the count
// moves.
inline constexpr ReportStep Reported( uint64_t ulWord, uint32_t uTime )
{
	const uint64_t ulUnreported = UnreportedAcquisitions( ulWord );
	if ( ulUnreported > 1 )
		return ReportStep{ .ulWord = ( ulWord & ~( k_ulUnreportedMax << k_uUnreportedShift ) ) | ( ( ulUnreported - 1 ) << k_uUnreportedShift ) };

	if ( ulWord & k_ulReleasePending )
		return ReportStep{ .bRelease = true };

	return ReportStep{ .ulWord = k_ulOwned | uTime };
}
}
