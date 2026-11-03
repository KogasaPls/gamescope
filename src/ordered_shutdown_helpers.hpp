#pragma once

#include <algorithm>
#include <cstdint>

namespace gamescope::ordered_shutdown
{
inline constexpr uint64_t k_ulChildPollNanos = 250'000'000ul;

enum class Phase
{
	Running,
	WaitingForChildren,
	Done,
};

enum class Step
{
	Wait,
	SignalChildren,
	Shutdown,
	ForceShutdown,
	GraceExpired,
};

struct State
{
	Phase ePhase = Phase::Running;
	uint64_t ulDeadlineNanos = 0;
};

inline constexpr uint64_t GraceNanosFromMs( int nMs )
{
	return nMs > 0 ? uint64_t( nMs ) * 1'000'000ul : 0;
}

// No grace period is upstream's immediate shutdown, which signals the
// children only once the display is gone.
inline Step OnTerminatingSignal( State &state, bool bHasChildren, uint64_t ulNowNanos, uint64_t ulGraceNanos )
{
	switch ( state.ePhase )
	{
	case Phase::Running:
		if ( !bHasChildren || !ulGraceNanos )
		{
			state.ePhase = Phase::Done;
			return Step::Shutdown;
		}
		state.ePhase = Phase::WaitingForChildren;
		state.ulDeadlineNanos = ulNowNanos + ulGraceNanos;
		return Step::SignalChildren;
	case Phase::WaitingForChildren:
		state.ePhase = Phase::Done;
		return Step::ForceShutdown;
	case Phase::Done:
		break;
	}
	return Step::Wait;
}

inline Step OnChildrenChecked( State &state, bool bHasChildren, uint64_t ulNowNanos )
{
	if ( state.ePhase != Phase::WaitingForChildren )
		return Step::Wait;

	if ( !bHasChildren )
	{
		state.ePhase = Phase::Done;
		return Step::Shutdown;
	}

	if ( ulNowNanos >= state.ulDeadlineNanos )
	{
		state.ePhase = Phase::Done;
		return Step::GraceExpired;
	}

	return Step::Wait;
}

// Rounded up, so the wake-up for the deadline does not land just short of it.
inline int PollTimeoutMs( const State &state, uint64_t ulNowNanos )
{
	if ( state.ePhase != Phase::WaitingForChildren )
		return -1;

	const uint64_t ulRemainingNanos = state.ulDeadlineNanos > ulNowNanos ? state.ulDeadlineNanos - ulNowNanos : 0;
	const uint64_t ulWaitNanos = std::min( ulRemainingNanos, k_ulChildPollNanos );
	return int( ( ulWaitNanos + 999'999ul ) / 1'000'000ul );
}
}
