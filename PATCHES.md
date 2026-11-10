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

The stack has 40 patches on upstream commit `89b9e6ee (2026-11-07, steamcompmgr, OpenVRBackend: subtract the cursor hotspot on nested backends again)`.

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

## Nested Wayland backend

### cursor-surface-scale

Patch 20, [`41b8e016`](https://github.com/KogasaPls/gamescope/commit/41b8e016ff7566477701c4b21a1b525cf66e645e): WaylandBackend: reuse the cursor surface and scale it to the host's fractional scale

Releases cursor buffers on the host's release event and reuses the nested cursor surface, scaling it with a viewport so it stays sharp at fractional scales.

- **Source:** [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked.
- **Upstream issues:** [#2428](https://github.com/ValveSoftware/gamescope/issues/2428), [#2432](https://github.com/ValveSoftware/gamescope/issues/2432).
- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/cursor-surface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/cursor-surface-scale).

### wayland-error-shutdown

Patch 26, [`e6ba11a9`](https://github.com/KogasaPls/gamescope/commit/e6ba11a987d389a1b1e57d7c24eb33f9e8ba241e): WaylandBackend: shut down instead of aborting on host Wayland errors

Raises SIGTERM and stops the input thread when the host Wayland connection fails or hangs up, instead of aborting with a core dump, so the launched game is asked to exit before Xwayland goes away. The buffers the dead host held are released, so clients keep presenting during the grace period.

- **Source:** [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked.
- **Depends on:** [`ordered-shutdown`](#ordered-shutdown).
- **Standalone branch:** [`topic/wayland-error-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-error-shutdown).

### color-manager-gate

Patch 28, [`43fe5569`](https://github.com/KogasaPls/gamescope/commit/43fe5569413e0322b8bdf633ea091b86b0da90e7): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Depends on:** [`patch-tests`](#patch-tests), [`icon-validation`](#icon-validation), [`image-description-ready`](#image-description-ready), [`cursor-surface-scale`](#cursor-surface-scale).
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 29, [`6fa440f7`](https://github.com/KogasaPls/gamescope/commit/6fa440f7c8e5ef3ecab3b79ea3e232645d6b0253): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR. On every backend without a Vulkan swapchain, an HDR switch forces a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 31, [`7c430f6d`](https://github.com/KogasaPls/gamescope/commit/7c430f6daac35c4f3a60b6ecb3a91bf19484802e): WaylandBackend: gate the output ring on host buffer ownership

Composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`subsurface-scale`](#subsurface-scale), [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 32, [`834177cb`](https://github.com/KogasaPls/gamescope/commit/834177cbb513c7fed568735db8ca3e937358944a): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 34, [`3d2bee31`](https://github.com/KogasaPls/gamescope/commit/3d2bee31fa7a942b4575ae231d78d70d899edfe7): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`image-description-leak`](#image-description-leak), [`color-manager-gate`](#color-manager-gate), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 35, [`8ce50b4e`](https://github.com/KogasaPls/gamescope/commit/8ce50b4e4823a77321489f482f94d057f9bce77b): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 37, [`b9ed27e6`](https://github.com/KogasaPls/gamescope/commit/b9ed27e689260ee91001a6b3c32e10e2d5e0981b): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`held-button-focus`](#held-button-focus), [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`wayland-error-shutdown`](#wayland-error-shutdown), [`output-ring-gate`](#output-ring-gate), [`content-type`](#content-type).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 38, [`1124425c`](https://github.com/KogasaPls/gamescope/commit/1124425c8357dec1a0620038e7f9d305076eec13): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control; host offers are read eagerly. A host without data control keeps upstream's outbound seat path. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`patch-tests`](#patch-tests), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

### host-xtest-mouse

Patch 39, [`a97cde72`](https://github.com/KogasaPls/gamescope/commit/a97cde72f268802e719a341002ee68629e13b9b5): WaylandBackend: bridge host XTEST mouse and keyboard input

Experimental opt-in host XTEST mouse and keyboard bridge for external Steam Input on the Wayland backend. Captures XTEST devices while focused and forwards relative motion and held keys, with a composited cursor for ungrabbed mouse movement.

- **Switch:** `GAMESCOPE_WAYLAND_XTEST_MOUSE=1`: opt-in, read at startup.
- **Depends on:** [`patch-tests`](#patch-tests), [`modifier-forwarding`](#modifier-forwarding), [`wayland-client-buffer`](#wayland-client-buffer), [`pointer-focus-churn`](#pointer-focus-churn), [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/host-xtest-mouse`](https://github.com/KogasaPls/gamescope/tree/topic/host-xtest-mouse).

### adaptive-sync-launch-tag

Patch 40, [`66e1c008`](https://github.com/KogasaPls/gamescope/commit/66e1c00816e365ffe1ea15e76266723e1b43da56): WaylandBackend: tag adaptive-sync launches

Tag native Wayland toplevels with adaptive-sync when adaptive sync is enabled at window creation, letting compositor rules opt in only requesting launches. Runtime preference changes do not retag the window.

- **Switch:** `--adaptive-sync`: tag the outer Wayland toplevel at creation..
- **Depends on:** [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/adaptive-sync-launch-tag`](https://github.com/KogasaPls/gamescope/tree/topic/adaptive-sync-launch-tag).

## Compatibility with specific software

### ewmh-maximize

Patch 19, [`3766a383`](https://github.com/KogasaPls/gamescope/commit/3766a3835fb688543a7f85135889db40ce7f96f6): steamcompmgr: handle EWMH maximize

Handle the EWMH maximize states, so windows that maximize through _NET_WM_STATE, such as GTK3 apps, fill the output and get their previous size back when unmaximized.

- **Source:** [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), reworked.
- **Standalone branch:** [`topic/ewmh-maximize`](https://github.com/KogasaPls/gamescope/tree/topic/ewmh-maximize).

### fifo-passthrough

Patch 36, [`100d3d74`](https://github.com/KogasaPls/gamescope/commit/100d3d741de4915b9052993968779ece98645cc0): layer: opt-in present mode passthrough for FIFO

Adds an opt-in that passes a client's FIFO present mode through to the driver instead of replacing it with MAILBOX. gamescope's own FIFO limits commits per vblank but never blocks vkQueuePresentKHR, so lsfg-vk, which paces its generated frames on that block, free-runs and drops frames.

- **Switch:** `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1`: opt-in, set in the client environment.
- **Obsolete when:** lsfg-vk gains a pacing mode other than pacing = "none", or gamescope gives nested clients driver-level FIFO back-pressure (for example through wp_fifo_v1).
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/fifo-passthrough`](https://github.com/KogasaPls/gamescope/tree/topic/fifo-passthrough).

## New options

### track-app-size

Patch 30, [`2a4c5a38`](https://github.com/KogasaPls/gamescope/commit/2a4c5a38346bf78054929e606416950eacd1be70): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Depends on:** [`patch-tests`](#patch-tests), [`null-fb-present`](#null-fb-present), [`pipewire-shutdown`](#pipewire-shutdown).
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 33, [`98fec72f`](https://github.com/KogasaPls/gamescope/commit/98fec72f793072fa4243bd90c6b414223fb5597b): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once no client window has existed for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`patch-tests`](#patch-tests), [`ordered-shutdown`](#ordered-shutdown), [`track-app-size`](#track-app-size), [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`43d1ab98`](https://github.com/KogasaPls/gamescope/commit/43d1ab98631195da21bd7815e0d0074dae7ff94b): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### held-button-focus

Patch 03, [`25e29d7a`](https://github.com/KogasaPls/gamescope/commit/25e29d7acce825c8fd576df8eb452c1485bdcf58): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 04, [`4b562e5b`](https://github.com/KogasaPls/gamescope/commit/4b562e5b5bf3d703b7c7c7e7cc49a6a6fa8b002f): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 05, [`b7cf348b`](https://github.com/KogasaPls/gamescope/commit/b7cf348b6ffdcd90625760c11606975d40092e3f): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 06, [`5c4eb7c2`](https://github.com/KogasaPls/gamescope/commit/5c4eb7c21975230d26bad62da78e5b7426ee065e): Forward Wayland modifier events to wlroots seat when nested

Forwards the host's keyboard modifier state into the nested session. Without it Xwayland clients see every key press with no Shift, Ctrl or Alt held.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### format32-long

Patch 07, [`6c861342`](https://github.com/KogasaPls/gamescope/commit/6c861342ad39c13d92edadfa8628bc1f012d6b86): steamcompmgr: write format-32 properties from long arrays

Writes every format-32 X property gamescope sets from an array of long, which is what Xlib reads. The 32-bit values it was given made Xlib read past them on 64-bit systems, publishing garbage in WM_STATE and the root window's feedback properties.

- **Standalone branch:** [`topic/format32-long`](https://github.com/KogasaPls/gamescope/tree/topic/format32-long).

### fullscreen-null-connector

Patch 08, [`c2533ae3`](https://github.com/KogasaPls/gamescope/commit/c2533ae3b624a824d137aa454b8e2ec181013723): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 09, [`75f9f3cb`](https://github.com/KogasaPls/gamescope/commit/75f9f3cbcd70c4c94e2932ec7d2794e2140eae13): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. The float sanitising guards are widened to cover -ffinite-math-only by testing the value of __FINITE_MATH_ONLY__, which GCC and Clang always define.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 10, [`9134c0bc`](https://github.com/KogasaPls/gamescope/commit/9134c0bccdeff492b4c4ef059ca12a303d77e607): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 11, [`c8fd4e8f`](https://github.com/KogasaPls/gamescope/commit/c8fd4e8faa8e1e2888f11266af600a74282492ba): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 12, [`4878b830`](https://github.com/KogasaPls/gamescope/commit/4878b830145cb16d61f5514c18903240da2aa190): WaylandBackend: present no buffer for a texture without an fb

Composites a frame whose layer has no backend framebuffer, such as an override blit image in a format the backend cannot scan out, and presents no buffer for such a texture instead of dereferencing null.

- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 13, [`bda60e53`](https://github.com/KogasaPls/gamescope/commit/bda60e5352cf3f1030ba903b345f584a06b4e228): WaylandBackend: scale subsurface positions as signed values

Scales subsurface positions as signed values, so a layer placed left of or above the nested output is no longer sent to the host as a large positive position. The fractional-scale conversions move into a tested helper with 64-bit arithmetic.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### image-description-leak

Patch 14, [`32d11671`](https://github.com/KogasaPls/gamescope/commit/32d11671435e2599c5eee84b521f7e1e9789381f): WaylandBackend: release the current image description with its plane

Destroys a plane's current wp_image_description_v1 when the plane is torn down, which leaked.

- **Standalone branch:** [`topic/image-description-leak`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-leak).

### image-description-ready

Patch 15, [`d127a0e3`](https://github.com/KogasaPls/gamescope/commit/d127a0e37d78830c89453d4cf90ef7ef1a893f1e): WaylandBackend: wait for image descriptions to become ready

Waits for a wp_image_description_v1's ready event before setting it on a surface or querying it, as wp_color_management_v1 requires, and presents without one when the host answers failed instead of raising a fatal protocol error.

- **Standalone branch:** [`topic/image-description-ready`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-ready).

### pipewire-capture-pacing

Patch 16, [`619aa79b`](https://github.com/KogasaPls/gamescope/commit/619aa79b678eddf4f3f486ac9fa60f144fb7fe2e): pipewire: pace capture to the negotiated framerate

Paces video capture to the consumer's negotiated framerate, independently of display refresh, and logs skipped capture attempts at debug severity when no buffer is free.

- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/pipewire-capture-pacing`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-pacing).

### wayland-client-buffer

Patch 17, [`65953a42`](https://github.com/KogasaPls/gamescope/commit/65953a42dafece326612fde3d3881c01c86adfbf): wlserver: allow bounded Wayland event bursts up to 1 MiB

Allow bounded input-event bursts up to 1 MiB so a temporarily busy nested Xwayland is not disconnected at the 4 KiB default limit.

- **Source:** Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400.
- **Obsolete when:** Upstream configures a suitable bounded Wayland client event buffer.
- **Standalone branch:** [`topic/wayland-client-buffer`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-client-buffer).

### modifier-max-extent

Patch 18, [`cd79f275`](https://github.com/KogasaPls/gamescope/commit/cd79f27565d41649e90238d5774b4951c1b3232b): rendervulkan: skip modifiers whose max extent is smaller than the image

Skip modifiers whose maximum image extent is smaller than the image when allocating flippable images, so a 4K nested output on RDNA no longer goes black with a modifier the driver cannot use at that size. Fail the allocation with an error when no modifier fits.

- **Source:** [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked.
- **Upstream issues:** [#1977](https://github.com/ValveSoftware/gamescope/issues/1977), [#2013](https://github.com/ValveSoftware/gamescope/issues/2013).
- **Standalone branch:** [`topic/modifier-max-extent`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-max-extent).

### pipewire-reconnect

Patch 21, [`3e3dcd49`](https://github.com/KogasaPls/gamescope/commit/3e3dcd494a0016c5838c4a573479d6cbadd53b0a): pipewire: reconnect the stream instead of exiting when it goes away

Reconnects the PipeWire stream when its node is destroyed or the stream errors, so consumers that find the node by name can capture again (a PipeWire daemon restart and clients holding the node id from gamescope_pipewire are not covered), and closes the races between steamcompmgr's buffer dequeue and buffer removal.

- **Source:** [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked.
- **Upstream issues:** [#2508](https://github.com/ValveSoftware/gamescope/issues/2508).
- **Depends on:** [`pipewire-capture-pacing`](#pipewire-capture-pacing).
- **Standalone branch:** [`topic/pipewire-reconnect`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-reconnect).

### pipewire-failed-buffers

Patch 22, [`353ba346`](https://github.com/KogasaPls/gamescope/commit/353ba34622ac8ea5de968ce8fa4a6b02a964ec90): pipewire: don't crash when add_buffer fails

Keeps capture running when a PipeWire buffer fails to initialize, withholding that buffer from the consumer instead of dereferencing its missing state.

- **Source:** [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked.
- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-failed-buffers`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-failed-buffers).

### pipewire-shutdown

Patch 23, [`6b3079b5`](https://github.com/KogasaPls/gamescope/commit/6b3079b5668fdd075f866a98093e1f80accb9933): pipewire: stop the stream thread before the backend is destroyed

Stops the PipeWire stream thread before the backend is deleted at exit, so a stream renegotiation at that moment no longer calls into a destroyed backend.

- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-shutdown).

### proc-stat-parse

Patch 24, [`2e31deb6`](https://github.com/KogasaPls/gamescope/commit/2e31deb653c942b64d5a707e8d51e74e7ba546a8): Process: parse /proc stat after the command name and skip zombies

Parses /proc/<pid>/stat after the last ')' of the command name, so a child whose name contains a space is still found and signalled, and skips zombie children, which have already exited, while keeping a process whose main thread exited but other threads still run.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part.
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/proc-stat-parse`](https://github.com/KogasaPls/gamescope/tree/topic/proc-stat-parse).

### ordered-shutdown

Patch 25, [`45383894`](https://github.com/KogasaPls/gamescope/commit/45383894adeede71d8ad57b4e89ff5647946f0c2): main: signal children first and keep the display alive on SIGTERM

On SIGTERM, SIGINT, SIGHUP or SIGQUIT, asks the launched processes to exit first and keeps Xwayland running until they have or a grace period passes, then kills what is left and shuts down; a second signal forces it. Closing the nested window over a Proton game no longer pulls the X server out from under it.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked.
- **Upstream issues:** [#885](https://github.com/ValveSoftware/gamescope/issues/885).
- **Switch:** `shutdown_grace_period_ms=<ms>`: how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once.
- **Depends on:** [`patch-tests`](#patch-tests), [`proc-stat-parse`](#proc-stat-parse).
- **Standalone branch:** [`topic/ordered-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/ordered-shutdown).

### wsi-flagged-queues

Patch 27, [`f228d1f6`](https://github.com/KogasaPls/gamescope/commit/f228d1f6b4a5436f0a773200f4eaa8502065a213): layer: give queues created with flags their dispatch

Fixes a crash on the first present of a Vulkan client that creates its queues with flags, such as mpv through libplacebo, by giving those queues an entry in the layer's dispatch tables.

- **Obsolete when:** vkroots registers queues created with flags through vkGetDeviceQueue2.
- **Standalone branch:** [`topic/wsi-flagged-queues`](https://github.com/KogasaPls/gamescope/tree/topic/wsi-flagged-queues).

## Build and tests

### patch-tests

Patch 01, [`200d07c2`](https://github.com/KogasaPls/gamescope/commit/200d07c2089b409ec858c565b21bfbd7fc2ed430): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build. Also adds a startup test that runs the built gamescope with --version, which catches defects that exist only in the linked binary, such as a log scope registered twice aborting before main.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
