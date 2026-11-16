#pragma once

#include "capture_tonemap.hpp"
#include "convar.h"

namespace gamescope::capture_tonemap
{
    inline AtomicConVar<float> cv_sdr_white_nits{ "capture_sdr_white_nits", 500.f,
        "Luminance in nits of HDR content that SDR screenshots and PipeWire capture show as white." };
    inline AtomicConVar<float> cv_hdr_peak_nits{ "capture_hdr_peak_nits", 0.f,
        "Peak luminance in nits of HDR content for SDR screenshots and PipeWire capture. Above capture_sdr_white_nits, highlights roll off with the BT.2390 EETF instead of clipping." };
    inline AtomicConVar<int> cv_tonemap_operator{ "capture_tonemap_operator", 3,
        "BT.2390 EETF variant for capture_hdr_peak_nits: 1 luma, 2 independent, 3 max channel." };

    inline CaptureTonemap CurrentSettings()
    {
        return Resolve( cv_sdr_white_nits.Get(), cv_hdr_peak_nits.Get(), cv_tonemap_operator.Get() );
    }
}
