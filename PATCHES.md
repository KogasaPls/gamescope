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

The stack has 45 patches on upstream commit `3738994f (2026-11-18, steamcompmgr, OpenVRBackend: click instead of touch in nested popups)`.

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
| 19 | [`ewmh-maximize`](#ewmh-maximize) | compat | [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), reworked |
| 20 | [`cursor-surface-scale`](#cursor-surface-scale) | nested | [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked |
| 21 | [`pipewire-reconnect`](#pipewire-reconnect) | fix | [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked |
| 22 | [`pipewire-failed-buffers`](#pipewire-failed-buffers) | fix | [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked |
| 23 | [`pipewire-shutdown`](#pipewire-shutdown) | fix | this fork |
| 24 | [`proc-stat-parse`](#proc-stat-parse) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part |
| 25 | [`ordered-shutdown`](#ordered-shutdown) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked |
| 26 | [`wayland-error-shutdown`](#wayland-error-shutdown) | nested | [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked |
| 27 | [`wsi-flagged-queues`](#wsi-flagged-queues) | fix | this fork |
| 28 | [`color-manager-gate`](#color-manager-gate) | nested | this fork |
| 29 | [`content-driven-hdr`](#content-driven-hdr) | nested | this fork |
| 30 | [`track-app-size`](#track-app-size) | feature | this fork |
| 31 | [`output-ring-gate`](#output-ring-gate) | nested | this fork |
| 32 | [`keep-toplevel-mapped`](#keep-toplevel-mapped) | nested | this fork |
| 33 | [`exit-when-empty`](#exit-when-empty) | feature | this fork |
| 34 | [`tearing-hints`](#tearing-hints) | nested | this fork |
| 35 | [`content-type`](#content-type) | nested | this fork |
| 36 | [`fifo-passthrough`](#fifo-passthrough) | compat | this fork |
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
| `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1` | opt-in, set in the client environment | [`fifo-passthrough`](#fifo-passthrough) |
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

Patch 20, [`9c88986c`](https://github.com/KogasaPls/gamescope/commit/9c88986c35854783b1e96b908e7190394296b4f6): WaylandBackend: reuse the cursor surface and scale it to the host's fractional scale

Releases cursor buffers on the host's release event and reuses the nested cursor surface, scaling it with a viewport so it stays sharp at fractional scales.

- **Source:** [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked.
- **Upstream issues:** [#2428](https://github.com/ValveSoftware/gamescope/issues/2428), [#2432](https://github.com/ValveSoftware/gamescope/issues/2432).
- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/cursor-surface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/cursor-surface-scale).

### wayland-error-shutdown

Patch 26, [`d2e18c63`](https://github.com/KogasaPls/gamescope/commit/d2e18c639ae94a57e38c678ef782bac593920bfa): WaylandBackend: shut down instead of aborting on host Wayland errors

Raises SIGTERM and stops the input thread when the host Wayland connection fails or hangs up, instead of aborting with a core dump, so the launched game is asked to exit before Xwayland goes away. The buffers the dead host held are released, so clients keep presenting during the grace period.

- **Source:** [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked.
- **Depends on:** [`ordered-shutdown`](#ordered-shutdown).
- **Standalone branch:** [`topic/wayland-error-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-error-shutdown).

### color-manager-gate

Patch 28, [`16f01063`](https://github.com/KogasaPls/gamescope/commit/16f010632bfca83b80d3160712daf1f4c22470c9): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Depends on:** [`patch-tests`](#patch-tests), [`icon-validation`](#icon-validation), [`image-description-ready`](#image-description-ready), [`cursor-surface-scale`](#cursor-surface-scale).
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 29, [`4d81051f`](https://github.com/KogasaPls/gamescope/commit/4d81051f1301977502ee02626dc42676cd4d39a3): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR. On every backend without a Vulkan swapchain, an HDR switch forces a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 31, [`77a1ceb1`](https://github.com/KogasaPls/gamescope/commit/77a1ceb1648400e7241d53604e026e0866d2e1b3): WaylandBackend: gate the output ring on host buffer ownership

Composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`subsurface-scale`](#subsurface-scale), [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 32, [`8821f2bb`](https://github.com/KogasaPls/gamescope/commit/8821f2bbc0e02c96e67411398df9c4805dfc1815): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 34, [`7ae9ae3e`](https://github.com/KogasaPls/gamescope/commit/7ae9ae3eb5b3b7d68c6c9d9bede02d226681e57c): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`image-description-leak`](#image-description-leak), [`color-manager-gate`](#color-manager-gate), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 35, [`0a7a0ba6`](https://github.com/KogasaPls/gamescope/commit/0a7a0ba6c767a9f8360358233b3d653c6209e405): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 37, [`10526771`](https://github.com/KogasaPls/gamescope/commit/1052677156f63a1f9ccb8bd98c98852934f88c02): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`held-button-focus`](#held-button-focus), [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`wayland-error-shutdown`](#wayland-error-shutdown), [`output-ring-gate`](#output-ring-gate), [`content-type`](#content-type).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 38, [`dba30624`](https://github.com/KogasaPls/gamescope/commit/dba306249148a1a59ca5f8f33c24535c89d453ab): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control; host offers are read eagerly. A host without data control keeps upstream's outbound seat path. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`patch-tests`](#patch-tests), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

### host-xtest-mouse

Patch 39, [`563c5b2b`](https://github.com/KogasaPls/gamescope/commit/563c5b2bb1f8d39be6e80b4c6abd7d03041f3645): WaylandBackend: bridge host XTEST mouse and keyboard input

Experimental opt-in host XTEST mouse and keyboard bridge for external Steam Input on the Wayland backend. Captures XTEST devices while focused and forwards relative motion and held keys, with a composited cursor for ungrabbed mouse movement.

- **Switch:** `GAMESCOPE_WAYLAND_XTEST_MOUSE=1`: opt-in, read at startup.
- **Depends on:** [`patch-tests`](#patch-tests), [`modifier-forwarding`](#modifier-forwarding), [`wayland-client-buffer`](#wayland-client-buffer), [`pointer-focus-churn`](#pointer-focus-churn), [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/host-xtest-mouse`](https://github.com/KogasaPls/gamescope/tree/topic/host-xtest-mouse).

### adaptive-sync-launch-tag

Patch 40, [`5d141f51`](https://github.com/KogasaPls/gamescope/commit/5d141f517018f0a67d8879904a1dff1efe0571ac): WaylandBackend: tag adaptive-sync launches

Tag native Wayland toplevels with adaptive-sync when adaptive sync is enabled at window creation, letting compositor rules opt in only requesting launches. Runtime preference changes do not retag the window.

- **Switch:** `--adaptive-sync`: tag the outer Wayland toplevel at creation..
- **Depends on:** [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/adaptive-sync-launch-tag`](https://github.com/KogasaPls/gamescope/tree/topic/adaptive-sync-launch-tag).

### hdr-offer-from-host

Patch 44, [`aeb5292e`](https://github.com/KogasaPls/gamescope/commit/aeb5292e777716715da4dc9d677e97b1f0d1d358): WaylandBackend: offer HDR to clients from the host's capability

Offers HDR to clients, through the WSI layer's feedback property and DXVK_HDR, whenever --hdr-enabled is set and the nested host accepts HDR10, instead of from the host's first preferred image description, which can describe SDR for the first second.

- **Depends on:** [`patch-tests`](#patch-tests), [`format32-long`](#format32-long), [`color-manager-gate`](#color-manager-gate), [`host-xtest-mouse`](#host-xtest-mouse).
- **Standalone branch:** [`topic/hdr-offer-from-host`](https://github.com/KogasaPls/gamescope/tree/topic/hdr-offer-from-host).

## Compatibility with specific software

### ewmh-maximize

Patch 19, [`03873c17`](https://github.com/KogasaPls/gamescope/commit/03873c17d32a2f0260da3db5693d9f9830e19785): steamcompmgr: handle EWMH maximize

Handle the EWMH maximize states, so windows that maximize through _NET_WM_STATE, such as GTK3 apps, fill the output and get their previous size back when unmaximized.

- **Source:** [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), reworked.
- **Standalone branch:** [`topic/ewmh-maximize`](https://github.com/KogasaPls/gamescope/tree/topic/ewmh-maximize).

### fifo-passthrough

Patch 36, [`fce2b6fc`](https://github.com/KogasaPls/gamescope/commit/fce2b6fc6ad95d79f0972fcf11d22fa555ce94cd): layer: opt-in present mode passthrough for FIFO

Adds an opt-in that passes a client's FIFO present mode through to the driver instead of replacing it with MAILBOX. gamescope's own FIFO limits commits per vblank but never blocks vkQueuePresentKHR, so lsfg-vk, which paces its generated frames on that block, free-runs and drops frames.

- **Switch:** `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1`: opt-in, set in the client environment.
- **Obsolete when:** lsfg-vk gains a pacing mode other than pacing = "none", or gamescope gives nested clients driver-level FIFO back-pressure (for example through wp_fifo_v1).
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/fifo-passthrough`](https://github.com/KogasaPls/gamescope/tree/topic/fifo-passthrough).

## New options

### track-app-size

Patch 30, [`3b430d95`](https://github.com/KogasaPls/gamescope/commit/3b430d95fbaf013937ed00eb91b3296fb5f83b26): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Depends on:** [`patch-tests`](#patch-tests), [`null-fb-present`](#null-fb-present), [`pipewire-shutdown`](#pipewire-shutdown).
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 33, [`30af239e`](https://github.com/KogasaPls/gamescope/commit/30af239e95b54b381d8132df963e8ec70e546735): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once no client window has existed for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`patch-tests`](#patch-tests), [`ordered-shutdown`](#ordered-shutdown), [`track-app-size`](#track-app-size), [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

### pipewire-capture-limits

Patch 41, [`18878215`](https://github.com/KogasaPls/gamescope/commit/18878215177576e90cf56a5b83030d0745ca9fbf): pipewire: add opt-in capture size and rate limits

Adds launch limits for PipeWire capture size and rate independently of game resolution and refresh.

- **Switch:** `--pipewire-capture-size WIDTHxHEIGHT`: bound the advertised capture size while preserving aspect ratio..
- **Switch:** `--pipewire-capture-fps FPS`: cap the capture cadence without changing display refresh..
- **Depends on:** [`pipewire-capture-pacing`](#pipewire-capture-pacing), [`pipewire-shutdown`](#pipewire-shutdown), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/pipewire-capture-limits`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-limits).

### pipewire-device-local

Patch 42, [`0c581ae5`](https://github.com/KogasaPls/gamescope/commit/0c581ae5fc72119aafba0e0b76732d8582be0965): pipewire: restore opt-in device-local capture buffers

Allows direct PipeWire DMA-BUF capture to use device-local memory with a host-visible fallback.

- **Switch:** `gamescope_pipewire_device_local=1`: prefer device-local PipeWire DMA-BUF capture textures..
- **Depends on:** [`pipewire-failed-buffers`](#pipewire-failed-buffers), [`pipewire-capture-limits`](#pipewire-capture-limits).
- **Standalone branch:** [`topic/pipewire-device-local`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-device-local).

### pipewire-async-capture

Patch 43, [`0446277d`](https://github.com/KogasaPls/gamescope/commit/0446277d1f8cbdf68bb19c571f7e9066d750b454): pipewire: publish capture after GPU completion without blocking display

Lets the PipeWire worker publish completed capture frames while the compositor continues processing display events.

- **Depends on:** [`pipewire-capture-limits`](#pipewire-capture-limits).
- **Standalone branch:** [`topic/pipewire-async-capture`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-async-capture).

### capture-tonemap

Patch 45, [`c9268db3`](https://github.com/KogasaPls/gamescope/commit/c9268db34b0e964effabc673bddd66dfc1843335): steamcompmgr: make HDR to SDR capture tone mapping configurable

Makes the HDR to SDR mapping of PipeWire capture and SDR screenshots configurable: the luminance shown as white, and an optional BT.2390 roll-off of highlights from a given peak instead of clipping above 500 nits.

- **Switch:** `capture_sdr_white_nits`: luminance in nits shown as white (default 500)..
- **Switch:** `capture_hdr_peak_nits`: source peak for the BT.2390 roll-off; 0 disables it..
- **Switch:** `capture_tonemap_operator`: 1 luma, 2 independent, 3 max channel (default)..
- **Depends on:** [`patch-tests`](#patch-tests), [`hdr-offer-from-host`](#hdr-offer-from-host).
- **Standalone branch:** [`topic/capture-tonemap`](https://github.com/KogasaPls/gamescope/tree/topic/capture-tonemap).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`b9fc6d66`](https://github.com/KogasaPls/gamescope/commit/b9fc6d663e1decd356946abfc2008b1589c677ee): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### held-button-focus

Patch 03, [`2c66ef57`](https://github.com/KogasaPls/gamescope/commit/2c66ef57ac047f3042305c1d1c9ca59b0fecae0c): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 04, [`9c9d3ac8`](https://github.com/KogasaPls/gamescope/commit/9c9d3ac8fde3f153f6b77eafa0d35a38fa533c0c): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 05, [`4aaf262c`](https://github.com/KogasaPls/gamescope/commit/4aaf262c075f8297c4bf6a8a3df743897f750df8): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 06, [`44c53993`](https://github.com/KogasaPls/gamescope/commit/44c539933bf437701bc3d4bb9a75248807f81eb0): Forward Wayland modifier events to wlroots seat when nested

Forwards the host's keyboard modifier state into the nested session. Without it Xwayland clients see every key press with no Shift, Ctrl or Alt held.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### format32-long

Patch 07, [`3a15299b`](https://github.com/KogasaPls/gamescope/commit/3a15299b56755e3f96fa03f0d47724ab77dd35cb): steamcompmgr: write format-32 properties from long arrays

Writes every format-32 X property gamescope sets from an array of long, which is what Xlib reads. The 32-bit values it was given made Xlib read past them on 64-bit systems, publishing garbage in WM_STATE and the root window's feedback properties.

- **Standalone branch:** [`topic/format32-long`](https://github.com/KogasaPls/gamescope/tree/topic/format32-long).

### fullscreen-null-connector

Patch 08, [`094b2cbc`](https://github.com/KogasaPls/gamescope/commit/094b2cbc590074510647688c2e736701d758d062): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 09, [`70150ef4`](https://github.com/KogasaPls/gamescope/commit/70150ef413d81705ccd3f00151e7aac05a0f6339): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. The float sanitising guards are widened to cover -ffinite-math-only by testing the value of __FINITE_MATH_ONLY__, which GCC and Clang always define.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 10, [`8c02053b`](https://github.com/KogasaPls/gamescope/commit/8c02053bb080e55b1629235784b7ba01a3739fb5): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 11, [`2c9a8976`](https://github.com/KogasaPls/gamescope/commit/2c9a897699b3cea1bebce9b442ae9c48ecd10a73): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 12, [`1b5dc36f`](https://github.com/KogasaPls/gamescope/commit/1b5dc36f08db78e45ad456b61b50a3fe8a795cf0): WaylandBackend: present no buffer for a texture without an fb

Composites a frame whose layer has no backend framebuffer, such as an override blit image in a format the backend cannot scan out, and presents no buffer for such a texture instead of dereferencing null.

- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 13, [`01f91ccf`](https://github.com/KogasaPls/gamescope/commit/01f91ccfba76898b7a14a7b7d69b55121eba7536): WaylandBackend: scale subsurface positions as signed values

Scales subsurface positions as signed values, so a layer placed left of or above the nested output is no longer sent to the host as a large positive position. The fractional-scale conversions move into a tested helper with 64-bit arithmetic.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### image-description-leak

Patch 14, [`e35c0541`](https://github.com/KogasaPls/gamescope/commit/e35c0541b0641050c514320c505815531b9a3132): WaylandBackend: release the current image description with its plane

Destroys a plane's current wp_image_description_v1 when the plane is torn down, which leaked.

- **Standalone branch:** [`topic/image-description-leak`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-leak).

### image-description-ready

Patch 15, [`7578220f`](https://github.com/KogasaPls/gamescope/commit/7578220fb2616cb5bd117a5e4787587155ef4995): WaylandBackend: wait for image descriptions to become ready

Waits for a wp_image_description_v1's ready event before setting it on a surface or querying it, as wp_color_management_v1 requires, and presents without one when the host answers failed instead of raising a fatal protocol error.

- **Standalone branch:** [`topic/image-description-ready`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-ready).

### pipewire-capture-pacing

Patch 16, [`67d06a78`](https://github.com/KogasaPls/gamescope/commit/67d06a785325239b642587d5a4cf1322cbefa320): pipewire: pace capture to the negotiated framerate

Paces video capture to the consumer's negotiated framerate, independently of display refresh, and logs skipped capture attempts at debug severity when no buffer is free.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/pipewire-capture-pacing`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-pacing).

### wayland-client-buffer

Patch 17, [`b9c65989`](https://github.com/KogasaPls/gamescope/commit/b9c659891c74ba5654b06a34fbd31d6da160c485): wlserver: allow bounded Wayland event bursts up to 1 MiB

Allow bounded input-event bursts up to 1 MiB so a temporarily busy nested Xwayland is not disconnected at the 4 KiB default limit.

- **Source:** Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400.
- **Obsolete when:** Upstream configures a suitable bounded Wayland client event buffer.
- **Standalone branch:** [`topic/wayland-client-buffer`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-client-buffer).

### modifier-max-extent

Patch 18, [`9f44a03a`](https://github.com/KogasaPls/gamescope/commit/9f44a03a57a5f1ec7ba901140b7b25874d7b8265): rendervulkan: skip modifiers whose max extent is smaller than the image

Skip modifiers whose maximum image extent is smaller than the image when allocating flippable images, so a 4K nested output on RDNA no longer goes black with a modifier the driver cannot use at that size. Fail the allocation with an error when no modifier fits.

- **Source:** [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked.
- **Upstream issues:** [#1977](https://github.com/ValveSoftware/gamescope/issues/1977), [#2013](https://github.com/ValveSoftware/gamescope/issues/2013).
- **Standalone branch:** [`topic/modifier-max-extent`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-max-extent).

### pipewire-reconnect

Patch 21, [`c52b4097`](https://github.com/KogasaPls/gamescope/commit/c52b4097a0505ae21fef7ea6a954133989e64603): pipewire: reconnect the stream instead of exiting when it goes away

Reconnects the PipeWire stream when its node is destroyed or the stream errors, so consumers that find the node by name can capture again (a PipeWire daemon restart and clients holding the node id from gamescope_pipewire are not covered), and closes the races between steamcompmgr's buffer dequeue and buffer removal.

- **Source:** [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked.
- **Upstream issues:** [#2508](https://github.com/ValveSoftware/gamescope/issues/2508).
- **Depends on:** [`pipewire-capture-pacing`](#pipewire-capture-pacing).
- **Standalone branch:** [`topic/pipewire-reconnect`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-reconnect).

### pipewire-failed-buffers

Patch 22, [`4bdd05d0`](https://github.com/KogasaPls/gamescope/commit/4bdd05d0d85e5c0dc09ffaa1ddbc1b48805703fd): pipewire: don't crash when add_buffer fails

Keeps capture running when a PipeWire buffer fails to initialize, withholding that buffer from the consumer instead of dereferencing its missing state.

- **Source:** [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked.
- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-failed-buffers`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-failed-buffers).

### pipewire-shutdown

Patch 23, [`f135a218`](https://github.com/KogasaPls/gamescope/commit/f135a2185557d0e8f45c3b6fb2dfdcf4465cd9cb): pipewire: stop the stream thread before the backend is destroyed

Stops the PipeWire stream thread before the backend is deleted at exit, so a stream renegotiation at that moment no longer calls into a destroyed backend.

- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-shutdown).

### proc-stat-parse

Patch 24, [`80217cfe`](https://github.com/KogasaPls/gamescope/commit/80217cfe7110229a96088b8e0f2753fb385b999f): Process: parse /proc stat after the command name and skip zombies

Parses /proc/<pid>/stat after the last ')' of the command name, so a child whose name contains a space is still found and signalled, and skips zombie children, which have already exited, while keeping a process whose main thread exited but other threads still run.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part.
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/proc-stat-parse`](https://github.com/KogasaPls/gamescope/tree/topic/proc-stat-parse).

### ordered-shutdown

Patch 25, [`66fcfc23`](https://github.com/KogasaPls/gamescope/commit/66fcfc23dad23ed49eac61c52375254e05c699e4): main: signal children first and keep the display alive on SIGTERM

On SIGTERM, SIGINT, SIGHUP or SIGQUIT, asks the launched processes to exit first and keeps Xwayland running until they have or a grace period passes, then kills what is left and shuts down; a second signal forces it. Closing the nested window over a Proton game no longer pulls the X server out from under it.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked.
- **Upstream issues:** [#885](https://github.com/ValveSoftware/gamescope/issues/885).
- **Switch:** `shutdown_grace_period_ms=<ms>`: how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once.
- **Depends on:** [`patch-tests`](#patch-tests), [`proc-stat-parse`](#proc-stat-parse).
- **Standalone branch:** [`topic/ordered-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/ordered-shutdown).

### wsi-flagged-queues

Patch 27, [`8f323708`](https://github.com/KogasaPls/gamescope/commit/8f32370842c7279dfb3ebc7b0879c30702163d81): layer: give queues created with flags their dispatch

Fixes a crash on the first present of a Vulkan client that creates its queues with flags, such as mpv through libplacebo, by giving those queues an entry in the layer's dispatch tables.

- **Obsolete when:** vkroots registers queues created with flags through vkGetDeviceQueue2.
- **Standalone branch:** [`topic/wsi-flagged-queues`](https://github.com/KogasaPls/gamescope/tree/topic/wsi-flagged-queues).

## Build and tests

### patch-tests

Patch 01, [`135595db`](https://github.com/KogasaPls/gamescope/commit/135595db6e00afcf94e6a9ef2388b6ba61151f24): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build. Also adds a startup test that runs the built gamescope with --version, which catches defects that exist only in the linked binary, such as a log scope registered twice aborting before main.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
