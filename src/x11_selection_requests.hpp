#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "wayland_selection_helpers.hpp"

namespace gamescope::x11_selection
{

// X handles are opaque here. pCtx identifies the Xwayland server; the shell
// checks it is still alive before answering, since a server can be destroyed
// while its request is parked.
struct RequestId
{
	void *pCtx = nullptr;
	uint64_t ulRequestor = 0;
	uint64_t ulSelection = 0;
	uint64_t ulTarget = 0;
	uint64_t ulProperty = 0;
	uint64_t ulTime = 0;

	bool operator==( const RequestId & ) const = default;
};

// A snapshot of one selection entry, taken by the shell under its mutex.
struct SlotView
{
	std::string_view sContents;
	std::string_view sMimeType;
	std::span<const std::string> hostMimeTypes;
	bool bLazy = false;
	uint64_t ulEpoch = 0;
	// Zero until XFixes reports our own acquisition back to us. ICCCM forbids
	// CurrentTime as a selection timestamp, so zero advertises no TIMESTAMP at
	// all rather than answering with it.
	uint32_t uAcquisitionTime = 0;
};

enum class TargetKind { Targets, Timestamp, Text, Other };

struct ParkedRequest
{
	RequestId id;
	int nSelection = 0;
	std::string sMimeType;
	uint64_t ulEpoch = 0;
	uint64_t ulDeadlineNanos = 0;
	bool bAsciiFriendlyTarget = false;
};

// One queue for both selections, so the cap is global.
struct RequestQueue
{
	static constexpr size_t k_uMaxParked = 16;
	std::vector<ParkedRequest> parked;
};

enum class ActionKind { AnswerTargets, AnswerTimestamp, AnswerBytes, Refuse, QueueFetch };

struct Action
{
	ActionKind eKind = ActionKind::Refuse;
	RequestId id;
	std::vector<size_t> targetIndices;   // AnswerTargets: indices into k_SupportedMimeTypes
	bool bIncludeTimestamp = false;      // AnswerTargets
	uint32_t uTime = 0;                  // AnswerTimestamp
	std::string sBytes;                  // AnswerBytes
	bool bUtf8Type = false;              // AnswerBytes
	std::string sMimeType;               // QueueFetch
	int nSelection = 0;                  // QueueFetch
	uint64_t ulEpoch = 0;                // QueueFetch

	bool operator==( const Action & ) const = default;
};

inline Action Refuse( const RequestId &id )
{
	return Action{ .eKind = ActionKind::Refuse, .id = id };
}

// Decides one SelectionRequest. A lazy slot with a serving host type parks the
// request and asks for one fetch unless a parked request already waits on that
// type at the same epoch; the shell calls OnFetchQueueFailed when the backend
// refuses the fetch.
inline std::vector<Action> OnRequest( RequestQueue &queue, const RequestId &id, int nSelection, TargetKind eKind,
	const char *pszTargetName, const SlotView &slot, uint64_t ulDeadlineNanos )
{
	std::vector<Action> actions;
	const std::vector<std::string> offered = slot.bLazy
		? std::vector<std::string>( slot.hostMimeTypes.begin(), slot.hostMimeTypes.end() )
		: std::vector<std::string>{ std::string( slot.sMimeType ) };

	switch ( eKind )
	{
	case TargetKind::Targets:
	{
		Action action{ .eKind = ActionKind::AnswerTargets, .id = id, .bIncludeTimestamp = slot.uAcquisitionTime != 0 };
		for ( size_t uIndex : wayland_selection::TargetsForOffer( offered ) )
			action.targetIndices.push_back( uIndex );
		actions.push_back( std::move( action ) );
		return actions;
	}
	case TargetKind::Timestamp:
		if ( slot.uAcquisitionTime == 0 )
			actions.push_back( Refuse( id ) );
		else
			actions.push_back( Action{ .eKind = ActionKind::AnswerTimestamp, .id = id, .uTime = slot.uAcquisitionTime } );
		return actions;
	case TargetKind::Other:
		actions.push_back( Refuse( id ) );
		return actions;
	case TargetKind::Text:
		break;
	}

	const char *pszMimeType = wayland_selection::MimeTypeForTarget( pszTargetName, offered );
	if ( !pszMimeType )
	{
		actions.push_back( Refuse( id ) );
		return actions;
	}

	const bool bAsciiFriendly = pszTargetName && ( !strcmp( pszTargetName, "STRING" ) || !strcmp( pszTargetName, "TEXT" ) );

	if ( !slot.bLazy )
	{
		actions.push_back( Action{ .eKind = ActionKind::AnswerBytes, .id = id, .sBytes = std::string( slot.sContents ),
			.bUtf8Type = wayland_selection::SelectionPropertyIsUtf8( bAsciiFriendly, slot.sMimeType, slot.sContents ) } );
		return actions;
	}

	if ( queue.parked.size() >= RequestQueue::k_uMaxParked )
	{
		actions.push_back( Refuse( id ) );
		return actions;
	}

	const bool bFetchRunning = std::any_of( queue.parked.begin(), queue.parked.end(),
		[ & ]( const ParkedRequest &p ) { return p.nSelection == nSelection && p.sMimeType == pszMimeType && p.ulEpoch == slot.ulEpoch; } );
	queue.parked.push_back( ParkedRequest{ id, nSelection, pszMimeType, slot.ulEpoch, ulDeadlineNanos, bAsciiFriendly } );
	if ( !bFetchRunning )
		actions.push_back( Action{ .eKind = ActionKind::QueueFetch, .id = id, .sMimeType = pszMimeType, .nSelection = nSelection, .ulEpoch = slot.ulEpoch } );
	return actions;
}

// The backend refused the fetch a request was parked for: refuse it now.
inline std::vector<Action> OnFetchQueueFailed( RequestQueue &queue, const RequestId &id )
{
	std::vector<Action> actions;
	std::erase_if( queue.parked, [ & ]( const ParkedRequest &p )
	{
		if ( !( p.id == id ) )
			return false;
		actions.push_back( Refuse( id ) );
		return true;
	} );
	return actions;
}

// Bytes (or failure) arrived for one fetch. A parked request answers only when
// the fetch was made against the announcement the slot still carries.
inline std::vector<Action> OnFetchDelivered( RequestQueue &queue, int nSelection, std::string_view sMimeType,
	std::string_view sBytes, bool bSuccess, uint64_t ulFetchEpoch, uint64_t ulCurrentEpoch )
{
	std::vector<Action> actions;
	std::erase_if( queue.parked, [ & ]( const ParkedRequest &p )
	{
		if ( p.nSelection != nSelection )
			return false;
		if ( p.ulEpoch != ulCurrentEpoch )
		{
			actions.push_back( Refuse( p.id ) );
			return true;
		}
		if ( p.sMimeType != sMimeType || p.ulEpoch != ulFetchEpoch )
			return false;
		if ( bSuccess )
			actions.push_back( Action{ .eKind = ActionKind::AnswerBytes, .id = p.id, .sBytes = std::string( sBytes ),
				.bUtf8Type = wayland_selection::SelectionPropertyIsUtf8( p.bAsciiFriendlyTarget, sMimeType, sBytes ) } );
		else
			actions.push_back( Refuse( p.id ) );
		return true;
	} );
	return actions;
}

// Refuses parked requests the host never answered in time, and those parked
// against an announcement that has since been replaced. A request parked at
// an epoch the slot has since left is refused here as well.
inline std::vector<Action> OnTick( RequestQueue &queue, uint64_t ulNowNanos, std::span<const uint64_t> currentEpochs )
{
	std::vector<Action> actions;
	std::erase_if( queue.parked, [ & ]( const ParkedRequest &p )
	{
		const bool bStale = size_t( p.nSelection ) < currentEpochs.size() && p.ulEpoch != currentEpochs[p.nSelection];
		if ( !bStale && ulNowNanos < p.ulDeadlineNanos )
			return false;
		actions.push_back( Refuse( p.id ) );
		return true;
	} );
	return actions;
}

}
