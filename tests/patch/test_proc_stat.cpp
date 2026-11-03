#include <catch2/catch_test_macros.hpp>

#include "proc_stat_helpers.hpp"

namespace
{
using gamescope::proc_stat::ParseLiveParentPid;
}

TEST_CASE( "The parent pid follows a plain command name", "[proc_stat]" )
{
	REQUIRE( ParseLiveParentPid( "4242 (gamescopereaper) S 17 4242 17 0 -1 4194560\n" ) == 17 );
}

TEST_CASE( "A command name with spaces does not shift the fields", "[proc_stat]" )
{
	REQUIRE( ParseLiveParentPid( "4242 (comm with space) S 17 4242 17 0 -1 4194560\n" ) == 17 );
}

TEST_CASE( "A command name containing parentheses ends at the last one", "[proc_stat]" )
{
	REQUIRE( ParseLiveParentPid( "4242 (a) b (c)) R 99 4242 99 0 -1 4194560\n" ) == 99 );
	REQUIRE( ParseLiveParentPid( "4242 ()) S 7 4242 7 0 -1 4194560\n" ) == 7 );
}

TEST_CASE( "A zombie reports no parent", "[proc_stat]" )
{
	REQUIRE_FALSE( ParseLiveParentPid( "4242 (wine64-preloader) Z 17 4242 17 0 -1 4227084\n" ).has_value() );
	REQUIRE_FALSE( ParseLiveParentPid( "4242 (comm with space) Z 17 4242 17 0 -1 4227084 0 0 0 0 3 1 0 0 20 0 1 0 900\n" ).has_value() );
}

TEST_CASE( "A process whose main thread exited keeps its parent while threads run", "[proc_stat]" )
{
	REQUIRE( ParseLiveParentPid( "4242 (wine64-preloader) Z 17 4242 17 0 -1 4227084 0 0 0 0 3 1 0 0 20 0 2 0 900\n" ) == 17 );
	REQUIRE( ParseLiveParentPid( "4242 (comm) with) Z 17 4242 17 0 -1 4227084 0 0 0 0 3 1 0 0 20 0 9 0 900\n" ) == 17 );
}

TEST_CASE( "A malformed line reports no parent", "[proc_stat]" )
{
	REQUIRE_FALSE( ParseLiveParentPid( "" ).has_value() );
	REQUIRE_FALSE( ParseLiveParentPid( "4242 comm S 17\n" ).has_value() );
	REQUIRE_FALSE( ParseLiveParentPid( "4242 (comm)\n" ).has_value() );
	REQUIRE_FALSE( ParseLiveParentPid( "4242 (comm) S\n" ).has_value() );
}
