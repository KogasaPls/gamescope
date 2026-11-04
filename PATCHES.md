# Patches in this fork

This fork carries a patch stack on top of upstream
[gamescope](https://github.com/ValveSoftware/gamescope). Most of the patches
target the nested Wayland backend, that is, gamescope running as a window on a
wlroots compositor such as sway. The DRM and OpenVR backends are not tested
here.

Branches:

- `patched/full-stack` is upstream plus every patch below, in order.
- `topic/<name>` is one patch applied to upstream together with the patches
  it depends on, suitable for an upstream pull request.
- `master` is the upstream commit the stack is based on, plus this page and
  the README note that links to it.

Each patch is one of five kinds:

- **nested**: behaviour of gamescope nested under a Wayland host.
- **compat**: a workaround for a specific piece of software (lsfg-vk, Proton).
- **feature**: a new command-line option.
- **fix**: an upstream bug that affects any setup.
- **build**: build system and test infrastructure.

This page is generated from the `Patch-*` trailers of each commit on
`patched/full-stack`, so it describes the stack as published.

The stack has 38 patches on upstream commit `309eee74 (2026-10-31, vblankmanager: keep the fixed-refresh wakeup after the previous vblank)`.

## Index

| # | Patch | Kind | Source |
|---|-------|------|--------|
| 01 | [`patch-tests`](#patch-tests) | build | this fork |
| 02 | [`rendervulkan-private-booleans`](#rendervulkan-private-booleans) | fix | [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen) |
| 03 | [`nested-cursor-hotspot`](#nested-cursor-hotspot) | fix | [#2534](https://github.com/ValveSoftware/gamescope/issues/2534) (matte-schwartz) |
| 04 | [`held-button-focus`](#held-button-focus) | fix | this fork |
| 05 | [`confine-region`](#confine-region) | fix | this fork |
| 06 | [`presentation-feedback-cancel`](#presentation-feedback-cancel) | fix | this fork |
| 07 | [`modifier-forwarding`](#modifier-forwarding) | fix | [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes |
| 08 | [`format32-long`](#format32-long) | fix | this fork |
| 09 | [`fullscreen-null-connector`](#fullscreen-null-connector) | fix | this fork |
| 10 | [`fast-math-scope`](#fast-math-scope) | fix | [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked |
| 11 | [`color-manager-gate`](#color-manager-gate) | nested | this fork |
| 12 | [`icon-validation`](#icon-validation) | fix | this fork |
| 13 | [`content-driven-hdr`](#content-driven-hdr) | nested | this fork |
| 14 | [`track-app-size`](#track-app-size) | feature | this fork |
| 15 | [`atomic-output-globals`](#atomic-output-globals) | fix | this fork |
| 16 | [`output-ring-gate`](#output-ring-gate) | nested | this fork |
| 17 | [`null-fb-present`](#null-fb-present) | fix | this fork |
| 18 | [`subsurface-scale`](#subsurface-scale) | fix | this fork |
| 19 | [`keep-toplevel-mapped`](#keep-toplevel-mapped) | nested | this fork |
| 20 | [`exit-when-empty`](#exit-when-empty) | feature | this fork |
| 21 | [`tearing-hints`](#tearing-hints) | nested | this fork |
| 22 | [`content-type`](#content-type) | nested | this fork |
| 23 | [`fifo-passthrough`](#fifo-passthrough) | compat | this fork |
| 24 | [`pointer-focus-churn`](#pointer-focus-churn) | nested | this fork |
| 25 | [`image-description-leak`](#image-description-leak) | fix | this fork |
| 26 | [`host-selection-sync`](#host-selection-sync) | nested | this fork |
| 27 | [`pipewire-capture-pacing`](#pipewire-capture-pacing) | fix | this fork |
| 28 | [`wayland-client-buffer`](#wayland-client-buffer) | fix | Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400 |
| 29 | [`host-xtest-mouse`](#host-xtest-mouse) | nested | this fork |
| 30 | [`adaptive-sync-launch-tag`](#adaptive-sync-launch-tag) | nested | this fork |
| 31 | [`modifier-max-extent`](#modifier-max-extent) | fix | [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked |
| 32 | [`ewmh-maximize`](#ewmh-maximize) | compat | [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), reworked |
| 33 | [`cursor-surface-scale`](#cursor-surface-scale) | nested | [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked |
| 34 | [`pipewire-reconnect`](#pipewire-reconnect) | fix | [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked |
| 35 | [`pipewire-failed-buffers`](#pipewire-failed-buffers) | fix | [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked |
| 36 | [`proc-stat-parse`](#proc-stat-parse) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part |
| 37 | [`ordered-shutdown`](#ordered-shutdown) | fix | [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked |
| 38 | [`wayland-error-shutdown`](#wayland-error-shutdown) | nested | [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked |

## Runtime switches

Patches not listed here have no switch and change behaviour unconditionally.

| Switch | Effect | Patch |
|--------|--------|-------|
| `--track-app-size` | opt-in | [`track-app-size`](#track-app-size) |
| `track_app_size_debug=1` | logs viewport changes | [`track-app-size`](#track-app-size) |
| `wayland_output_ring_backpressure=0` | disables the gate | [`output-ring-gate`](#output-ring-gate) |
| `--exit-when-empty[=seconds]` | opt-in; the default grace period is 30 seconds | [`exit-when-empty`](#exit-when-empty) |
| `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1` | opt-in, set in the client environment | [`fifo-passthrough`](#fifo-passthrough) |
| `--adaptive-sync` | tag the outer Wayland toplevel at creation. | [`adaptive-sync-launch-tag`](#adaptive-sync-launch-tag) |
| `shutdown_grace_period_ms=<ms>` | how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once | [`ordered-shutdown`](#ordered-shutdown) |

## Nested Wayland backend

### color-manager-gate

Patch 11, [`159e5114`](https://github.com/KogasaPls/gamescope/commit/159e511421945b43a811badec364f433a57e527e): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 13, [`60b8a327`](https://github.com/KogasaPls/gamescope/commit/60b8a3277a2146973359ec4f3ca0269260927572): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR, and switches HDR there by forcing a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 16, [`011d2fac`](https://github.com/KogasaPls/gamescope/commit/011d2facd309c45d8866e7e49476a6f897645cd9): WaylandBackend: gate the output ring on host buffer ownership

Composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 19, [`608f71fc`](https://github.com/KogasaPls/gamescope/commit/608f71fcd0112911dbdd0fc5e9d6bf5e8e4d1a1c): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 21, [`963fec63`](https://github.com/KogasaPls/gamescope/commit/963fec6348cdbc60d726d721964d5f9ce3fcd0de): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`icon-validation`](#icon-validation), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 22, [`416b434a`](https://github.com/KogasaPls/gamescope/commit/416b434a284dda99a660ca204fc7a94f4c2f174a): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 24, [`e778c6d9`](https://github.com/KogasaPls/gamescope/commit/e778c6d9f0a811e81bd4f10b4f715a5b650ea619): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`modifier-forwarding`](#modifier-forwarding), [`output-ring-gate`](#output-ring-gate), [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`content-type`](#content-type).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 26, [`1da83dea`](https://github.com/KogasaPls/gamescope/commit/1da83dea37ab0f643f131ef94a92c53ff3669a4f): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control, falling back to the seat devices; host offers are read eagerly. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

### host-xtest-mouse

Patch 29, [`013d6d0f`](https://github.com/KogasaPls/gamescope/commit/013d6d0fa45fe9b749dbd3f50bb1b523d230e21e): WaylandBackend: bridge host XTEST mouse and keyboard input

Experimental opt-in host XTEST mouse and keyboard bridge for external Steam Input on the Wayland backend. Captures XTEST devices while focused and forwards relative motion and held keys, with a composited cursor for ungrabbed mouse movement.

- **Depends on:** [`patch-tests`](#patch-tests), [`nested-cursor-hotspot`](#nested-cursor-hotspot), [`pointer-focus-churn`](#pointer-focus-churn), [`host-selection-sync`](#host-selection-sync), [`wayland-client-buffer`](#wayland-client-buffer).
- **Standalone branch:** [`topic/host-xtest-mouse`](https://github.com/KogasaPls/gamescope/tree/topic/host-xtest-mouse).

### adaptive-sync-launch-tag

Patch 30, [`773503d6`](https://github.com/KogasaPls/gamescope/commit/773503d6538374b162b87710aed4493469529895): WaylandBackend: tag adaptive-sync launches

Tag native Wayland toplevels with adaptive-sync when adaptive sync is enabled at window creation, letting compositor rules opt in only requesting launches. Runtime preference changes do not retag the window.

- **Switch:** `--adaptive-sync`: tag the outer Wayland toplevel at creation..
- **Depends on:** [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/adaptive-sync-launch-tag`](https://github.com/KogasaPls/gamescope/tree/topic/adaptive-sync-launch-tag).

### cursor-surface-scale

Patch 33, [`e99257f0`](https://github.com/KogasaPls/gamescope/commit/e99257f01912febd7c66e418a0021621010aa85f): WaylandBackend: reuse the cursor surface and scale it to the host's fractional scale

Releases cursor buffers on the host's release event and reuses the nested cursor surface, scaling it with a viewport so it stays sharp at fractional scales.

- **Source:** [#2441](https://github.com/ValveSoftware/gamescope/issues/2441) (connormclaud), reworked.
- **Upstream issues:** [#2428](https://github.com/ValveSoftware/gamescope/issues/2428), [#2432](https://github.com/ValveSoftware/gamescope/issues/2432).
- **Depends on:** [`subsurface-scale`](#subsurface-scale), [`color-manager-gate`](#color-manager-gate), [`pointer-focus-churn`](#pointer-focus-churn), [`host-xtest-mouse`](#host-xtest-mouse).
- **Standalone branch:** [`topic/cursor-surface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/cursor-surface-scale).

### wayland-error-shutdown

Patch 38, [`f721fed6`](https://github.com/KogasaPls/gamescope/commit/f721fed691cb5c581c3dc2114ad6d5f17e4b1bb0): WaylandBackend: shut down instead of aborting on host Wayland errors

Raises SIGTERM and stops the input thread when the host Wayland connection fails or hangs up, instead of aborting with a core dump, so the launched game is asked to exit before Xwayland goes away.

- **Source:** [#2393](https://github.com/ValveSoftware/gamescope/issues/2393) (adamsjo95), reworked.
- **Depends on:** [`ordered-shutdown`](#ordered-shutdown), [`pointer-focus-churn`](#pointer-focus-churn), [`host-xtest-mouse`](#host-xtest-mouse).
- **Standalone branch:** [`topic/wayland-error-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-error-shutdown).

## Compatibility with specific software

### fifo-passthrough

Patch 23, [`c5c30c15`](https://github.com/KogasaPls/gamescope/commit/c5c30c154ce3b461b025c8c5c5363a7e6933a35b): layer: opt-in present mode passthrough for FIFO

Adds an opt-in that passes a client's FIFO present mode through to the driver instead of replacing it with MAILBOX. gamescope's own FIFO limits commits per vblank but never blocks vkQueuePresentKHR, so lsfg-vk, which paces its generated frames on that block, free-runs and drops frames.

- **Switch:** `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1`: opt-in, set in the client environment.
- **Obsolete when:** lsfg-vk gains a pacing mode other than pacing = "none", or gamescope gives nested clients driver-level FIFO back-pressure (for example through wp_fifo_v1).
- **Standalone branch:** [`topic/fifo-passthrough`](https://github.com/KogasaPls/gamescope/tree/topic/fifo-passthrough).

### ewmh-maximize

Patch 32, [`c1d4cf11`](https://github.com/KogasaPls/gamescope/commit/c1d4cf11238655994022d1c4c18ee4a8338fd11d): steamcompmgr: handle EWMH maximize

Handle the EWMH maximize states, so windows that maximize through _NET_WM_STATE, such as GTK3 apps, fill the output and get their previous size back when unmaximized.

- **Source:** [#2533](https://github.com/ValveSoftware/gamescope/issues/2533) (eerii), reworked.
- **Standalone branch:** [`topic/ewmh-maximize`](https://github.com/KogasaPls/gamescope/tree/topic/ewmh-maximize).

## New options

### track-app-size

Patch 14, [`fd3ab400`](https://github.com/KogasaPls/gamescope/commit/fd3ab400acff35153fa776d1fc826553661a12aa): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 20, [`f0b27929`](https://github.com/KogasaPls/gamescope/commit/f0b27929c409dfcc8dd785fe25763999aa7296ae): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once no client window has existed for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`94e6fb19`](https://github.com/KogasaPls/gamescope/commit/94e6fb19997faeebf45214679161149b0d2a9f3c): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### nested-cursor-hotspot

Patch 03, [`b0f482dd`](https://github.com/KogasaPls/gamescope/commit/b0f482dd563d47243a3d9a700d95563ab3e95528): steamcompmgr, OpenVRBackend: subtract the cursor hotspot on nested backends

Place the composited cursor by its top-left corner on nested backends again, so it lines up with the pointer under --force-grab-cursor. OpenVR adds the hotspot back for its cursor overlay position.

- **Source:** [#2534](https://github.com/ValveSoftware/gamescope/issues/2534) (matte-schwartz).
- **Upstream issues:** [#2529](https://github.com/ValveSoftware/gamescope/issues/2529).
- **Standalone branch:** [`topic/nested-cursor-hotspot`](https://github.com/KogasaPls/gamescope/tree/topic/nested-cursor-hotspot).

### held-button-focus

Patch 04, [`4c6ec4c4`](https://github.com/KogasaPls/gamescope/commit/4c6ec4c46b8fea035875df3cf8336a67acb0b509): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 05, [`72af57b8`](https://github.com/KogasaPls/gamescope/commit/72af57b88527db2591bdc01d4b68aa17ee5fff4d): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 06, [`5e6b579a`](https://github.com/KogasaPls/gamescope/commit/5e6b579a0ac3af9a6672cb5fdef8096090318b70): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 07, [`9c5e3c1c`](https://github.com/KogasaPls/gamescope/commit/9c5e3c1c52d81136900032a4efaa4fee23f98eb1): Forward Wayland modifier events to wlroots seat when nested

Forwards the host's keyboard modifier state into the nested session. Without it Xwayland clients see every key press with no Shift, Ctrl or Alt held.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### format32-long

Patch 08, [`48e1c428`](https://github.com/KogasaPls/gamescope/commit/48e1c428ba5acec5c937b79e85ef980497cf6c06): steamcompmgr: write format-32 properties from long arrays

Writes every format-32 X property gamescope sets from an array of long, which is what Xlib reads. The 32-bit values it was given made Xlib read past them on 64-bit systems, publishing garbage in WM_STATE and the root window's feedback properties.

- **Standalone branch:** [`topic/format32-long`](https://github.com/KogasaPls/gamescope/tree/topic/format32-long).

### fullscreen-null-connector

Patch 09, [`d4fc852d`](https://github.com/KogasaPls/gamescope/commit/d4fc852d8ac26aab98f8ce0a2ba5e1fbd92a6147): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 10, [`0a5509ce`](https://github.com/KogasaPls/gamescope/commit/0a5509ceb7e9e402831dc1edcd3e735e71bab959): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. Also fixes a sanitising guard that tested defined(__FINITE_MATH_ONLY__), which is true in every build.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 12, [`27f7d6c6`](https://github.com/KogasaPls/gamescope/commit/27f7d6c6bb96fabc108d46e2a6d6cc20c6162304): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`color-manager-gate`](#color-manager-gate).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 15, [`8079fad9`](https://github.com/KogasaPls/gamescope/commit/8079fad9785a9e1dc4ae7a5d20b47b705a29e77d): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 17, [`e1fe83c8`](https://github.com/KogasaPls/gamescope/commit/e1fe83c8d32e82dd877cf6a1e690b6f79ea5f15f): WaylandBackend: present no buffer for a texture without an fb

Composites a frame whose layer has no backend framebuffer, such as an override blit image in a format the backend cannot scan out, and presents no buffer for such a texture instead of dereferencing null.

- **Depends on:** [`output-ring-gate`](#output-ring-gate).
- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 18, [`93d9fe6f`](https://github.com/KogasaPls/gamescope/commit/93d9fe6f3d7a75371115d08134d4c9d014bd27a3): WaylandBackend: scale subsurface positions as signed values

Moves fractional-scale conversion into a tested helper with 64-bit signed arithmetic, so a negative subsurface position cannot pass through unsigned math.

- **Depends on:** [`null-fb-present`](#null-fb-present).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### image-description-leak

Patch 25, [`89921166`](https://github.com/KogasaPls/gamescope/commit/8992116636c01753641ed33a58d8716c77ab8657): WaylandBackend: release the current image description with its plane

Destroys a plane's current wp_image_description_v1 when the plane is torn down, which leaked.

- **Depends on:** [`pointer-focus-churn`](#pointer-focus-churn).
- **Standalone branch:** [`topic/image-description-leak`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-leak).

### pipewire-capture-pacing

Patch 27, [`4421d8e6`](https://github.com/KogasaPls/gamescope/commit/4421d8e6ff76ee1d64b5e5d9a58525ae6e544d1a): pipewire: pace capture to the negotiated framerate

Paces video capture to the consumer's negotiated framerate, independently of display refresh, and logs skipped capture attempts at debug severity when no buffer is free.

- **Standalone branch:** [`topic/pipewire-capture-pacing`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-pacing).

### wayland-client-buffer

Patch 28, [`32022325`](https://github.com/KogasaPls/gamescope/commit/32022325a0af318127017ba94fbf5845f360e2e8): wlserver: allow bounded Wayland event bursts up to 1 MiB

Allow bounded input-event bursts up to 1 MiB so a temporarily busy nested Xwayland is not disconnected at the 4 KiB default limit.

- **Source:** Same bounded buffer adjustment as https://github.com/swaywm/sway/pull/8532 and https://github.com/labwc/labwc/pull/3400.
- **Obsolete when:** Upstream configures a suitable bounded Wayland client event buffer.
- **Standalone branch:** [`topic/wayland-client-buffer`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-client-buffer).

### modifier-max-extent

Patch 31, [`7669d48f`](https://github.com/KogasaPls/gamescope/commit/7669d48f20e26cd965ce64ffca284556d0f21701): rendervulkan: skip modifiers whose max extent is smaller than the image

Skip modifiers whose maximum image extent is smaller than the image when allocating flippable images, so a 4K nested output on RDNA no longer goes black with a modifier the driver cannot use at that size. Fail the allocation with an error when no modifier fits.

- **Source:** [#2049](https://github.com/ValveSoftware/gamescope/issues/2049) (njdom24), reworked.
- **Upstream issues:** [#1977](https://github.com/ValveSoftware/gamescope/issues/1977), [#2013](https://github.com/ValveSoftware/gamescope/issues/2013).
- **Standalone branch:** [`topic/modifier-max-extent`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-max-extent).

### pipewire-reconnect

Patch 34, [`f6b68f22`](https://github.com/KogasaPls/gamescope/commit/f6b68f2234aa76559f1971f8a604cf7f353a38f1): pipewire: reconnect the stream instead of exiting when it goes away

Reconnects the PipeWire stream when its node is destroyed or the stream errors, so consumers that find the node by name can capture again (a PipeWire daemon restart and clients holding the node id from gamescope_pipewire are not covered), and closes the races between steamcompmgr's buffer dequeue and buffer removal.

- **Source:** [#2522](https://github.com/ValveSoftware/gamescope/issues/2522) (unsound-mint), reworked.
- **Upstream issues:** [#2508](https://github.com/ValveSoftware/gamescope/issues/2508).
- **Depends on:** [`track-app-size`](#track-app-size), [`pipewire-capture-pacing`](#pipewire-capture-pacing).
- **Standalone branch:** [`topic/pipewire-reconnect`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-reconnect).

### pipewire-failed-buffers

Patch 35, [`203aab1c`](https://github.com/KogasaPls/gamescope/commit/203aab1c3a131676c9de4584bd15ca0281aa64d3): pipewire: don't crash when add_buffer fails

Keeps capture running when a PipeWire buffer fails to initialize, withholding that buffer from the consumer instead of dereferencing its missing state.

- **Source:** [#2460](https://github.com/ValveSoftware/gamescope/issues/2460) (Danielslay86), reworked.
- **Depends on:** [`pipewire-reconnect`](#pipewire-reconnect).
- **Standalone branch:** [`topic/pipewire-failed-buffers`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-failed-buffers).

### proc-stat-parse

Patch 36, [`d80f28f7`](https://github.com/KogasaPls/gamescope/commit/d80f28f7781fb7782556b3d6e9a8c48992d16f6a): Process: parse /proc stat after the command name and skip zombies

Parses /proc/<pid>/stat after the last ')' of the command name, so a child whose name contains a space is still found and signalled, and skips zombie children, which have already exited, while keeping a process whose main thread exited but other threads still run.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), in part.
- **Depends on:** [`patch-tests`](#patch-tests).
- **Standalone branch:** [`topic/proc-stat-parse`](https://github.com/KogasaPls/gamescope/tree/topic/proc-stat-parse).

### ordered-shutdown

Patch 37, [`fb321652`](https://github.com/KogasaPls/gamescope/commit/fb32165287acc31be1ef94bdde713b15c22faeb4): main: signal children first and keep the display alive on SIGTERM

On SIGTERM, SIGINT, SIGHUP or SIGQUIT, asks the launched processes to exit first and keeps Xwayland running until they have or a grace period passes, then kills what is left and shuts down; a second signal forces it. Closing the nested window over a Proton game no longer pulls the X server out from under it.

- **Source:** [#2498](https://github.com/ValveSoftware/gamescope/issues/2498) (connormclaud), reworked.
- **Upstream issues:** [#885](https://github.com/ValveSoftware/gamescope/issues/885).
- **Switch:** `shutdown_grace_period_ms=<ms>`: how long a terminating signal waits for the launched processes before killing them (10000 by default); 0 shuts down at once.
- **Depends on:** [`patch-tests`](#patch-tests), [`proc-stat-parse`](#proc-stat-parse).
- **Standalone branch:** [`topic/ordered-shutdown`](https://github.com/KogasaPls/gamescope/tree/topic/ordered-shutdown).

## Build and tests

### patch-tests

Patch 01, [`29ddba80`](https://github.com/KogasaPls/gamescope/commit/29ddba80d745250109b6f58b4db3d232ad124373): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
