#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "wayland_selection_helpers.hpp"

namespace gamescope::x11_selection
{

// A snapshot of one selection entry, taken by the shell under its mutex.
struct SlotView
{
	std::string_view sContents;
	std::string_view sMimeType;
	// Zero until XFixes reports our own acquisition back to us. ICCCM forbids
	// CurrentTime as a selection timestamp, so zero advertises no TIMESTAMP at
	// all rather than answering with it.
	uint32_t uAcquisitionTime = 0;
};

enum class TargetKind { Targets, Timestamp, Text, Other };

enum class ActionKind { AnswerTargets, AnswerTimestamp, AnswerBytes, Refuse };

struct Action
{
	ActionKind eKind = ActionKind::Refuse;
	std::vector<size_t> targetIndices;   // AnswerTargets: indices into k_SupportedMimeTypes
	bool bIncludeTimestamp = false;      // AnswerTargets
	uint32_t uTime = 0;                  // AnswerTimestamp
	std::string sBytes;                  // AnswerBytes
	bool bUtf8Type = false;              // AnswerBytes

	bool operator==( const Action & ) const = default;
};

inline Action Refuse()
{
	return Action{};
}

// Decides one SelectionRequest against what we hold for its selection; the
// shell executes the result with Xlib.
inline Action OnRequest( TargetKind eKind, const char *pszTargetName, const SlotView &slot )
{
	const std::vector<std::string> offered = { std::string( slot.sMimeType ) };

	switch ( eKind )
	{
	case TargetKind::Targets:
	{
		Action action{ .eKind = ActionKind::AnswerTargets, .bIncludeTimestamp = slot.uAcquisitionTime != 0 };
		action.targetIndices = wayland_selection::TargetsForOffer( offered );
		return action;
	}
	case TargetKind::Timestamp:
		if ( slot.uAcquisitionTime == 0 )
			return Refuse();
		return Action{ .eKind = ActionKind::AnswerTimestamp, .uTime = slot.uAcquisitionTime };
	case TargetKind::Other:
		return Refuse();
	case TargetKind::Text:
		break;
	}

	if ( !wayland_selection::MimeTypeForTarget( pszTargetName, offered ) )
		return Refuse();

	const bool bAsciiFriendly = pszTargetName && ( !strcmp( pszTargetName, "STRING" ) || !strcmp( pszTargetName, "TEXT" ) );
	return Action{ .eKind = ActionKind::AnswerBytes, .sBytes = std::string( slot.sContents ),
		.bUtf8Type = wayland_selection::SelectionPropertyIsUtf8( bAsciiFriendly, slot.sMimeType, slot.sContents ) };
}

}
