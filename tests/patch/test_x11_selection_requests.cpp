#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

#include "x11_selection_requests.hpp"

namespace
{
using namespace gamescope::x11_selection;

const std::vector<std::string> k_Utf8Offer = { "text/plain;charset=utf-8", "text/plain" };
const std::vector<std::string> k_PlainOffer = { "text/plain" };
constexpr int k_Clipboard = 0;
constexpr int k_Primary = 1;

RequestId Id( uint64_t n ) { return RequestId{ nullptr, n, 1, 2, 3, 4 }; }

SlotView Lazy( const std::vector<std::string> &offer, uint64_t ulEpoch, uint32_t uTime = 0 )
{
	return SlotView{ "", "", offer, true, ulEpoch, uTime };
}

SlotView Eager( std::string_view sBytes, std::string_view sMime, uint64_t ulEpoch, uint32_t uTime = 0 )
{
	return SlotView{ sBytes, sMime, {}, false, ulEpoch, uTime };
}
}

TEST_CASE( "TARGETS lists the served targets and adds TIMESTAMP once acquisition is known", "[x11_selection]" )
{
	RequestQueue queue;
	auto before = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Targets, "TARGETS", Lazy( k_Utf8Offer, 1, 0 ), 1'000 );
	REQUIRE( before.size() == 1 );
	REQUIRE( before[0].eKind == ActionKind::AnswerTargets );
	REQUIRE_FALSE( before[0].bIncludeTimestamp );
	REQUIRE( before[0].targetIndices == gamescope::wayland_selection::TargetsForOffer( k_Utf8Offer ) );
	auto after = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Targets, "TARGETS", Lazy( k_Utf8Offer, 1, 42 ), 1'000 );
	REQUIRE( after[0].bIncludeTimestamp );
	REQUIRE( queue.parked.empty() );
}

TEST_CASE( "TIMESTAMP answers the acquisition time or refuses", "[x11_selection]" )
{
	RequestQueue queue;
	auto none = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Timestamp, "TIMESTAMP", Lazy( k_Utf8Offer, 1, 0 ), 1'000 );
	REQUIRE( none == std::vector<Action>{ Refuse( Id( 1 ) ) } );
	auto some = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Timestamp, "TIMESTAMP", Lazy( k_Utf8Offer, 1, 42 ), 1'000 );
	REQUIRE( some[0].eKind == ActionKind::AnswerTimestamp );
	REQUIRE( some[0].uTime == 42 );
}

TEST_CASE( "Unknown targets refuse", "[x11_selection]" )
{
	RequestQueue queue;
	REQUIRE( OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Other, "MULTIPLE", Eager( "x", "text/plain", 1 ), 1'000 ) == std::vector<Action>{ Refuse( Id( 1 ) ) } );
	REQUIRE( OnRequest( queue, Id( 2 ), k_Clipboard, TargetKind::Text, "image/png", Eager( "x", "text/plain", 1 ), 1'000 ) == std::vector<Action>{ Refuse( Id( 2 ) ) } );
}

TEST_CASE( "An eager slot answers at once with the property type following the bytes", "[x11_selection]" )
{
	RequestQueue queue;
	auto ascii = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "STRING", Eager( "plain", "text/plain;charset=utf-8", 1 ), 1'000 );
	REQUIRE( ascii[0].eKind == ActionKind::AnswerBytes );
	REQUIRE( ascii[0].sBytes == "plain" );
	REQUIRE_FALSE( ascii[0].bUtf8Type );
	auto utf8 = OnRequest( queue, Id( 2 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Eager( "caf\xc3\xa9", "text/plain;charset=utf-8", 1 ), 1'000 );
	REQUIRE( utf8[0].bUtf8Type );
	REQUIRE( queue.parked.empty() );
}

TEST_CASE( "UTF8_STRING is served only from a UTF-8 host type", "[x11_selection]" )
{
	RequestQueue queue;
	REQUIRE( OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_PlainOffer, 1 ), 1'000 ) == std::vector<Action>{ Refuse( Id( 1 ) ) } );
	auto text = OnRequest( queue, Id( 2 ), k_Clipboard, TargetKind::Text, "TEXT", Lazy( k_PlainOffer, 1 ), 1'000 );
	REQUIRE( text.size() == 1 );
	REQUIRE( text[0].eKind == ActionKind::QueueFetch );
	REQUIRE( text[0].sMimeType == "text/plain" );
	REQUIRE( queue.parked.size() == 1 );
}

TEST_CASE( "A lazy slot parks and fetches once per type and epoch", "[x11_selection]" )
{
	RequestQueue queue;
	auto first = OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	REQUIRE( first.size() == 1 );
	REQUIRE( first[0].eKind == ActionKind::QueueFetch );
	REQUIRE( first[0].ulEpoch == 1 );
	auto second = OnRequest( queue, Id( 2 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	REQUIRE( second.empty() );
	auto newer = OnRequest( queue, Id( 3 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 2 ), 1'000 );
	REQUIRE( newer.size() == 1 );
	REQUIRE( newer[0].ulEpoch == 2 );
	REQUIRE( queue.parked.size() == 3 );
}

TEST_CASE( "A failed queue refuses the request and parks nothing", "[x11_selection]" )
{
	RequestQueue queue;
	OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	REQUIRE( OnFetchQueueFailed( queue, Id( 1 ) ) == std::vector<Action>{ Refuse( Id( 1 ) ) } );
	REQUIRE( queue.parked.empty() );
}

TEST_CASE( "Delivery answers every parked request for its type at the current epoch", "[x11_selection]" )
{
	RequestQueue queue;
	OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	OnRequest( queue, Id( 2 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	OnRequest( queue, Id( 3 ), k_Primary, TargetKind::Text, "STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	auto actions = OnFetchDelivered( queue, k_Clipboard, "text/plain;charset=utf-8", "caf\xc3\xa9", true, 1, 1 );
	REQUIRE( actions.size() == 2 );
	REQUIRE( actions[0].eKind == ActionKind::AnswerBytes );
	REQUIRE( actions[0].sBytes == "caf\xc3\xa9" );
	REQUIRE( actions[0].bUtf8Type );
	REQUIRE( actions[1].id == Id( 2 ) );
	REQUIRE( queue.parked.size() == 1 );
	REQUIRE( queue.parked[0].id == Id( 3 ) );

	auto ascii = OnFetchDelivered( queue, k_Primary, "text/plain;charset=utf-8", "plain", true, 1, 1 );
	REQUIRE( ascii.size() == 1 );
	REQUIRE( ascii[0].eKind == ActionKind::AnswerBytes );
	REQUIRE( ascii[0].sBytes == "plain" );
	REQUIRE_FALSE( ascii[0].bUtf8Type );
	REQUIRE( queue.parked.empty() );
}

TEST_CASE( "Delivery behind the current epoch refuses", "[x11_selection]" )
{
	RequestQueue queue;
	OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	auto actions = OnFetchDelivered( queue, k_Clipboard, "text/plain;charset=utf-8", "old", true, 1, 2 );
	REQUIRE( actions == std::vector<Action>{ Refuse( Id( 1 ) ) } );
	REQUIRE( queue.parked.empty() );
}

TEST_CASE( "A failed delivery refuses", "[x11_selection]" )
{
	RequestQueue queue;
	OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	REQUIRE( OnFetchDelivered( queue, k_Clipboard, "text/plain;charset=utf-8", "", false, 1, 1 ) == std::vector<Action>{ Refuse( Id( 1 ) ) } );
}

TEST_CASE( "The tick refuses expired and stale requests and keeps the rest", "[x11_selection]" )
{
	RequestQueue queue;
	OnRequest( queue, Id( 1 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	OnRequest( queue, Id( 2 ), k_Primary, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 5 ), 5'000 );
	OnRequest( queue, Id( 3 ), k_Primary, TargetKind::Text, "TEXT", Lazy( k_Utf8Offer, 5 ), 5'000 );
	const std::array<uint64_t, 2> epochs = { 1, 6 };
	auto actions = OnTick( queue, 2'000, epochs );
	REQUIRE( actions.size() == 3 );
	REQUIRE( queue.parked.empty() );
	RequestQueue fresh;
	OnRequest( fresh, Id( 4 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 5'000 );
	REQUIRE( OnTick( fresh, 2'000, epochs ).empty() );
	REQUIRE( fresh.parked.size() == 1 );
}

TEST_CASE( "The seventeenth parked request is refused across both selections", "[x11_selection]" )
{
	RequestQueue queue;
	for ( uint64_t n = 0; n < 8; n++ )
	{
		OnRequest( queue, Id( n ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
		OnRequest( queue, Id( 100 + n ), k_Primary, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 );
	}
	REQUIRE( queue.parked.size() == 16 );
	REQUIRE( OnRequest( queue, Id( 999 ), k_Clipboard, TargetKind::Text, "UTF8_STRING", Lazy( k_Utf8Offer, 1 ), 1'000 ) == std::vector<Action>{ Refuse( Id( 999 ) ) } );
	REQUIRE( queue.parked.size() == 16 );
}
