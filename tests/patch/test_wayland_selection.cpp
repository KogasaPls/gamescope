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

TEST_CASE( "A running read serves a request only for the same announcement", "[wayland_selection]" )
{
	using gamescope::wayland_selection::SelectionReadServes;

	REQUIRE( SelectionReadServes( false, 0, "UTF8_STRING", 7, 0, "UTF8_STRING", 7 ) );
	REQUIRE_FALSE( SelectionReadServes( false, 0, "UTF8_STRING", 6, 0, "UTF8_STRING", 7 ) );
	REQUIRE_FALSE( SelectionReadServes( true, 0, "UTF8_STRING", 7, 0, "UTF8_STRING", 7 ) );
	REQUIRE_FALSE( SelectionReadServes( false, 0, "text/plain", 7, 0, "UTF8_STRING", 7 ) );
	REQUIRE_FALSE( SelectionReadServes( false, 1, "UTF8_STRING", 7, 0, "UTF8_STRING", 7 ) );
}
