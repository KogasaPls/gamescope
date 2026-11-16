#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "color_helpers_impl.h"
#include "capture_tonemap.hpp"

namespace
{
void RequireColor( const glm::vec3 &actual, const glm::vec3 &expected, float tolerance )
{
	for ( int channel = 0; channel < 3; ++channel )
	{
		INFO( "channel " << channel << ": " << actual[channel] << " expected " << expected[channel] );
		REQUIRE( std::isfinite( actual[channel] ) );
		REQUIRE( std::abs( actual[channel] - expected[channel] ) < tolerance );
	}
}
}

TEST_CASE( "SDR saturation targets the panel primaries in an HDR container", "[color_helpers]" )
{
	const float wideness = GENERATE( 0.f, 0.5f );
	CAPTURE( wideness );
	displaycolorimetry_t source;
	colormapping_t mapping;
	buildSDRColorimetry( &source, &mapping, wideness, displaycolorimetry_709 );
	tonemapping_t tonemapping;
	tonemapping.g22_luminance = 100.f;
	lut3d_t lut;
	calcColorTransform<rendervulkan::s_nLutEdgeSize3d>( nullptr, 0, &lut,
		source, EOTF_Gamma22, displaycolorimetry_2020, EOTF_PQ, displaycolorimetry_709,
		glm::vec2( 0.f ), k_EChromaticAdapatationMethod_XYZ,
		mapping, nightmode_t{}, tonemapping, nullptr, 1.f );
	const int edge = rendervulkan::s_nLutEdgeSize3d;
	const int last = edge - 1;
	struct Sample
	{
		int r, g, b;
		glm::vec3 nits;
	};
	// Rec.709 primaries expressed in linear BT.2020 at a 100-nit reference white.
	const Sample samples[] = {
		{ last, 0, 0, { 62.74039f, 6.90973f, 1.63914f } },
		{ 0, last, 0, { 32.92830f, 91.95404f, 8.80132f } },
		{ 0, 0, last, { 4.33131f, 1.13624f, 89.55954f } },
		{ last, last, last, { 100.f, 100.f, 100.f } },
		{ 0, 0, 0, { 0.f, 0.f, 0.f } },
	};
	for ( const auto &sample : samples )
	{
		CAPTURE( sample.r, sample.g, sample.b );
		const auto &encoded = lut.data[sample.r + edge * ( sample.g + edge * sample.b )];
		RequireColor( pq_to_nits( encoded ), sample.nits, 0.02f );
	}
}

TEST_CASE( "Neutral HDR capture samples clip or roll off into SDR white", "[color_helpers]" )
{
	const bool rollOff = GENERATE( false, true );
	const auto capture = gamescope::capture_tonemap::Resolve( 200.f, rollOff ? 1000.f : 0.f, 3 );
	tonemapping_t mapping;
	mapping.g22_luminance = capture.flSDRWhiteNits;
	if ( capture.bRollOff )
	{
		mapping.eOperator = static_cast<ETonemapOperator>( capture.nOperator );
		mapping.eetf2390.init( { 0.f, capture.flSourcePeakNits }, { 0.f, capture.flSDRWhiteNits } );
	}

	lut1d_t shaper;
	calcColorTransform<rendervulkan::s_nLutEdgeSize3d>( &shaper, rendervulkan::s_nLutSize1d, nullptr,
		displaycolorimetry_2020, EOTF_PQ, displaycolorimetry_709, EOTF_Gamma22, displaycolorimetry_709,
		{}, k_EChromaticAdapatationMethod_XYZ, {}, {}, mapping, nullptr, 1.f );

	auto gray = [&]( float nits ) {
		return shaper.dataR[std::lround( nits_to_pq( nits ) * ( shaper.lutSize - 1 ) )];
	};
	REQUIRE( gray( 0.f ) < 0.001f );
	REQUIRE( gray( 1000.f ) > 0.995f );
	if ( rollOff )
	{
		REQUIRE( gray( 200.f ) < gray( 500.f ) );
		REQUIRE( gray( 500.f ) < gray( 1000.f ) );
	}
	else
	{
		REQUIRE( std::abs( gray( 50.f ) - std::pow( 0.25f, 1.f / 2.2f ) ) < 0.002f );
		REQUIRE( gray( 500.f ) == 1.f );
	}
}
