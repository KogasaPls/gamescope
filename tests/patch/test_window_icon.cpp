#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "window_icon.hpp"

using gamescope::window_icon::ParseWindowIcon;
using gamescope::window_icon::WindowIcon;

TEST_CASE( "An empty _NET_WM_ICON property has no icon", "[window_icon]" )
{
	const std::vector<uint32_t> words;

	REQUIRE_FALSE( ParseWindowIcon( words ).has_value() );
}

TEST_CASE( "A property with only dimensions has no icon", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 1, 1 };

	REQUIRE_FALSE( ParseWindowIcon( words ).has_value() );
}

TEST_CASE( "Dimensions larger than the payload are rejected", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 16, 16, 0xFFFFFFFF };

	REQUIRE_FALSE( ParseWindowIcon( words ).has_value() );
}

TEST_CASE( "A record that exactly fits the payload is accepted", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 2, 2, 1, 2, 3, 4 };

	const std::optional<WindowIcon> oIcon = ParseWindowIcon( words );

	REQUIRE( oIcon.has_value() );
	REQUIRE( oIcon->uWidth == 2 );
	REQUIRE( oIcon->uHeight == 2 );
	REQUIRE( oIcon->pixels.size() == 4 );
	REQUIRE( oIcon->pixels.data() == &words[2] );
}

TEST_CASE( "The largest of several records wins", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 1, 1, 0xAA, 2, 1, 0xBB, 0xCC };

	const std::optional<WindowIcon> oIcon = ParseWindowIcon( words );

	REQUIRE( oIcon.has_value() );
	REQUIRE( oIcon->uWidth == 2 );
	REQUIRE( oIcon->uHeight == 1 );
	REQUIRE( oIcon->pixels.size() == 2 );
	REQUIRE( oIcon->pixels[0] == 0xBB );
}

TEST_CASE( "A truncated trailing record leaves the earlier one usable", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 1, 1, 0xAA, 4, 4, 0xBB };

	const std::optional<WindowIcon> oIcon = ParseWindowIcon( words );

	REQUIRE( oIcon.has_value() );
	REQUIRE( oIcon->uWidth == 1 );
	REQUIRE( oIcon->uHeight == 1 );
	REQUIRE( oIcon->pixels.size() == 1 );
	REQUIRE( oIcon->pixels[0] == 0xAA );
}

TEST_CASE( "Dimensions whose product overflows 32 bits are rejected", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 65536, 65536, 0xAA, 0xBB, 0xCC };

	REQUIRE_FALSE( ParseWindowIcon( words ).has_value() );
}

TEST_CASE( "A zero dimension record is skipped", "[window_icon]" )
{
	const std::vector<uint32_t> words = { 0, 0, 1, 1, 0xAA };

	const std::optional<WindowIcon> oIcon = ParseWindowIcon( words );

	REQUIRE( oIcon.has_value() );
	REQUIRE( oIcon->uWidth == 1 );
	REQUIRE( oIcon->uHeight == 1 );
	REQUIRE( oIcon->pixels[0] == 0xAA );
}

TEST_CASE( "An implausibly large dimension is skipped", "[window_icon]" )
{
	std::vector<uint32_t> words = { gamescope::window_icon::k_uMaxIconDimension + 1, 1 };
	words.resize( words.size() + gamescope::window_icon::k_uMaxIconDimension + 1, 0xAA );
	words.push_back( 1 );
	words.push_back( 1 );
	words.push_back( 0xBB );

	const std::optional<WindowIcon> oIcon = ParseWindowIcon( words );

	REQUIRE( oIcon.has_value() );
	REQUIRE( oIcon->uWidth == 1 );
	REQUIRE( oIcon->pixels[0] == 0xBB );
}

TEST_CASE( "A record whose area overflows 32 bits is measured in 64", "[window_icon]" )
{
	using gamescope::window_icon::ParseWindowIcon;

	// 65536 * 65536 is 2^32, which wraps to 0 in 32-bit arithmetic and would make
	// the two header words look like a complete record with no payload, so the
	// walk would accept it and hand back a span it does not have.
	const std::vector<uint32_t> words = { 65536, 65536, 0xdeadbeef, 0xdeadbeef };

	REQUIRE( !ParseWindowIcon( words ).has_value() );

	// Same shape one record in, so the overflow cannot be caught by the
	// dimension cap alone: a usable 1x1 record follows it and must not be found
	// past a record whose length wrapped.
	const std::vector<uint32_t> after = { 65536, 65536, 1, 1, 0x11223344 };

	REQUIRE( !ParseWindowIcon( after ).has_value() );
}
