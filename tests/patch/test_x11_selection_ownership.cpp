#include <catch2/catch_test_macros.hpp>

#include "x11_selection_ownership.hpp"

namespace
{
using namespace gamescope::x11_selection::ownership;
}

TEST_CASE( "Taking a selection records it as ours with the time unknown", "[x11_selection]" )
{
	const uint64_t ulWord = Acquire( 0 );
	REQUIRE( IsOwned( ulWord ) );
	REQUIRE( AcquisitionTime( ulWord ) == 0 );
	REQUIRE( UnreportedAcquisitions( ulWord ) == 1 );

	// A second take before the first is reported waits for two reports.
	REQUIRE( UnreportedAcquisitions( Acquire( ulWord ) ) == 2 );

	// Taking again supersedes a release that was waiting on the report.
	REQUIRE( UnreportedAcquisitions( Acquire( ulWord | k_ulReleasePending ) ) == 2 );
	REQUIRE_FALSE( Acquire( ulWord | k_ulReleasePending ) & k_ulReleasePending );
}

TEST_CASE( "The report of an acquisition brings the time once nothing newer is outstanding", "[x11_selection]" )
{
	const ReportStep step = Reported( Acquire( 0 ), 42 );
	REQUIRE( step == ReportStep{ .ulWord = k_ulOwned | 42 } );
	REQUIRE( AcquisitionTime( step.ulWord ) == 42 );
	REQUIRE( UnreportedAcquisitions( step.ulWord ) == 0 );
}

TEST_CASE( "A release with the time known goes out stamped with it", "[x11_selection]" )
{
	const uint64_t ulWord = Reported( Acquire( 0 ), 42 ).ulWord;
	REQUIRE( Release( ulWord ) == ReleaseStep{ .ulWord = 0, .eAction = ReleaseAction::Release, .uTime = 42 } );
	REQUIRE( Release( 0 ) == ReleaseStep{ .ulWord = 0, .eAction = ReleaseAction::NotOurs } );
}

TEST_CASE( "A release with the time unknown is made by the report that brings it", "[x11_selection]" )
{
	const uint64_t ulOwned = Acquire( 0 );
	const ReleaseStep release = Release( ulOwned );
	REQUIRE( release.eAction == ReleaseAction::Deferred );
	REQUIRE( IsOwned( release.ulWord ) );
	REQUIRE( UnreportedAcquisitions( release.ulWord ) == 1 );

	REQUIRE( Reported( release.ulWord, 42 ) == ReportStep{ .ulWord = 0, .bRelease = true } );
}

TEST_CASE( "The report of an older acquisition does not make a pending release", "[x11_selection]" )
{
	// own(A), own(B), release while the time is unknown: the release stamped
	// with A's time would be ignored by a server that holds B, leaving us owner
	// with nothing to serve. B's report is the one to release with.
	uint64_t ulWord = Acquire( Acquire( 0 ) );
	const ReleaseStep release = Release( ulWord );
	REQUIRE( release.eAction == ReleaseAction::Deferred );
	ulWord = release.ulWord;

	const ReportStep reportA = Reported( ulWord, 100 );
	REQUIRE_FALSE( reportA.bRelease );
	REQUIRE( IsOwned( reportA.ulWord ) );
	REQUIRE( AcquisitionTime( reportA.ulWord ) == 0 );
	REQUIRE( UnreportedAcquisitions( reportA.ulWord ) == 1 );
	REQUIRE( reportA.ulWord & k_ulReleasePending );

	REQUIRE( Reported( reportA.ulWord, 200 ) == ReportStep{ .ulWord = 0, .bRelease = true } );
}

TEST_CASE( "Without a release pending the last report adopts its time", "[x11_selection]" )
{
	const uint64_t ulWord = Acquire( Acquire( 0 ) );
	const ReportStep reportA = Reported( ulWord, 100 );
	REQUIRE( AcquisitionTime( reportA.ulWord ) == 0 );
	REQUIRE( Release( reportA.ulWord ).eAction == ReleaseAction::Deferred );

	const ReportStep reportB = Reported( reportA.ulWord, 200 );
	REQUIRE( reportB == ReportStep{ .ulWord = k_ulOwned | 200 } );
	REQUIRE( Release( reportB.ulWord ).uTime == 200 );
}

TEST_CASE( "A report with no acquisition outstanding still adopts its time", "[x11_selection]" )
{
	// Another client's take zeroed the record before our later take was
	// reported; the report says the server holds ours after all.
	REQUIRE( Reported( 0, 42 ) == ReportStep{ .ulWord = k_ulOwned | 42 } );
	REQUIRE( Reported( k_ulOwned | 7, 42 ) == ReportStep{ .ulWord = k_ulOwned | 42 } );
}

TEST_CASE( "The unreported count saturates instead of wrapping into the flags", "[x11_selection]" )
{
	uint64_t ulWord = k_ulOwned | ( ( k_ulUnreportedMax - 1 ) << k_uUnreportedShift );
	REQUIRE( UnreportedAcquisitions( Acquire( ulWord ) ) == k_ulUnreportedMax );
	ulWord = Acquire( Acquire( ulWord ) );

	REQUIRE( UnreportedAcquisitions( ulWord ) == k_ulUnreportedMax );
	REQUIRE( IsOwned( ulWord ) );
	REQUIRE( AcquisitionTime( ulWord ) == 0 );
	REQUIRE_FALSE( ulWord & k_ulReleasePending );
}
