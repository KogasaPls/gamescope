#pragma once

#include <cstdint>

namespace gamescope::empty_exit
{
inline constexpr uint64_t k_ulDefaultGraceNanos = 30'000'000'000ul;

// The flag itself is the opt-in, so a value of zero or none at all asks for the
// default grace period rather than for no exit.
inline constexpr uint64_t GraceNanosFromSeconds( int nSeconds )
{
	return nSeconds > 0 ? uint64_t( nSeconds ) * 1'000'000'000ul : k_ulDefaultGraceNanos;
}

struct State
{
	bool bHasShownWindow = false;
	uint64_t ulEmptySinceNanos = 0;
};

// An empty session before the first window is one that has not started rather
// than one that has ended, so the clock only runs after one has been shown.
inline bool UpdateEmpty( State &state, bool bWindowShown, uint64_t ulNowNanos, uint64_t ulGraceNanos )
{
	if ( bWindowShown )
	{
		state.bHasShownWindow = true;
		state.ulEmptySinceNanos = 0;
		return false;
	}

	if ( !state.bHasShownWindow || !ulGraceNanos )
		return false;

	if ( !state.ulEmptySinceNanos )
		state.ulEmptySinceNanos = ulNowNanos;

	return ulNowNanos >= state.ulEmptySinceNanos &&
	       ulNowNanos - state.ulEmptySinceNanos >= ulGraceNanos;
}
}
