#include <catch2/catch_test_macros.hpp>

#include "hdr_metadata_sanitize.hpp"

using gamescope::hdr_metadata::SanitizedHdrLuminance;
using gamescope::hdr_metadata::SanitizeHdrLuminance;

TEST_CASE( "Mastering luminance with max equal to min is dropped", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 10000, 1, 1, 1 );

	REQUIRE_FALSE( sanitized.masteringLuminance.has_value() );
	REQUIRE_FALSE( sanitized.maxCll.has_value() );
	REQUIRE_FALSE( sanitized.maxFall.has_value() );
}

TEST_CASE( "Mastering luminance with max below min is dropped", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 50000, 1, 1, 1 );

	REQUIRE_FALSE( sanitized.masteringLuminance.has_value() );
}

TEST_CASE( "An all zero metadata blob yields nothing to send", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 0, 0, 0, 0 );

	REQUIRE_FALSE( sanitized.masteringLuminance.has_value() );
	REQUIRE_FALSE( sanitized.maxCll.has_value() );
	REQUIRE_FALSE( sanitized.maxFall.has_value() );
}

TEST_CASE( "Typical mastering metadata passes through unchanged", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 1, 1000, 800, 400 );

	REQUIRE( sanitized.masteringLuminance.has_value() );
	REQUIRE( sanitized.masteringLuminance->first == 1u );
	REQUIRE( sanitized.masteringLuminance->second == 1000u );
	REQUIRE( sanitized.maxCll.has_value() );
	REQUIRE( *sanitized.maxCll == 800u );
	REQUIRE( sanitized.maxFall.has_value() );
	REQUIRE( *sanitized.maxFall == 400u );
}

TEST_CASE( "Max CLL equal to max L is kept", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 1, 1000, 1000, 1000 );

	REQUIRE( sanitized.maxCll.has_value() );
	REQUIRE( *sanitized.maxCll == 1000u );
	REQUIRE( sanitized.maxFall.has_value() );
	REQUIRE( *sanitized.maxFall == 1000u );
}

TEST_CASE( "Max CLL equal to min L is dropped", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 10000, 1000, 1, 1 );

	REQUIRE( sanitized.masteringLuminance.has_value() );
	REQUIRE_FALSE( sanitized.maxCll.has_value() );
	REQUIRE_FALSE( sanitized.maxFall.has_value() );
}

TEST_CASE( "Max CLL above max L is dropped and an in range max FALL is kept", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 1, 1000, 4000, 500 );

	REQUIRE( sanitized.masteringLuminance.has_value() );
	REQUIRE_FALSE( sanitized.maxCll.has_value() );
	REQUIRE( sanitized.maxFall.has_value() );
	REQUIRE( *sanitized.maxFall == 500u );
}

TEST_CASE( "Max FALL above max CLL is dropped", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 1, 1000, 400, 800 );

	REQUIRE( sanitized.maxCll.has_value() );
	REQUIRE( *sanitized.maxCll == 400u );
	REQUIRE_FALSE( sanitized.maxFall.has_value() );
}

TEST_CASE( "Nothing is sent alongside a dropped mastering range", "[hdr_metadata]" )
{
	const SanitizedHdrLuminance sanitized = SanitizeHdrLuminance( 20000, 1, 500, 400 );

	REQUIRE_FALSE( sanitized.masteringLuminance.has_value() );
	REQUIRE_FALSE( sanitized.maxCll.has_value() );
	REQUIRE_FALSE( sanitized.maxFall.has_value() );
}

TEST_CASE( "max_cll is compared against the mastering minimum in its own units", "[hdr_metadata]" )
{
	using gamescope::hdr_metadata::SanitizeHdrLuminance;

	// min_display_mastering_luminance is in cd/m2 * 10000, max_cll in cd/m2, so
	// a 0.5 nit floor must not reject a 1 nit max_cll. Without the rescale the
	// comparison is 1 > 5000 and both values are dropped.
	const auto sanitized = SanitizeHdrLuminance( 5000, 1000, 1, 1 );

	REQUIRE( sanitized.masteringLuminance.has_value() );
	REQUIRE( sanitized.maxCll.has_value() );
	REQUIRE( *sanitized.maxCll == 1 );
	REQUIRE( sanitized.maxFall.has_value() );
}
