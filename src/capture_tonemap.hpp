#pragma once

#include <algorithm>
#include <cmath>

namespace gamescope::capture_tonemap
{
    struct CaptureTonemap
    {
        float flSDRWhiteNits = 500.f;
        bool bRollOff = false;
        float flSourcePeakNits = 0.f;
        // An ETonemapOperator EETF 2390 variant: 1 luma, 2 independent, 3 max channel.
        int nOperator = 3;

        bool operator==( const CaptureTonemap & ) const = default;
    };

    inline CaptureTonemap Resolve( float flSDRWhiteNits, float flHDRPeakNits, int nOperator )
    {
        constexpr float k_flPQPeakNits = 10000.f;

        CaptureTonemap result;
        if ( std::isfinite( flSDRWhiteNits ) && flSDRWhiteNits > 0.f )
            result.flSDRWhiteNits = std::min( flSDRWhiteNits, k_flPQPeakNits );

        if ( std::isfinite( flHDRPeakNits ) )
        {
            const float flPeak = std::min( flHDRPeakNits, k_flPQPeakNits );
            if ( flPeak > result.flSDRWhiteNits )
            {
                result.bRollOff = true;
                result.flSourcePeakNits = flPeak;
            }
        }

        if ( nOperator >= 1 && nOperator <= 3 )
            result.nOperator = nOperator;

        return result;
    }
}
