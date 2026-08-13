#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

#include "wayland_selection_helpers.hpp"

namespace
{
using gamescope::wayland_selection::FirstSupportedMimeType;

constexpr std::array<const char *, 3> k_Supported = { "text/plain;charset=utf-8", "UTF8_STRING", "text/plain" };
}

TEST_CASE( "The first supported type follows our preference order, not the offer's", "[wayland_selection]" )
{
	const char *pMatch = FirstSupportedMimeType( k_Supported, { "text/plain", "image/png", "UTF8_STRING" } );
	REQUIRE( pMatch == k_Supported[1] );

	REQUIRE( FirstSupportedMimeType( k_Supported, { "text/plain;charset=utf-8" } ) == k_Supported[0] );
	REQUIRE( FirstSupportedMimeType( k_Supported, { "image/png", "text/html" } ) == nullptr );
	REQUIRE( FirstSupportedMimeType( k_Supported, {} ) == nullptr );

	REQUIRE( FirstSupportedMimeType( k_Supported, { "application/octet-stream", "text/plain" } ) == k_Supported[2] );
}

TEST_CASE( "TARGETS lists our types the host offers plus the text targets we serve", "[wayland_selection]" )
{
	using gamescope::wayland_selection::TargetsForOffer;
	using gamescope::wayland_selection::k_SupportedMimeTypes;
	using gamescope::wayland_selection::k_uMimeTypeUtf8String;
	using gamescope::wayland_selection::k_uMimeTypeString;
	using gamescope::wayland_selection::k_uMimeTypeText;

	const std::vector<size_t> targets = TargetsForOffer( { "TEXT", "image/png", "UTF8_STRING" } );
	REQUIRE( targets == std::vector<size_t>{ k_uMimeTypeUtf8String, k_uMimeTypeString, k_uMimeTypeText } );
	REQUIRE( std::string( k_SupportedMimeTypes[ targets[0] ] ) == "UTF8_STRING" );

	// A conversion to any of these is served from a text/plain;charset=utf-8
	// offer, so a toolkit picking strictly from TARGETS has something to pick.
	REQUIRE( TargetsForOffer( { "text/plain;charset=utf-8" } ) ==
		std::vector<size_t>{ 0, k_uMimeTypeUtf8String, k_uMimeTypeString, k_uMimeTypeText } );

	REQUIRE( TargetsForOffer( { "image/png" } ).empty() );
	REQUIRE( TargetsForOffer( {} ).empty() );
}

TEST_CASE( "An untyped text target falls back to the host's best text offer", "[wayland_selection]" )
{
	using gamescope::wayland_selection::MimeTypeForTarget;
	using gamescope::wayland_selection::k_SupportedMimeTypes;

	const std::vector<std::string> utf8Only = { "text/plain;charset=utf-8" };

	REQUIRE( MimeTypeForTarget( "STRING", utf8Only ) == k_SupportedMimeTypes[0] );
	REQUIRE( MimeTypeForTarget( "TEXT", utf8Only ) == k_SupportedMimeTypes[0] );
	REQUIRE( MimeTypeForTarget( "UTF8_STRING", utf8Only ) == k_SupportedMimeTypes[0] );
	REQUIRE( MimeTypeForTarget( "text/plain", utf8Only ) == k_SupportedMimeTypes[0] );
}

TEST_CASE( "A target the host offers by name is asked for by that name", "[wayland_selection]" )
{
	using gamescope::wayland_selection::MimeTypeForTarget;
	using gamescope::wayland_selection::k_SupportedMimeTypes;

	const std::vector<std::string> both = { "text/plain;charset=utf-8", "STRING" };

	REQUIRE( MimeTypeForTarget( "STRING", both ) == k_SupportedMimeTypes[3] );
	REQUIRE( MimeTypeForTarget( "text/plain;charset=utf-8", both ) == k_SupportedMimeTypes[0] );
}

TEST_CASE( "A non-text target and an offer with no text are not served", "[wayland_selection]" )
{
	using gamescope::wayland_selection::MimeTypeForTarget;

	REQUIRE( MimeTypeForTarget( "image/png", { "image/png" } ) == nullptr );
	// We have no compound text encoder, so we do not claim to serve one.
	REQUIRE( MimeTypeForTarget( "COMPOUND_TEXT", { "text/plain;charset=utf-8" } ) == nullptr );
	REQUIRE( MimeTypeForTarget( "TARGETS", { "text/plain" } ) == nullptr );
	REQUIRE( MimeTypeForTarget( nullptr, { "text/plain" } ) == nullptr );
	REQUIRE( MimeTypeForTarget( "STRING", { "image/png" } ) == nullptr );
	REQUIRE( MimeTypeForTarget( "STRING", {} ) == nullptr );
}

TEST_CASE( "An offer with no UTF-8 type neither advertises nor serves UTF8_STRING", "[wayland_selection]" )
{
	using gamescope::wayland_selection::MimeTypeForTarget;
	using gamescope::wayland_selection::TargetsForOffer;
	using gamescope::wayland_selection::k_SupportedMimeTypes;
	using gamescope::wayland_selection::k_uMimeTypeString;
	using gamescope::wayland_selection::k_uMimeTypeText;

	const std::vector<std::string> latin1Only = { "STRING" };

	REQUIRE( TargetsForOffer( latin1Only ) == std::vector<size_t>{ k_uMimeTypeString, k_uMimeTypeText } );
	REQUIRE( MimeTypeForTarget( "UTF8_STRING", latin1Only ) == nullptr );
	REQUIRE( MimeTypeForTarget( "text/plain;charset=utf-8", latin1Only ) == nullptr );
	REQUIRE( MimeTypeForTarget( "TEXT", latin1Only ) == k_SupportedMimeTypes[k_uMimeTypeString] );

	// text/plain leaves the encoding to the sender, so it cannot answer a
	// target that names UTF-8 either.
	const std::vector<std::string> plainOnly = { "text/plain" };

	REQUIRE( TargetsForOffer( plainOnly ) == std::vector<size_t>{ 2, k_uMimeTypeString, k_uMimeTypeText } );
	REQUIRE( MimeTypeForTarget( "UTF8_STRING", plainOnly ) == nullptr );
	REQUIRE( MimeTypeForTarget( "STRING", plainOnly ) == k_SupportedMimeTypes[2] );
}

TEST_CASE( "ASCII is the subset both encodings agree on", "[wayland_selection]" )
{
	using gamescope::wayland_selection::IsAsciiOnly;

	REQUIRE( IsAsciiOnly( "" ) );
	REQUIRE( IsAsciiOnly( "plain text\n" ) );
	REQUIRE( !IsAsciiOnly( "caf\xc3\xa9" ) );
	REQUIRE( !IsAsciiOnly( std::string( 1, char( 0x80 ) ) ) );
}

TEST_CASE( "Only the UTF-8 MIME types are typed UTF8_STRING", "[wayland_selection]" )
{
	using gamescope::wayland_selection::IsUtf8MimeType;

	REQUIRE( IsUtf8MimeType( "text/plain;charset=utf-8" ) );
	REQUIRE( IsUtf8MimeType( "UTF8_STRING" ) );

	// STRING is ISO 8859-1, and TEXT leaves the encoding to the owner.
	REQUIRE( !IsUtf8MimeType( "STRING" ) );
	REQUIRE( !IsUtf8MimeType( "TEXT" ) );
	REQUIRE( !IsUtf8MimeType( "text/plain" ) );
	REQUIRE( !IsUtf8MimeType( "" ) );
}

TEST_CASE( "The property type follows the bytes, with ASCII under STRING or TEXT typed STRING", "[wayland_selection]" )
{
	using gamescope::wayland_selection::SelectionPropertyIsUtf8;

	REQUIRE_FALSE( SelectionPropertyIsUtf8( true, "text/plain;charset=utf-8", "plain ascii" ) );
	REQUIRE( SelectionPropertyIsUtf8( false, "text/plain;charset=utf-8", "plain ascii" ) );
	REQUIRE( SelectionPropertyIsUtf8( true, "text/plain;charset=utf-8", "caf\xc3\xa9" ) );
	REQUIRE_FALSE( SelectionPropertyIsUtf8( false, "text/plain", "anything" ) );
}

TEST_CASE( "The source marker names the process and the source and is not a text type", "[wayland_selection]" )
{
	using gamescope::wayland_selection::IsSupportedMimeType;
	using gamescope::wayland_selection::SelectionSourceMarker;
	using gamescope::wayland_selection::SelectionSourceMarkerId;
	using gamescope::wayland_selection::SelectionSourceMarkerPrefix;
	using gamescope::wayland_selection::k_SupportedMimeTypes;

	const std::string sPrefix = SelectionSourceMarkerPrefix( 0xfeedfacecafef00dull );
	const std::string sMarker = SelectionSourceMarker( sPrefix, 7 );
	REQUIRE( sMarker == "application/x-gamescope-selection-source;nonce=18369614221190033421;id=7" );
	REQUIRE( sMarker != SelectionSourceMarker( sPrefix, 8 ) );
	REQUIRE( sMarker != SelectionSourceMarker( SelectionSourceMarkerPrefix( 1 ), 7 ) );

	// An echo names the source it carries the marker of; another process's
	// marker and anything that is not exactly an id are foreign.
	REQUIRE( SelectionSourceMarkerId( sPrefix, { "text/plain", sMarker } ) == 7 );
	REQUIRE( SelectionSourceMarkerId( sPrefix, { SelectionSourceMarker( sPrefix, 4294967295u ) } ) == 4294967295u );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, { "text/plain" } ).has_value() );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, { SelectionSourceMarker( SelectionSourceMarkerPrefix( 1 ), 7 ) } ).has_value() );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, { sPrefix } ).has_value() );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, { sMarker + "x" } ).has_value() );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, { sPrefix + "99999999999" } ).has_value() );
	REQUIRE_FALSE( SelectionSourceMarkerId( sPrefix, {} ).has_value() );

	// An offer that carries only the marker has nothing we read, and the marker
	// is never served as an X target.
	REQUIRE_FALSE( IsSupportedMimeType( sMarker.c_str() ) );
	REQUIRE( FirstSupportedMimeType( k_SupportedMimeTypes, { sMarker } ) == nullptr );
	REQUIRE( FirstSupportedMimeType( k_SupportedMimeTypes, { sMarker, "text/plain" } ) == k_SupportedMimeTypes[2] );
}

TEST_CASE( "A newer serial is less than half the range ahead", "[wayland_selection]" )
{
	using gamescope::wayland_selection::SerialIsNewer;

	REQUIRE( SerialIsNewer( 11, 10 ) );
	REQUIRE_FALSE( SerialIsNewer( 10, 11 ) );
	REQUIRE_FALSE( SerialIsNewer( 10, 10 ) );
	REQUIRE( SerialIsNewer( 2, 0xfffffffeu ) );
	REQUIRE_FALSE( SerialIsNewer( 0xfffffffeu, 2 ) );
	REQUIRE( SerialIsNewer( 0x7fffffffu, 0 ) );
	REQUIRE_FALSE( SerialIsNewer( 0x80000000u, 0 ) );
}

namespace
{
using Tracker = gamescope::wayland_selection::SeatSourceTracker<int>;
}

TEST_CASE( "A seat source the host echoes stands and its sync destroys nothing", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	tracker.Publish( 1, 10 );
	REQUIRE( tracker.SyncOutstanding() );

	// The host re-sent its selection before it saw our request: retained.
	REQUIRE( tracker.OnForeign( destroy ) );
	REQUIRE( destroy.empty() );

	REQUIRE( tracker.OnEcho( 1, destroy ) == 10 );
	REQUIRE( tracker.Held() == 10 );
	REQUIRE_FALSE( tracker.OnSyncDone( 10 ).has_value() );
	REQUIRE_FALSE( tracker.SyncOutstanding() );
	REQUIRE( tracker.Held() == 10 );
	REQUIRE( destroy.empty() );
}

TEST_CASE( "A seat source the host never echoes is refused and destroyed at its sync", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	tracker.Publish( 1, 10 );
	REQUIRE( tracker.OnForeign( destroy ) );
	REQUIRE( tracker.OnSyncDone( 10 ) == 10 );
	REQUIRE_FALSE( tracker.Held().has_value() );
	REQUIRE( tracker.Empty() );

	// With nothing in flight the foreign offer is the host's selection.
	REQUIRE_FALSE( tracker.OnForeign( destroy ) );
	REQUIRE( destroy.empty() );
}

TEST_CASE( "A refused replacement leaves the held seat source alone and does not leak", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	tracker.Publish( 1, 10 );
	REQUIRE( tracker.OnEcho( 1, destroy ) == 10 );
	REQUIRE_FALSE( tracker.OnSyncDone( 10 ).has_value() );

	tracker.Publish( 2, 20 );
	REQUIRE( tracker.SyncOutstanding() );
	REQUIRE( tracker.OnSyncDone( 20 ) == 20 );
	REQUIRE( tracker.Held() == 10 );
	REQUIRE_FALSE( tracker.SyncOutstanding() );
	REQUIRE( destroy.empty() );
}

TEST_CASE( "An accepted replacement is preceded by the cancel of the source it replaces", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	tracker.Publish( 1, 10 );
	REQUIRE( tracker.OnEcho( 1, destroy ) == 10 );
	REQUIRE_FALSE( tracker.OnSyncDone( 10 ).has_value() );

	tracker.Publish( 2, 20 );
	tracker.OnCancelled( 10 );
	REQUIRE_FALSE( tracker.Held().has_value() );
	REQUIRE( tracker.OnEcho( 2, destroy ) == 20 );
	REQUIRE_FALSE( tracker.OnSyncDone( 20 ).has_value() );
	REQUIRE( tracker.Held() == 20 );
	REQUIRE( destroy.empty() );
}

TEST_CASE( "An echo destroys the live seat sources the host does not hold, once their syncs are in", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	// A host that sends no cancelled when it switches between our sources.
	tracker.Publish( 1, 10 );
	REQUIRE( tracker.OnEcho( 1, destroy ) == 10 );
	REQUIRE_FALSE( tracker.OnSyncDone( 10 ).has_value() );
	tracker.Publish( 2, 20 );
	tracker.Publish( 3, 30 );
	REQUIRE( tracker.OnEcho( 3, destroy ) == 30 );
	// 10 is not held and not awaiting; 20 awaits its own sync.
	REQUIRE( destroy == std::vector<int>{ 10 } );
	REQUIRE( tracker.OnSyncDone( 20 ) == 20 );
	REQUIRE_FALSE( tracker.OnSyncDone( 30 ).has_value() );
	REQUIRE( tracker.Held() == 30 );

	// An echo for an id that is not ours changes nothing.
	destroy.clear();
	REQUIRE_FALSE( tracker.OnEcho( 99, destroy ).has_value() );
	REQUIRE( tracker.Held() == 30 );
	REQUIRE( destroy.empty() );
}

TEST_CASE( "A foreign offer with no sync outstanding takes every live seat source with it", "[wayland_selection]" )
{
	Tracker tracker;
	std::vector<int> destroy;

	tracker.Publish( 1, 10 );
	REQUIRE( tracker.OnEcho( 1, destroy ) == 10 );
	REQUIRE_FALSE( tracker.OnSyncDone( 10 ).has_value() );

	REQUIRE_FALSE( tracker.OnForeign( destroy ) );
	REQUIRE( destroy == std::vector<int>{ 10 } );
	REQUIRE_FALSE( tracker.Held().has_value() );
	REQUIRE( tracker.Empty() );

	// A cancelled source is gone before its sync comes back.
	tracker.Publish( 2, 20 );
	tracker.OnCancelled( 20 );
	REQUIRE_FALSE( tracker.SyncOutstanding() );
	REQUIRE_FALSE( tracker.OnSyncDone( 20 ).has_value() );
}
