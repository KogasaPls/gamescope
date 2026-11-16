#pragma once

namespace gamescope { class IBackend; }

namespace gamescope::hdr_offer
{
    bool CurrentOffer( IBackend &backend );

    // Clients read the offer once: the Gamescope WSI layer when a surface is
    // created, and DXVK when a child process starts. A nested host's first
    // preferred image description can describe SDR until the surface is on an
    // HDR output, so the host's ability to take HDR10 counts on its own.
    inline bool OfferHDRToClients( bool bHDREnabled, bool bOutputHDRCapable, bool bHostAcceptsHDR10, bool bForceSupportDebug )
    {
        if ( bForceSupportDebug )
            return true;

        return bHDREnabled && ( bOutputHDRCapable || bHostAcceptsHDR10 );
    }
}
