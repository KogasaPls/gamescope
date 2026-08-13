#include <catch2/catch_test_macros.hpp>
#include <array>
#include "output_ring.hpp"

TEST_CASE( "An exhausted output ring waits for release across repeated paints", "[output_ring]" )
{
    std::array<bool, 4> held = { true, true, true, true };
    auto isHeld = [&]( uint32_t slot ) { return held[slot]; };
    uint32_t cursor = 2;
    for ( int i = 0; i < 300; ++i )
    {
        REQUIRE_FALSE( gamescope::RotateToFreeOutputSlot( cursor, held.size(), isHeld ) );
        REQUIRE( cursor == 2 );
    }
    held[0] = false;
    REQUIRE( gamescope::RotateToFreeOutputSlot( cursor, held.size(), isHeld ) );
    REQUIRE( cursor == 0 );
}
