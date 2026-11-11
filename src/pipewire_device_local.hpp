#pragma once

#include "convar.h"

namespace gamescope::pipewire_capture
{
    inline AtomicConVar<bool> cv_device_local{ "pipewire_device_local", false,
        "Prefer device-local memory for PipeWire DMA-BUF capture buffers." };
}
