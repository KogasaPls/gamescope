#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "Utils/AtomicValue.h"

namespace
{
using gamescope::AtomicValue;

template <typename A, typename B>
concept Conditional = requires( bool b, A a, B &v ) { b ? a : v; };

static_assert( Conditional<uint32_t, AtomicValue<uint32_t>> );
static_assert( Conditional<uint64_t, AtomicValue<uint32_t>> );
static_assert( Conditional<double, AtomicValue<uint32_t>> );
static_assert( Conditional<int, AtomicValue<int>> );
static_assert( Conditional<AtomicValue<uint32_t>, AtomicValue<uint32_t>> );

static_assert( !Conditional<int, AtomicValue<uint32_t>> );
static_assert( !Conditional<int32_t, AtomicValue<uint32_t>> );
static_assert( !Conditional<bool, AtomicValue<uint32_t>> );
static_assert( !Conditional<short, AtomicValue<int>> );
}

TEST_CASE( "AtomicValue reads and writes like its value type", "[atomic_value]" )
{
	AtomicValue<uint32_t> uWidth{ 1280u };
	AtomicValue<int> nRefresh{ 60000 };

	REQUIRE( uint32_t( uWidth ) == 1280u );
	uWidth = 2560u;
	REQUIRE( ( true ? uWidth : uWidth ) == 2560u );
	REQUIRE( ( false ? 7u : uWidth ) == 2560u );
	REQUIRE( ( nRefresh ? nRefresh : 0 ) == 60000 );
}
