#pragma once

#include <cstdint>
#include <optional>

namespace gamescope::xkb_modifiers
{
// Modifier and layout indices are private to a keymap, so state carries over
// to another keymap only through the names behind them. fnSourceName returns
// nullptr for an index the source does not define, and fnDestIndex returns an
// index at or above uDestCount for a name the destination lacks.
template <typename SourceName, typename DestIndex>
inline std::optional<uint32_t> TranslateIndex( uint32_t uIndex, SourceName &&fnSourceName, DestIndex &&fnDestIndex, uint32_t uDestCount )
{
	const char *pszName = fnSourceName( uIndex );
	if ( !pszName )
		return std::nullopt;

	const uint32_t uDest = fnDestIndex( pszName );
	if ( uDest >= uDestCount )
		return std::nullopt;

	return uDest;
}

// A modifier the destination lacks is dropped from the mask.
template <typename SourceName, typename DestIndex>
inline uint32_t TranslateMask( uint32_t uMask, SourceName &&fnSourceName, DestIndex &&fnDestIndex )
{
	uint32_t uResult = 0;
	for ( uint32_t i = 0; i < 32; i++ )
	{
		if ( !( uMask & ( 1u << i ) ) )
			continue;

		if ( std::optional<uint32_t> oDest = TranslateIndex( i, fnSourceName, fnDestIndex, 32 ) )
			uResult |= 1u << *oDest;
	}
	return uResult;
}
}
