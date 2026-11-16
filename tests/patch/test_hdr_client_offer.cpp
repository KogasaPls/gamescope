#include <catch2/catch_test_macros.hpp>

#include "hdr_client_offer.hpp"

using gamescope::hdr_offer::OfferHDRToClients;

TEST_CASE( "A host that accepts HDR10 gets the offer while its output still describes SDR", "[hdr_offer]" )
{
	REQUIRE( OfferHDRToClients( true, false, true, false ) );
}

TEST_CASE( "An HDR-capable output gets the offer without host HDR10 support", "[hdr_offer]" )
{
	REQUIRE( OfferHDRToClients( true, true, false, false ) );
}

TEST_CASE( "Without --hdr-enabled there is no offer", "[hdr_offer]" )
{
	REQUIRE_FALSE( OfferHDRToClients( false, true, true, false ) );
}

TEST_CASE( "An SDR output on a host without HDR10 support gets no offer", "[hdr_offer]" )
{
	REQUIRE_FALSE( OfferHDRToClients( true, false, false, false ) );
}

TEST_CASE( "--hdr-debug-force-support makes the offer on its own", "[hdr_offer]" )
{
	REQUIRE( OfferHDRToClients( false, false, false, true ) );
}
