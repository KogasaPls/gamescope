#pragma once

#include <cstdint>

namespace gamescope
{
    template <typename Held>
    bool RotateToFreeOutputSlot( uint32_t &uCursor, uint32_t uSize, Held isHeld )
    {
        for ( uint32_t i = 0; i < uSize; ++i )
        {
            const uint32_t uSlot = ( uCursor + i ) % uSize;
            if ( !isHeld( uSlot ) )
            {
                uCursor = uSlot;
                return true;
            }
        }
        return false;
    }
}
