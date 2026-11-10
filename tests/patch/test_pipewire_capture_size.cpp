#include <catch2/catch_test_macros.hpp>
#include "pipewire_capture_size.hpp"

TEST_CASE("PipeWire rounds within consumer bounds", "[pipewire]")
{
	using namespace gamescope::pipewire_capture;
	auto size = FitSize({1920, 1080}, {853, 481});
	REQUIRE(size.width == 852);
	REQUIRE(size.height == 480);
	REQUIRE(SupportsNV12(size));

	// Preserve the 1:1 viewport and leave one pixel for chroma padding.
	size = FitSize({641, 401}, {UINT32_MAX, UINT32_MAX});
	REQUIRE(size.width == 642);
	REQUIRE(size.height == 402);

	size = FitSize({801, 601}, {801, 601});
	REQUIRE(size.width == 800);
	REQUIRE(size.height == 600);

	size = FitSize({1920, 1080}, {1, 1});
	REQUIRE(size.width == 1);
	REQUIRE(size.height == 1);
	REQUIRE_FALSE(SupportsNV12(size));
}
