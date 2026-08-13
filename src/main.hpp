#pragma once

#include <getopt.h>

#include <atomic>

extern const char *gamescope_optstring;
extern const struct option *gamescope_options;

extern std::atomic< bool > g_bRun;

extern int g_nNestedWidth;
extern int g_nNestedHeight;
extern int g_nNestedRefresh; // mHz
extern int g_nNestedUnfocusedRefresh; // mHz
extern int g_nNestedDisplayIndex;

// The output size is written by one thread, the one that owns the backend's
// size paths, and read by several: the Wayland input thread's pointer and touch
// normalisation, wlserver, the pipewire capture thread, and mangoapp. Plain uint32_t globals made every one of those reads a
// data race, so the compiler was free to hoist or duplicate them -- undefined
// behaviour, not merely a stale value.
//
// std::atomic<uint32_t> with the default operators removes that. The implicit
// load and store conversions keep every existing use site compiling unchanged,
// and on x86-64 a seq_cst load is a plain mov; only the stores take a fence,
// and those happen on a mode set or a window resize, not per frame.
//
// The two are individually atomic, NOT jointly. A reader can still see a new
// width against an old height for one event, which costs at worst one
// mispositioned pointer event or one capture frame sized from a mismatched
// pair, both self-correcting on the next one. A consumer that needs the pair as
// one value has to carry it as one value; no global here does.
extern std::atomic< uint32_t > g_nOutputWidth;
extern std::atomic< uint32_t > g_nOutputHeight;
extern bool g_bForceRelativeMouse;
extern std::atomic< int > g_nOutputRefresh; // mHz
// Atomic for the same reason as the output size above: it is read
// cross-thread (e.g. the OpenVR backend's input thread), and tying it to app
// content turned it from write-once-at-startup into a write every few
// hundred milliseconds on the steamcompmgr thread.
extern std::atomic< bool > g_bOutputHDREnabled;
extern bool g_bForceInternal;

extern bool g_bForceCompositionRotation;
extern uint32_t g_uOutputRotation;

extern bool g_bFullscreen;

extern bool g_bGrabbed;
extern bool g_bTrackAppSize;

extern float g_mouseSensitivity;
extern const char *g_sOutputName;

enum class GamescopeUpscaleFilter : uint32_t
{
    LINEAR = 0,
    NEAREST,
    FSR,
    NIS,
    PIXEL,

    FROM_VIEW = 0xF, // internal
};

static constexpr bool DoesHardwareSupportUpscaleFilter( GamescopeUpscaleFilter eFilter )
{
    // Could do nearest someday... AMDGPU DC supports custom tap placement to an extent.

    return eFilter == GamescopeUpscaleFilter::LINEAR;
}

enum class GamescopeUpscaleScaler : uint32_t
{
    AUTO,
    INTEGER,
    FIT,
    FILL,
    STRETCH,
};

struct UpscaleSettings_t
{
    GamescopeUpscaleFilter eFilter{};
    GamescopeUpscaleScaler eScaler{};
};

// XXX(misyl): This is bad! We shouldnt change the upscaler like this at all!!!
// We should move this to business logic in paint_window or something!
static constexpr UpscaleSettings_t GetUpscaleSettings(
    bool bFocusIsSteam,
    GamescopeUpscaleFilter eWantedFilter,
    GamescopeUpscaleScaler eWantedScaler )
{
    if ( bFocusIsSteam )
        return UpscaleSettings_t{ GamescopeUpscaleFilter::LINEAR, GamescopeUpscaleScaler::FIT };

    return UpscaleSettings_t{ eWantedFilter, eWantedScaler };
}

extern GamescopeUpscaleFilter g_wantedUpscaleFilter;
extern GamescopeUpscaleScaler g_wantedUpscaleScaler;
extern int g_upscaleFilterSharpness;

extern bool g_bBorderlessOutputWindow;

extern bool g_bExposeWayland;

extern bool g_bRt;

extern int g_nXWaylandCount;
extern bool g_bNoTouchPointerEmulation;

extern uint32_t g_preferVendorID;
extern uint32_t g_preferDeviceID;

