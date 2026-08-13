#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gamescope::wayland_selection
{
inline constexpr const char *k_szUtf8MimeType = "text/plain;charset=utf-8";

// MIME types we advertise and accept, in order of preference.
inline constexpr std::array<const char *, 5> k_SupportedMimeTypes = {
	k_szUtf8MimeType, "UTF8_STRING", "text/plain", "STRING", "TEXT"
};

// X selection targets that name a text conversion without naming an encoding.
// UTF8_STRING is not among them: it names UTF-8, so bytes of any other type
// must not be served under it. COMPOUND_TEXT is not either: we have no
// compound text encoder.
inline constexpr std::array<const char *, 2> k_UntypedTextTargets = {
	"TEXT", "STRING"
};

// A parked conversion rides on a running read only if the read answers the
// same announcement: bytes fetched for an older epoch are dropped on delivery.
inline bool SelectionReadServes( bool bEager, int nReadSelection, std::string_view sReadMime, uint64_t ulReadEpoch,
	int nWantSelection, std::string_view sWantMime, uint64_t ulWantEpoch )
{
	return !bEager && nReadSelection == nWantSelection && sReadMime == sWantMime && ulReadEpoch == ulWantEpoch;
}

inline constexpr size_t IndexOfMimeType( std::string_view name )
{
	for ( size_t i = 0; i < k_SupportedMimeTypes.size(); i++ )
	{
		if ( std::string_view( k_SupportedMimeTypes[i] ) == name )
			return i;
	}

	return k_SupportedMimeTypes.size();
}

inline constexpr size_t k_uMimeTypeUtf8String = IndexOfMimeType( "UTF8_STRING" );
inline constexpr size_t k_uMimeTypeString = IndexOfMimeType( "STRING" );
inline constexpr size_t k_uMimeTypeText = IndexOfMimeType( "TEXT" );

inline constexpr size_t k_uMimeTypeUtf8Text = IndexOfMimeType( k_szUtf8MimeType );

static_assert( k_uMimeTypeUtf8String < k_SupportedMimeTypes.size() );
static_assert( k_uMimeTypeString < k_SupportedMimeTypes.size() );
static_assert( k_uMimeTypeText < k_SupportedMimeTypes.size() );
static_assert( k_uMimeTypeUtf8Text < k_SupportedMimeTypes.size() );

// The types whose bytes are UTF-8, in preference order.
inline constexpr std::array<const char *, 2> k_Utf8MimeTypes = {
	k_SupportedMimeTypes[k_uMimeTypeUtf8Text], k_SupportedMimeTypes[k_uMimeTypeUtf8String]
};

// Whether pszTarget names one of the MIME types we serve, so bytes we already
// hold can answer a conversion to it.
inline bool IsSupportedMimeType( const char *pszTarget )
{
	if ( !pszTarget )
		return false;

	return std::any_of( k_SupportedMimeTypes.begin(), k_SupportedMimeTypes.end(),
		[ pszTarget ]( const char *pszName ) { return !strcmp( pszName, pszTarget ); } );
}

// ASCII is the subset UTF-8 and ISO 8859-1 agree on, so bytes that stay inside
// it can be served under either name.
inline bool IsAsciiOnly( std::string_view data )
{
	return std::all_of( data.begin(), data.end(),
		[]( char ch ) { return static_cast<unsigned char>( ch ) < 0x80; } );
}

// Whether bytes of this MIME type are UTF-8. ICCCM defines STRING as ISO
// 8859-1, so UTF-8 answered under any target must be typed UTF8_STRING or a
// non-ASCII paste is mangled.
inline bool IsUtf8MimeType( std::string_view name )
{
	return name == k_szUtf8MimeType || name == "UTF8_STRING";
}

// ICCCM: the property type names the encoding of the bytes, not the target that
// was asked for. STRING is ISO 8859-1, which ASCII is a subset of, so a requestor
// that asked for STRING or TEXT and discards any other type is still served when
// the bytes stay inside it.
inline bool SelectionPropertyIsUtf8( bool bAsciiFriendlyTarget, std::string_view sMimeType, std::string_view sData )
{
	if ( bAsciiFriendlyTarget && IsAsciiOnly( sData ) )
		return false;

	return IsUtf8MimeType( sMimeType );
}

// The first of our types the offer carries, in our preference order, or
// nullptr. The returned pointer is the entry of `supported`.
inline const char *FirstSupportedMimeType( std::span<const char *const> supported, const std::vector<std::string> &offered )
{
	for ( const char *pMimeType : supported )
	{
		if ( std::find( offered.begin(), offered.end(), pMimeType ) != offered.end() )
			return pMimeType;
	}

	return nullptr;
}

// The selection targets to answer TARGETS with, as indices into
// k_SupportedMimeTypes in our preference order: every one of our types the
// host actually offers, plus the untyped text targets, which any text offer
// serves, plus UTF8_STRING when one of the offered types is UTF-8. The caller adds TARGETS and TIMESTAMP, which are not MIME types.
// MULTIPLE is not served and so is not listed.
inline std::vector<size_t> TargetsForOffer( const std::vector<std::string> &offered )
{
	std::array<bool, k_SupportedMimeTypes.size()> advertised = {};

	for ( size_t i = 0; i < k_SupportedMimeTypes.size(); i++ )
		advertised[i] = std::find( offered.begin(), offered.end(), k_SupportedMimeTypes[i] ) != offered.end();

	if ( std::find( advertised.begin(), advertised.end(), true ) != advertised.end() )
	{
		advertised[k_uMimeTypeString] = true;
		advertised[k_uMimeTypeText] = true;
	}

	if ( advertised[k_uMimeTypeUtf8Text] )
		advertised[k_uMimeTypeUtf8String] = true;

	std::vector<size_t> targets;
	for ( size_t i = 0; i < advertised.size(); i++ )
	{
		if ( advertised[i] )
			targets.push_back( i );
	}

	return targets;
}

// The MIME type to ask the host for to answer a conversion to pszTarget, or
// nullptr when we serve nothing for it. An untyped text target resolves to the
// host's best text offer, so a host offering only text/plain;charset=utf-8
// still serves a Wine or Xt client asking for STRING.
inline const char *MimeTypeForTarget( const char *pszTarget, const std::vector<std::string> &offered )
{
	if ( !pszTarget )
		return nullptr;

	auto fnNamed = [ pszTarget ]( const char *pszName ) { return !strcmp( pszName, pszTarget ); };

	for ( const char *pMimeType : k_SupportedMimeTypes )
	{
		if ( fnNamed( pMimeType ) && std::find( offered.begin(), offered.end(), pMimeType ) != offered.end() )
			return pMimeType;
	}

	// A target that names UTF-8 is served only by a type that is UTF-8.
	if ( IsUtf8MimeType( pszTarget ) )
		return FirstSupportedMimeType( k_Utf8MimeTypes, offered );

	const bool bTextTarget =
		std::any_of( k_UntypedTextTargets.begin(), k_UntypedTextTargets.end(), fnNamed ) ||
		IsSupportedMimeType( pszTarget );

	if ( !bTextTarget )
		return nullptr;

	return FirstSupportedMimeType( k_SupportedMimeTypes, offered );
}
}
