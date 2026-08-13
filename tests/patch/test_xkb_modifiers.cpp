#include <catch2/catch_test_macros.hpp>

#include "xkb_modifier_helpers.hpp"

#include <cstring>
#include <vector>

namespace
{
using gamescope::xkb_modifiers::TranslateIndex;
using gamescope::xkb_modifiers::TranslateMask;

constexpr uint32_t k_uInvalid = 0xffffffffu;

struct FakeKeymap
{
	std::vector<const char *> names;

	const char *Name( uint32_t uIndex ) const
	{
		return uIndex < names.size() ? names[ uIndex ] : nullptr;
	}

	uint32_t Index( const char *pszName ) const
	{
		for ( uint32_t i = 0; i < names.size(); i++ )
		{
			if ( names[ i ] && !strcmp( names[ i ], pszName ) )
				return i;
		}
		return k_uInvalid;
	}
};

uint32_t Translate( uint32_t uMask, const FakeKeymap &source, const FakeKeymap &dest )
{
	return TranslateMask( uMask,
		[&]( uint32_t i ) { return source.Name( i ); },
		[&]( const char *pszName ) { return dest.Index( pszName ); } );
}

const FakeKeymap k_Core{ { "Shift", "Lock", "Control", "Mod1", "Mod2", "Mod3", "Mod4", "Mod5" } };
}

TEST_CASE( "Modifiers at the same index in both keymaps pass through", "[xkb_modifiers]" )
{
	FakeKeymap source = k_Core;
	source.names.push_back( "NumLock" );
	FakeKeymap dest = source;

	REQUIRE( Translate( 0x1ff, source, dest ) == 0x1ff );
	REQUIRE( Translate( 0, source, dest ) == 0 );
}

TEST_CASE( "Modifiers are matched by name, not by index", "[xkb_modifiers]" )
{
	FakeKeymap source = k_Core;
	source.names.push_back( "NumLock" );
	source.names.push_back( "LevelThree" );
	FakeKeymap dest = k_Core;
	dest.names.push_back( "LevelThree" );
	dest.names.push_back( "NumLock" );

	REQUIRE( Translate( 1u << 8, source, dest ) == 1u << 9 );
	REQUIRE( Translate( 1u << 9, source, dest ) == 1u << 8 );
	REQUIRE( Translate( ( 1u << 9 ) | 0x4, source, dest ) == ( ( 1u << 8 ) | 0x4 ) );
}

TEST_CASE( "A modifier the destination lacks is dropped", "[xkb_modifiers]" )
{
	FakeKeymap source = k_Core;
	source.names.push_back( "Hyper" );
	const FakeKeymap &dest = k_Core;

	REQUIRE( Translate( ( 1u << 8 ) | 0x1, source, dest ) == 0x1 );
}

TEST_CASE( "A bit the source does not define is dropped", "[xkb_modifiers]" )
{
	REQUIRE( Translate( 0x80000001u, k_Core, k_Core ) == 0x1 );
}

TEST_CASE( "A destination index past 32 bits is dropped", "[xkb_modifiers]" )
{
	const FakeKeymap source{ { "Shift" } };
	FakeKeymap dest;
	for ( int i = 0; i < 32; i++ )
		dest.names.push_back( "Pad" );
	dest.names.push_back( "Shift" );

	REQUIRE( Translate( 0x1, source, dest ) == 0 );
}

TEST_CASE( "A layout is translated by name within the destination's layouts", "[xkb_modifiers]" )
{
	const FakeKeymap source{ { "English (US)", "Russian" } };
	const FakeKeymap dest{ { "Russian", "English (US)" } };
	const FakeKeymap other{ { "German" } };

	auto translate = [&]( uint32_t uIndex, const FakeKeymap &to )
	{
		return TranslateIndex( uIndex,
			[&]( uint32_t i ) { return source.Name( i ); },
			[&]( const char *pszName ) { return to.Index( pszName ); },
			uint32_t( to.names.size() ) );
	};

	REQUIRE( translate( 0, dest ) == 1u );
	REQUIRE( translate( 1, dest ) == 0u );
	REQUIRE_FALSE( translate( 2, dest ).has_value() );
	REQUIRE_FALSE( translate( 0, other ).has_value() );
}
