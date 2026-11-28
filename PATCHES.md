# Patches in this fork

This fork carries a patch stack on top of upstream
[gamescope](https://github.com/ValveSoftware/gamescope). Most of the patches
target the nested Wayland backend, that is, gamescope running as a window on a
wlroots compositor such as sway. The DRM and OpenVR backends are not tested
here.

Branches:

- `patched/full-stack` is upstream plus every patch below, in order.
- `topic/<name>` is one patch applied to upstream together with the patches
  it depends on. Bug fixes and carried upstream pull requests come first in
  the stack and depend only on each other, so their topics are suitable for
  an upstream pull request; a local patch's topic may carry other patches.
- `master` is the upstream commit the stack is based on, plus this page and
  the README note that links to it.

Each patch is one of five kinds:

- **nested**: behaviour of gamescope nested under a Wayland host.
- **compat**: a workaround for a specific piece of software (lsfg-vk, GTK3).
- **feature**: a new command-line option.
- **fix**: an upstream bug, in shared code or in one backend.
- **build**: build system and test infrastructure.

This page is generated from the `Patch-*` trailers of each commit on
`patched/full-stack`, so it describes the stack as published.

The stack has 45 patches on upstream commit `7ed5e25f (2026-11-27, backends/waylandbackend: support windows bt2100 image descriptions)`.

## Index

| # | Patch | Kind | Source |
|---|-------|------|--------|
| 01 | [`patch-tests`](#patch-tests) | build | this fork |
| 02 | [`rendervulkan-private-booleans`](#rendervulkan-private-booleans) | fix | [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen) |
| 03 | [`held-button-focus`](#held-button-focus) | fix | this fork |
| 04 | [`confine-region`](#confine-region) | fix | this fork |
| 05 | [`presentation-feedback-cancel`](#presentation-feedback-cancel) | fix | this fork |
| 06 | [`modifier-forwarding`](#modifier-forwarding) | fix | [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes |
| 07 | [`format32-long`](#format32-long) | fix | this fork |
| 08 | [`fullscreen-null-connector`](#fullscreen-null-connector) | fix | this fork |
| 09 | [`fast-math-scope`](#fast-math-scope) | fix | [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked |
| 10 | [`icon-validation`](#icon-validation) | fix | this fork |
| 11 | [`atomic-output-globals`](#atomic-output-globals) | fix | this fork |
| 12 | [`null-fb-present`](#null-fb-present) | fix | this fork |
| 13 | [`subsurface-scale`](#subsurface-scale) | fix | this fork |
| 14 | [`image-description-leak`](#image-description-leak) | fix | this fork |
| 15 | [`image-description-ready`](#image-description-ready) | fix | this fork |
| 16 | [`pipewire-capture-pacing`](#pipewire-capture-pacing) | fix | this fork |
| 17 | [`wayland-client-buffer`](#wayland-client-buffer) | fix | Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400 |
| 18 | [`modifier-max-extent`](#modifier-max-extent) | fix | [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked |
| 19 | [`ewmh-maximize-desktop`](#ewmh-maximize-desktop) | compat | [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), the part upstream's 7d957c51 left out |
| 20 | [`cursor-surface-scale`](#cursor-surface-scale) | nested | [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked |
| 21 | [`pipewire-reconnect`](#pipewire-reconnect) | fix | [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked |
| 22 | [`pipewire-failed-buffers`](#pipewire-failed-buffers) | fix | [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked |
| 23 | [`pipewire-shutdown`](#pipewire-shutdown) | fix | this fork |
| 24 | [`proc-stat-parse`](#proc-stat-parse) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part |
| 25 | [`ordered-shutdown`](#ordered-shutdown) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked |
| 26 | [`wayland-error-shutdown`](#wayland-error-shutdown) | nested | [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked |
| 27 | [`wsi-flagged-queues`](#wsi-flagged-queues) | fix | https://github.com/Joshua-Ashton/vkroots/pull/17 |
| 28 | [`adaptive-sync-fifo-cap`](#adaptive-sync-fifo-cap) | fix | this fork |
| 29 | [`color-manager-gate`](#color-manager-gate) | nested | this fork |
| 30 | [`content-driven-hdr`](#content-driven-hdr) | nested | this fork |
| 31 | [`track-app-size`](#track-app-size) | feature | this fork |
| 32 | [`output-ring-gate`](#output-ring-gate) | nested | this fork |
| 33 | [`keep-toplevel-mapped`](#keep-toplevel-mapped) | nested | this fork |
| 34 | [`exit-when-empty`](#exit-when-empty) | feature | this fork |
| 35 | [`tearing-hints`](#tearing-hints) | nested | this fork |
| 36 | [`content-type`](#content-type) | nested | this fork |
| 37 | [`pointer-focus-churn`](#pointer-focus-churn) | nested | this fork |
| 38 | [`host-selection-sync`](#host-selection-sync) | nested | this fork |
| 39 | [`host-xtest-mouse`](#host-xtest-mouse) | nested | this fork |
| 40 | [`adaptive-sync-launch-tag`](#adaptive-sync-launch-tag) | nested | this fork |
| 41 | [`pipewire-capture-limits`](#pipewire-capture-limits) | feature | this fork |
| 42 | [`pipewire-device-local`](#pipewire-device-local) | feature | this fork |
| 43 | [`pipewire-async-capture`](#pipewire-async-capture) | feature | this fork |
| 44 | [`hdr-offer-from-host`](#hdr-offer-from-host) | nested | this fork |
| 45 | [`capture-tonemap`](#capture-tonemap) | feature | this fork |

## Runtime switches

Patches not listed here have no switch and change behaviour unconditionally.

| Switch | Effect | Patch |
|--------|--------|-------|
| `shutdown_grace_period_ms=<ms>` | how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once | [`ordered-shutdown`](#ordered-shutdown) |
| `--track-app-size` | opt-in | [`track-app-size`](#track-app-size) |
| `track_app_size_debug=1` | logs viewport changes | [`track-app-size`](#track-app-size) |
| `wayland_output_ring_backpressure=0` | disables the gate | [`output-ring-gate`](#output-ring-gate) |
| `--exit-when-empty[=seconds]` | opt-in; the default grace period is 30 seconds | [`exit-when-empty`](#exit-when-empty) |
| `GAMESCOPE_WAYLAND_XTEST_MOUSE=1` | opt-in, read at startup | [`host-xtest-mouse`](#host-xtest-mouse) |
| `--adaptive-sync` | tag the outer Wayland toplevel at creation. | [`adaptive-sync-launch-tag`](#adaptive-sync-launch-tag) |
| `--pipewire-capture-size WIDTHxHEIGHT` | bound the advertised capture size while preserving aspect ratio. | [`pipewire-capture-limits`](#pipewire-capture-limits) |
| `--pipewire-capture-fps FPS` | cap the capture cadence without changing display refresh. | [`pipewire-capture-limits`](#pipewire-capture-limits) |
| `gamescope_pipewire_device_local=1` | prefer device-local PipeWire DMA-BUF capture textures. | [`pipewire-device-local`](#pipewire-device-local) |
| `capture_sdr_white_nits` | luminance in nits shown as white (default 500). | [`capture-tonemap`](#capture-tonemap) |
| `capture_hdr_peak_nits` | source peak for the BT.2390 roll-off; 0 disables it. | [`capture-tonemap`](#capture-tonemap) |
| `capture_tonemap_operator` | 1 luma, 2 independent, 3 max channel (default). | [`capture-tonemap`](#capture-tonemap) |

## Nested Wayland backend

### cursor-surface-scale

Patch 20, [`5095de26`](https://github.com/KogasaPls/gamescope/commit/5095de261f68e019ff8561929d5bac5ecda9f933): WaylandBackend: reuse the cursor surface and scale it to the host's fractional scale

Releases cursor buffers on the host's release event and reuses the nested cursor surface, scaling it with a viewport so it stays sharp at fractional scales.

- **Source:** [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked.
- **Upstream issues:** [#2428](https://github.com/ValveSoftware/gamescope/issues/2428), [#2432](https://github.com/ValveSoftware/gamescope/issues/2432).
- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/cursor-surface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/cursor-surface-scale).

### wayland-error-shutdown

Patch 26, [`2e71902d`](https://github.com/KogasaPls/gamescope/commit/2e71902da0ed1d59eb4fd1f0603c5654ba6db109): WaylandBackend: shut down instead of aborting on host Wayland errors

Raises SIGTERM and stops the input thread when the host Wayland connection fails or hangs up, instead of aborting with a core dump, so the launched game is asked to exit before Xwayland goes away. The buffers the dead host held are released, so clients keep presenting during the grace period.

- **Source:** [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked.
- **Depends on:** [`ordered-shutdown`](#ordered-shutdown).
- **Standalone branch:** [`topic/wayland-error-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-error-shutdown).

### color-manager-gate

Patch 29, [`0cd4ad2e`](https://github.com/KogasaPls/gamescope/commit/0cd4ad2eee87a2cf11e43f43768cc4561791d38c): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Depends on:** [`patch-tests`](#patch-tests), [`icon-validation`](#icon-validation), [`image-description-ready`](#image-description-ready), [`cursor-surface-scale`](#cursor-surface-scale).
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 30, [`6946344a`](https://github.com/KogasaPls/gamescope/commit/6946344ad6f290bf9d8b1b01abba564abb141d14): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR. On every backend without a Vulkan swapchain, an HDR switch forces a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 32, [`a992f8f7`](https://github.com/KogasaPls/gamescope/commit/a992f8f7b74d24ce2d3d7b2668956e747f9efab6): WaylandBackend: gate the output ring on host buffer ownership

By default composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`subsurface-scale`](#subsurface-scale), [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size), [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 33, [`003f0c91`](https://github.com/KogasaPls/gamescope/commit/003f0c91532829614b9d1ec846cbc6d0410fed06): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 35, [`c95c0f7c`](https://github.com/KogasaPls/gamescope/commit/c95c0f7cbd2d40e7af22641ba362698a7179877b): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`image-description-leak`](#image-description-leak), [`color-manager-gate`](#color-manager-gate), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 36, [`fbfb9c8b`](https://github.com/KogasaPls/gamescope/commit/fbfb9c8b84d0e23e77e338173edf66b507cde096): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 37, [`bf98fb20`](https://github.com/KogasaPls/gamescope/commit/bf98fb20d8ae3b684c905bcbba51c72f7ebae986): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`held-button-focus`](#held-button-focus), [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`wayland-error-shutdown`](#wayland-error-shutdown), [`output-ring-gate`](#output-ring-gate), [`content-type`](#content-type).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 38, [`ba054af2`](https://github.com/KogasaPls/gamescope/commit/ba054af2b8293e6537712c28f339f64f4719b136): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control; host offers are read eagerly. A host without data control keeps upstream's outbound seat path. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`patch-tests`](#patch-tests), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

### host-xtest-mouse

Patch 39, [`5468eed5`](https://github.com/KogasaPls/gamescope/commit/5468eed54f2e44b5dcd8bee75cc2f225c5bf7f65): WaylandBackend: bridge host XTEST mouse and keyboard input

Experimental opt-in host XTEST mouse and keyboard bridge for external Steam Input on the Wayland backend. Captures XTEST devices while focused and forwards relative motion and held keys, with a composited cursor for ungrabbed mouse movement.

- **Switch:** `GAMESCOPE_WAYLAND_XTEST_MOUSE=1`: opt-in, read at startup.
- **Depends on:** [`patch-tests`](#patch-tests), [`modifier-forwarding`](#modifier-forwarding), [`wayland-client-buffer`](#wayland-client-buffer), [`pointer-focus-churn`](#pointer-focus-churn), [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/host-xtest-mouse`](https://github.com/KogasaPls/gamescope/tree/topic/host-xtest-mouse).

### adaptive-sync-launch-tag

Patch 40, [`0588c6b1`](https://github.com/KogasaPls/gamescope/commit/0588c6b194fe55c82bee5d5f12def6887595f912): WaylandBackend: tag adaptive-sync launches

Tag native Wayland toplevels with adaptive-sync when adaptive sync is enabled at window creation, letting compositor rules opt in only requesting launches. Runtime preference changes do not retag the window.

- **Switch:** `--adaptive-sync`: tag the outer Wayland toplevel at creation..
- **Depends on:** [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/adaptive-sync-launch-tag`](https://github.com/KogasaPls/gamescope/tree/topic/adaptive-sync-launch-tag).

### hdr-offer-from-host

Patch 44, [`32accd03`](https://github.com/KogasaPls/gamescope/commit/32accd03f8202e522848096c3d48a43189d709ce): WaylandBackend: offer HDR to clients from the host's capability

Offers HDR to clients, through the WSI layer's feedback property and DXVK_HDR, whenever --hdr-enabled is set and the nested host accepts HDR10, instead of from the host's first preferred image description, which can describe SDR for the first second.

- **Depends on:** [`patch-tests`](#patch-tests), [`format32-long`](#format32-long), [`color-manager-gate`](#color-manager-gate), [`host-xtest-mouse`](#host-xtest-mouse).
- **Standalone branch:** [`topic/hdr-offer-from-host`](https://github.com/KogasaPls/gamescope/tree/topic/hdr-offer-from-host).

## Compatibility with specific software

### ewmh-maximize-desktop

Patch 19, [`6d667280`](https://github.com/KogasaPls/gamescope/commit/6d6672809c3cbb2c9d6baef7dafca10b84763e36): steamcompmgr: fill the output with maximized desktop windows

Makes maximized desktop windows (no app ID) that set size hints fill the output, as upstream already does for windows with an app ID.

- **Source:** [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), the part upstream's 7d957c51 left out.
- **Standalone branch:** [`topic/ewmh-maximize-desktop`](https://github.com/KogasaPls/gamescope/tree/topic/ewmh-maximize-desktop).

## New options

### track-app-size

Patch 31, [`46301246`](https://github.com/KogasaPls/gamescope/commit/46301246d7b0e96be32bca28b76f943077dce490): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Depends on:** [`patch-tests`](#patch-tests), [`null-fb-present`](#null-fb-present), [`pipewire-shutdown`](#pipewire-shutdown).
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 34, [`1658ce11`](https://github.com/KogasaPls/gamescope/commit/1658ce11f069c0821ab9624f4b23a0d3ad42fc8e): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once every connector has lacked a displayable focus window for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`patch-tests`](#patch-tests), [`ordered-shutdown`](#ordered-shutdown), [`track-app-size`](#track-app-size), [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

### pipewire-capture-limits

Patch 41, [`0c29dd77`](https://github.com/KogasaPls/gamescope/commit/0c29dd7706ab3a2d977785b60e6cbabc1d003e17): pipewire: add opt-in capture size and rate limits

Adds launch limits for PipeWire capture size and rate independently of game resolution and refresh.

- **Switch:** `--pipewire-capture-size WIDTHxHEIGHT`: bound the advertised capture size while preserving aspect ratio..
- **Switch:** `--pipewire-capture-fps FPS`: cap the capture cadence without changing display refresh..
- **Depends on:** [`pipewire-capture-pacing`](#pipewire-capture-pacing), [`pipewire-shutdown`](#pipewire-shutdown), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/pipewire-capture-limits`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-limits).

### pipewire-device-local

Patch 42, [`5a128145`](https://github.com/KogasaPls/gamescope/commit/5a128145a389925d263385bb7e543356c10cefc9): pipewire: restore opt-in device-local capture buffers

Allows direct PipeWire DMA-BUF capture to use device-local memory with a host-visible fallback.

- **Switch:** `gamescope_pipewire_device_local=1`: prefer device-local PipeWire DMA-BUF capture textures..
- **Depends on:** [`pipewire-failed-buffers`](#pipewire-failed-buffers), [`pipewire-capture-limits`](#pipewire-capture-limits).
- **Standalone branch:** [`topic/pipewire-device-local`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-device-local).

### pipewire-async-capture

Patch 43, [`ff5776ae`](https://github.com/KogasaPls/gamescope/commit/ff5776ae986ee0036cdfe68f8e767ed68a8be761): pipewire: publish capture after GPU completion without blocking display

Lets the PipeWire worker publish completed capture frames while the compositor continues processing display events.

- **Depends on:** [`pipewire-capture-limits`](#pipewire-capture-limits).
- **Standalone branch:** [`topic/pipewire-async-capture`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-async-capture).

### capture-tonemap

Patch 45, [`90981075`](https://github.com/KogasaPls/gamescope/commit/90981075fa3594a16f54567988e11996f280de7b): steamcompmgr: make HDR to SDR capture tone mapping configurable

Makes the HDR to SDR mapping of PipeWire capture and SDR screenshots configurable: the luminance shown as white, and an optional BT.2390 roll-off of highlights from a given peak instead of clipping above 500 nits.

- **Switch:** `capture_sdr_white_nits`: luminance in nits shown as white (default 500)..
- **Switch:** `capture_hdr_peak_nits`: source peak for the BT.2390 roll-off; 0 disables it..
- **Switch:** `capture_tonemap_operator`: 1 luma, 2 independent, 3 max channel (default)..
- **Depends on:** [`patch-tests`](#patch-tests), [`hdr-offer-from-host`](#hdr-offer-from-host), [`pipewire-reconnect`](#pipewire-reconnect), [`pipewire-device-local`](#pipewire-device-local).
- **Standalone branch:** [`topic/capture-tonemap`](https://github.com/KogasaPls/gamescope/tree/topic/capture-tonemap).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`19fedef2`](https://github.com/KogasaPls/gamescope/commit/19fedef2e12f7669bd5e877df3eb0183c655a0b8): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### held-button-focus

Patch 03, [`38ea440a`](https://github.com/KogasaPls/gamescope/commit/38ea440a0f927386f419ea082f0e0391579e2172): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 04, [`f0041dee`](https://github.com/KogasaPls/gamescope/commit/f0041deeaee2c9fa7d0108bc0d76a6d51a8192a2): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 05, [`67cdc3ba`](https://github.com/KogasaPls/gamescope/commit/67cdc3ba81ad96dc8fcd2b849e29c77f88c1451a): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 06, [`7ba4dc9d`](https://github.com/KogasaPls/gamescope/commit/7ba4dc9d88e8621275c8fb95e1812b72cbdba18d): wlserver: apply host modifiers through the group keyboard

Applies host modifiers through the group's keyboard and maps modifiers and layouts by name across host and nested keymaps.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### format32-long

Patch 07, [`840c5130`](https://github.com/KogasaPls/gamescope/commit/840c5130fe3cec367a0d1a6e085d48f39ed48be6): steamcompmgr: write format-32 properties from long arrays

Supplies Xlib format-32 properties as arrays of long, correcting array stride and scalar overreads on LP64.

- **Standalone branch:** [`topic/format32-long`](https://github.com/KogasaPls/gamescope/tree/topic/format32-long).

### fullscreen-null-connector

Patch 08, [`799434cd`](https://github.com/KogasaPls/gamescope/commit/799434cd56bf5fc23e624d9d3dad520e53fb5981): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 09, [`616ddb23`](https://github.com/KogasaPls/gamescope/commit/616ddb23463c22452c7ac9c6c2be4c78287749c8): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. The float sanitising guards are widened to cover -ffinite-math-only by testing the value of __FINITE_MATH_ONLY__, which GCC and Clang always define.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 10, [`b39e27e1`](https://github.com/KogasaPls/gamescope/commit/b39e27e175e25fe664c363e5fe84f26fd61ed0cd): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 11, [`290d2104`](https://github.com/KogasaPls/gamescope/commit/290d2104ac0c3f266b0e89a5980213aff4dc13fa): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 12, [`78b19b7c`](https://github.com/KogasaPls/gamescope/commit/78b19b7cb75312b91f6813863f12ff5ce1a75b85): WaylandBackend: present no buffer for a texture without an fb

Composites a frame whose layer has no backend framebuffer, such as an override blit image in a format the backend cannot scan out, and presents no buffer for such a texture instead of dereferencing null.

- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 13, [`085d2bc3`](https://github.com/KogasaPls/gamescope/commit/085d2bc3510e019e0b46d8d781ed91e41a7e82ac): WaylandBackend: scale subsurface positions as signed values

Scales subsurface positions as signed values, so a layer placed left of or above the nested output is no longer sent to the host as a large positive position. The fractional-scale conversions move into a tested helper with 64-bit arithmetic.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### image-description-leak

Patch 14, [`3c7ec90b`](https://github.com/KogasaPls/gamescope/commit/3c7ec90bbeb0cb71e562f16c2711b93533d6fa52): WaylandBackend: release the current image description with its plane

Destroys a plane's current wp_image_description_v1 when the plane is torn down, which leaked.

- **Standalone branch:** [`topic/image-description-leak`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-leak).

### image-description-ready

Patch 15, [`989d46ed`](https://github.com/KogasaPls/gamescope/commit/989d46ed6a5667938b6cd1eaa4d6d07e7711c7e6): WaylandBackend: wait for image descriptions to become ready

Waits for a wp_image_description_v1's ready event before setting it on a surface or querying it, as wp_color_management_v1 requires, and presents without one when the host answers failed instead of raising a fatal protocol error.

- **Standalone branch:** [`topic/image-description-ready`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-ready).

### pipewire-capture-pacing

Patch 16, [`d4451ae3`](https://github.com/KogasaPls/gamescope/commit/d4451ae33da6eeac80532544ed368b593deef049): pipewire: pace capture to the negotiated framerate

Paces video capture to the consumer's negotiated framerate, independently of display refresh, and logs skipped capture attempts at debug severity when no buffer is free.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/pipewire-capture-pacing`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-pacing).

### wayland-client-buffer

Patch 17, [`cd0737d3`](https://github.com/KogasaPls/gamescope/commit/cd0737d38dbcce48f733be208f61154e02835d0d): wlserver: allow bounded Wayland event bursts up to 1 MiB

Allow bounded input-event bursts up to 1 MiB so a temporarily busy nested Xwayland is not disconnected at the 4 KiB default limit.

- **Source:** Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400.
- **Obsolete when:** Upstream configures a suitable bounded Wayland client event buffer.
- **Standalone branch:** [`topic/wayland-client-buffer`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-client-buffer).

### modifier-max-extent

Patch 18, [`37399666`](https://github.com/KogasaPls/gamescope/commit/3739966605aa4a653fde2b0856e5573302da7f62): rendervulkan: skip modifiers whose max extent is smaller than the image

Skip modifiers whose maximum image extent is smaller than the image when allocating flippable images, so a 4K nested output on RDNA no longer goes black with a modifier the driver cannot use at that size. Fail the allocation with an error when no modifier fits.

- **Source:** [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked.
- **Upstream issues:** [#1977](https://github.com/ValveSoftware/gamescope/issues/1977), [#2013](https://github.com/ValveSoftware/gamescope/issues/2013).
- **Standalone branch:** [`topic/modifier-max-extent`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-max-extent).

### pipewire-reconnect

Patch 21, [`e340d9c3`](https://github.com/KogasaPls/gamescope/commit/e340d9c37d73786846d6b08232f495a6d74e4ad1): pipewire: reconnect the stream instead of exiting when it goes away

Reconnects the PipeWire stream when its node is destroyed or the stream errors, so consumers that find the node by name can capture again (a PipeWire daemon restart and clients holding the node id from gamescope_pipewire are not covered), and closes the races between steamcompmgr's buffer dequeue and buffer removal.

- **Source:** [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked.
- **Upstream issues:** [#2508](https://github.com/ValveSoftware/gamescope/issues/2508).
- **Depends on:** [`pipewire-capture-pacing`](#pipewire-capture-pacing).
- **Standalone branch:** [`topic/pipewire-reconnect`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-reconnect).

### pipewire-failed-buffers

Patch 22, [`076cfc68`](https://github.com/KogasaPls/gamescope/commit/076cfc6873df25d958d00ccdaa02fc3386f5a975): pipewire: don't crash when add_buffer fails

Keeps capture running when a PipeWire buffer fails to initialize, withholding that buffer from the consumer instead of dereferencing its missing state.

- **Source:** [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked.
- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-failed-buffers`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-failed-buffers).

### pipewire-shutdown

Patch 23, [`5762d664`](https://github.com/KogasaPls/gamescope/commit/5762d6640c46f747d0315e5f0394f5447da8c8d3): pipewire: stop the stream thread before the backend is destroyed

Stops the PipeWire stream thread before the backend is deleted at exit, so a stream renegotiation at that moment no longer calls into a destroyed backend.

- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-shutdown).

### proc-stat-parse

Patch 24, [`b4bf397e`](https://github.com/KogasaPls/gamescope/commit/b4bf397ef02f616ca50a661e1015ba8cd9539ab6): Process: parse /proc stat after the command name and skip zombies

Parses /proc/<pid>/stat after the last ')' of the command name, so a child whose name contains a space is still found and signalled, and skips zombie children, which have already exited, while keeping a process whose main thread exited but other threads still run.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part.
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/proc-stat-parse`](https://github.com/KogasaPls/gamescope/tree/topic/proc-stat-parse).

### ordered-shutdown

Patch 25, [`1d3b4415`](https://github.com/KogasaPls/gamescope/commit/1d3b4415ae268142b47b3138e55f87314715e20e): main: signal children first and keep the display alive on SIGTERM

On SIGTERM, SIGINT, SIGHUP or SIGQUIT, asks the launched processes to exit first and keeps Xwayland running until they have or a grace period passes, then kills what is left and shuts down; a second signal forces it. Closing the nested window over a Proton game no longer pulls the X server out from under it.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked.
- **Upstream issues:** [#885](https://github.com/ValveSoftware/gamescope/issues/885).
- **Switch:** `shutdown_grace_period_ms=<ms>`: how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once.
- **Depends on:** [`patch-tests`](#patch-tests), [`proc-stat-parse`](#proc-stat-parse).
- **Standalone branch:** [`topic/ordered-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/ordered-shutdown).

### wsi-flagged-queues

Patch 27, [`1e5a4ac6`](https://github.com/KogasaPls/gamescope/commit/1e5a4ac6b9c87acc3736834d272801e3ab348faa): layer: backport flag-aware queue dispatch to Gamescope's vkroots header

Builds the WSI layer with the flagged-queue fix applied to Gamescope's selected vkroots header. This keeps Vulkan clients such as mpv from losing the queue dispatch on their first present.

- **Source:** https://github.com/Joshua-Ashton/vkroots/pull/17.
- **Obsolete when:** Gamescope pins vkroots with flag-aware queue registration and teardown.
- **Standalone branch:** [`topic/wsi-flagged-queues`](https://github.com/KogasaPls/gamescope/tree/topic/wsi-flagged-queues).

### adaptive-sync-fifo-cap

Patch 28, [`7df06bce`](https://github.com/KogasaPls/gamescope/commit/7df06bce974131b75d0ebec029aa3405516bceba): steamcompmgr: hold FIFO commits to one latch per refresh with adaptive sync

Holds a window's FIFO commits to one latch per refresh cycle while adaptive sync is active. Without it a vsync'd client runs far above the refresh rate, and lsfg-vk's generated frames are not paced at all.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/adaptive-sync-fifo-cap`](https://github.com/KogasaPls/gamescope/tree/topic/adaptive-sync-fifo-cap).

## Build and tests

### patch-tests

Patch 01, [`ccd41124`](https://github.com/KogasaPls/gamescope/commit/ccd411248dfb0afd077b4ff4c83a4295a5b6ef27): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build. Also adds a startup test that runs the built gamescope with --version, which catches defects that exist only in the linked binary, such as a log scope registered twice aborting before main.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
