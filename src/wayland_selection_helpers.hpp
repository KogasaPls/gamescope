#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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

// Offered on every source we publish besides the text types, so the host
// handing one of our selections back is told from a host copy by what the
// offer carries rather than by when it arrives, and the id says which of our
// sources it is. The nonce is drawn at random per process: two nested
// sessions on one host must not take each other's copies for their own, and a
// pid repeats across PID namespaces.
inline std::string SelectionSourceMarkerPrefix( uint64_t ulNonce )
{
	return std::string( "application/x-gamescope-selection-source;nonce=" ) + std::to_string( ulNonce ) + ";id=";
}

inline std::string SelectionSourceMarker( std::string_view sPrefix, uint32_t uSourceId )
{
	return std::string( sPrefix ) + std::to_string( uSourceId );
}

// The id of the source of ours whose marker the offer carries, or nullopt for
// a foreign offer. Everything after the prefix must be the id.
inline std::optional<uint32_t> SelectionSourceMarkerId( std::string_view sPrefix, const std::vector<std::string> &offered )
{
	for ( const std::string &sType : offered )
	{
		if ( !sType.starts_with( sPrefix ) )
			continue;

		const std::string_view sId = std::string_view( sType ).substr( sPrefix.size() );
		uint32_t uId = 0;
		const auto [ pEnd, ec ] = std::from_chars( sId.data(), sId.data() + sId.size(), uId );
		if ( ec == std::errc{} && pEnd == sId.data() + sId.size() )
			return uId;
	}

	return std::nullopt;
}

// Wayland serials wrap, so newer is less than half the range ahead. wlroots
// applies the same test to set_selection and refuses a serial older than that
// of the selection it would replace.
inline constexpr bool SerialIsNewer( uint32_t uSerial, uint32_t uThan )
{
	const uint32_t uDistance = uSerial - uThan;
	return uDistance != 0 && uDistance <= UINT32_MAX / 2;
}

// The sources handed to a seat device for one selection, until the host says
// what became of each. The seat may drop a set_selection without a word, so a
// source is live from its publish until the host cancels it, until the sync
// behind its publish comes back with no echo of it, or until an echo shows the
// host holding another of ours or a foreign offer shows it holding none. An
// offer that arrives while a sync is outstanding may have been queued before
// the host saw the request, so it is retained rather than acted on until the
// done says whether the request stood.
template <typename THandle>
class SeatSourceTracker
{
public:
	// A source went out with a sync behind it.
	void Publish( uint32_t uId, THandle handle )
	{
		m_Entries.push_back( Entry{ uId, handle, true } );
	}

	// The host handed back an offer carrying the marker of uId: it holds that
	// source and so none of the others, which it will never cancel; those not
	// still awaiting their sync are appended to destroy. Returns the held
	// handle, or nullopt for an id that is not a live source of ours, which
	// changes nothing.
	std::optional<THandle> OnEcho( uint32_t uId, std::vector<THandle> &destroy )
	{
		const auto it = std::find_if( m_Entries.begin(), m_Entries.end(), [ uId ]( const Entry &entry ) { return entry.uId == uId; } );
		if ( it == m_Entries.end() )
			return std::nullopt;

		const THandle held = it->handle;
		m_oHeld = held;
		std::erase_if( m_Entries, [ & ]( const Entry &entry )
		{
			if ( entry.handle == held || entry.bAwaitingSync )
				return false;

			destroy.push_back( entry.handle );
			return true;
		} );

		return held;
	}

	// The host cancelled handle, which the caller destroys.
	void OnCancelled( THandle handle )
	{
		std::erase_if( m_Entries, [ handle ]( const Entry &entry ) { return entry.handle == handle; } );
		if ( m_oHeld == handle )
			m_oHeld.reset();
	}

	// A foreign or null offer arrived. Returns true when it is to be retained
	// because a sync is outstanding. Otherwise the host holds none of ours:
	// every live handle is appended to destroy.
	bool OnForeign( std::vector<THandle> &destroy )
	{
		if ( SyncOutstanding() )
			return true;

		for ( const Entry &entry : m_Entries )
			destroy.push_back( entry.handle );
		m_Entries.clear();
		m_oHeld.reset();
		return false;
	}

	// The sync behind handle came back. Unless an echo has named it since its
	// publish the host refused it, and it is returned for destruction.
	std::optional<THandle> OnSyncDone( THandle handle )
	{
		const auto it = std::find_if( m_Entries.begin(), m_Entries.end(), [ handle ]( const Entry &entry ) { return entry.handle == handle; } );
		if ( it == m_Entries.end() )
			return std::nullopt;

		it->bAwaitingSync = false;
		if ( m_oHeld == handle )
			return std::nullopt;

		m_Entries.erase( it );
		return handle;
	}

	bool SyncOutstanding() const
	{
		return std::any_of( m_Entries.begin(), m_Entries.end(), []( const Entry &entry ) { return entry.bAwaitingSync; } );
	}

	std::optional<THandle> Held() const { return m_oHeld; }
	bool Empty() const { return m_Entries.empty(); }

private:
	struct Entry
	{
		uint32_t uId;
		THandle handle;
		bool bAwaitingSync;
	};

	std::vector<Entry> m_Entries;
	std::optional<THandle> m_oHeld;
};

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
