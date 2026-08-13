#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "x11_selection_requests.hpp"

namespace
{
using namespace gamescope::x11_selection;

SlotView Utf8( std::string_view sBytes, uint32_t uTime = 0 )
{
	return SlotView{ sBytes, "text/plain;charset=utf-8", uTime };
}

SlotView Plain( std::string_view sBytes, uint32_t uTime = 0 )
{
	return SlotView{ sBytes, "text/plain", uTime };
}
}

TEST_CASE( "TARGETS lists the served targets and adds TIMESTAMP once acquisition is known", "[x11_selection]" )
{
	const Action before = OnRequest( TargetKind::Targets, "TARGETS", Utf8( "x", 0 ) );
	REQUIRE( before.eKind == ActionKind::AnswerTargets );
	REQUIRE_FALSE( before.bIncludeTimestamp );
	REQUIRE( before.targetIndices == gamescope::wayland_selection::TargetsForOffer( { "text/plain;charset=utf-8" } ) );

	const Action after = OnRequest( TargetKind::Targets, "TARGETS", Utf8( "x", 42 ) );
	REQUIRE( after.bIncludeTimestamp );
}

TEST_CASE( "TIMESTAMP answers the acquisition time or refuses", "[x11_selection]" )
{
	REQUIRE( OnRequest( TargetKind::Timestamp, "TIMESTAMP", Utf8( "x", 0 ) ) == Refuse() );

	const Action some = OnRequest( TargetKind::Timestamp, "TIMESTAMP", Utf8( "x", 42 ) );
	REQUIRE( some.eKind == ActionKind::AnswerTimestamp );
	REQUIRE( some.uTime == 42 );
}

TEST_CASE( "Unknown targets refuse", "[x11_selection]" )
{
	REQUIRE( OnRequest( TargetKind::Other, "MULTIPLE", Plain( "x" ) ) == Refuse() );
	REQUIRE( OnRequest( TargetKind::Text, "image/png", Plain( "x" ) ) == Refuse() );
	REQUIRE( OnRequest( TargetKind::Text, nullptr, Plain( "x" ) ) == Refuse() );
}

TEST_CASE( "A text target answers with the property type following the bytes", "[x11_selection]" )
{
	const Action ascii = OnRequest( TargetKind::Text, "STRING", Utf8( "plain" ) );
	REQUIRE( ascii.eKind == ActionKind::AnswerBytes );
	REQUIRE( ascii.sBytes == "plain" );
	REQUIRE_FALSE( ascii.bUtf8Type );

	const Action utf8 = OnRequest( TargetKind::Text, "UTF8_STRING", Utf8( "caf\xc3\xa9" ) );
	REQUIRE( utf8.eKind == ActionKind::AnswerBytes );
	REQUIRE( utf8.sBytes == "caf\xc3\xa9" );
	REQUIRE( utf8.bUtf8Type );

	// A STRING request for non-ASCII UTF-8 is still typed as what the bytes are.
	REQUIRE( OnRequest( TargetKind::Text, "STRING", Utf8( "caf\xc3\xa9" ) ).bUtf8Type );
}

TEST_CASE( "UTF8_STRING is served only from a UTF-8 type", "[x11_selection]" )
{
	REQUIRE( OnRequest( TargetKind::Text, "UTF8_STRING", Plain( "x" ) ) == Refuse() );

	const Action text = OnRequest( TargetKind::Text, "TEXT", Plain( "x" ) );
	REQUIRE( text.eKind == ActionKind::AnswerBytes );
	REQUIRE_FALSE( text.bUtf8Type );
}

TEST_CASE( "An empty slot lists no text targets and refuses every conversion", "[x11_selection]" )
{
	const SlotView empty{ "", "", 7 };

	const Action targets = OnRequest( TargetKind::Targets, "TARGETS", empty );
	REQUIRE( targets.eKind == ActionKind::AnswerTargets );
	REQUIRE( targets.targetIndices.empty() );
	REQUIRE( targets.bIncludeTimestamp );

	REQUIRE( OnRequest( TargetKind::Text, "UTF8_STRING", empty ) == Refuse() );
	REQUIRE( OnRequest( TargetKind::Text, "STRING", empty ) == Refuse() );
}
