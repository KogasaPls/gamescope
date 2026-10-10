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

The stack has 27 patches on upstream commit `32640af7 (2026-10-10, WaylandBackend: pair hotkey presses with their releases)`.

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
| 10 | [`color-manager-gate`](#color-manager-gate) | nested | this fork |
| 11 | [`icon-validation`](#icon-validation) | fix | this fork |
| 12 | [`content-driven-hdr`](#content-driven-hdr) | nested | this fork |
| 13 | [`track-app-size`](#track-app-size) | feature | this fork |
| 14 | [`atomic-output-globals`](#atomic-output-globals) | fix | this fork |
| 15 | [`output-ring-gate`](#output-ring-gate) | nested | this fork |
| 16 | [`null-fb-present`](#null-fb-present) | fix | this fork |
| 17 | [`subsurface-scale`](#subsurface-scale) | fix | this fork |
| 18 | [`keep-toplevel-mapped`](#keep-toplevel-mapped) | nested | this fork |
| 19 | [`exit-when-empty`](#exit-when-empty) | feature | this fork |
| 20 | [`tearing-hints`](#tearing-hints) | nested | this fork |
| 21 | [`content-type`](#content-type) | nested | this fork |
| 22 | [`fifo-passthrough`](#fifo-passthrough) | compat | this fork |
| 23 | [`pointer-focus-churn`](#pointer-focus-churn) | nested | this fork |
| 24 | [`image-description-leak`](#image-description-leak) | fix | this fork |
| 25 | [`host-selection-sync`](#host-selection-sync) | nested | this fork |
| 26 | [`pipewire-capture-pacing`](#pipewire-capture-pacing) | fix | this fork |
| 27 | [`host-xtest-mouse`](#host-xtest-mouse) | nested | this fork |

## Runtime switches

Patches not listed here have no switch and change behaviour unconditionally.

| Switch | Effect | Patch |
|--------|--------|-------|
| `--track-app-size` | opt-in | [`track-app-size`](#track-app-size) |
| `track_app_size_debug=1` | logs viewport changes | [`track-app-size`](#track-app-size) |
| `wayland_output_ring_backpressure=0` | disables the gate | [`output-ring-gate`](#output-ring-gate) |
| `--exit-when-empty[=seconds]` | opt-in; the default grace period is 30 seconds | [`exit-when-empty`](#exit-when-empty) |
| `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1` | opt-in, set in the client environment | [`fifo-passthrough`](#fifo-passthrough) |

## Nested Wayland backend

### color-manager-gate

Patch 10, [`296654c4`](https://github.com/KogasaPls/gamescope/commit/296654c46ff388f95d94c0a2fd11b2cdf30c613f): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts and fixes an integer overflow in the mastering primaries.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 12, [`47abb414`](https://github.com/KogasaPls/gamescope/commit/47abb41466287dd7db67e8663fc48e713d97364e): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR, and switches HDR there by forcing a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 15, [`246cfe3d`](https://github.com/KogasaPls/gamescope/commit/246cfe3d458895b0c5e7c0264708493aedd248e4): WaylandBackend: gate the output ring on host buffer ownership

Composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size), [`atomic-output-globals`](#atomic-output-globals).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 18, [`2509fc32`](https://github.com/KogasaPls/gamescope/commit/2509fc3200495cf6a44c7354b9cd09faf7a41bfb): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 20, [`6399ae6a`](https://github.com/KogasaPls/gamescope/commit/6399ae6a0eca3af2696bbd8a1014d36b4f1ac0a2): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`icon-validation`](#icon-validation), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 21, [`ce3a11dd`](https://github.com/KogasaPls/gamescope/commit/ce3a11dd99298f2a1a34fe73a5ecc201562d2f6b): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 23, [`f301045d`](https://github.com/KogasaPls/gamescope/commit/f301045d4da12d8dd2ad7f6fce92a008afbf0354): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`modifier-forwarding`](#modifier-forwarding), [`output-ring-gate`](#output-ring-gate), [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`content-type`](#content-type).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 25, [`85406c53`](https://github.com/KogasaPls/gamescope/commit/85406c53c45eeb30f7d716be3fd34bb6dd49a3e9): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control, falling back to the seat devices; host offers are read eagerly. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

### host-xtest-mouse

Patch 27, [`78abf9ad`](https://github.com/KogasaPls/gamescope/commit/78abf9ada36f4dbabe0aad2f1a265d4510dabba4): WaylandBackend: bridge host XTEST relative mouse motion

Experimental opt-in host XTEST relative mouse bridge for external Steam Joystick Mouse on the Wayland backend. Movement only; forwards while the native window has keyboard focus, with a composited cursor for ungrabbed mouse movement.

- **Depends on:** [`pointer-focus-churn`](#pointer-focus-churn), [`host-selection-sync`](#host-selection-sync).
- **Standalone branch:** [`topic/host-xtest-mouse`](https://github.com/KogasaPls/gamescope/tree/topic/host-xtest-mouse).

## Compatibility with specific software

### fifo-passthrough

Patch 22, [`8dab86df`](https://github.com/KogasaPls/gamescope/commit/8dab86df635091466e7f4a6d3d9e38a4065ad5f9): layer: opt-in present mode passthrough for FIFO

Adds an opt-in that passes a client's FIFO present mode through to the driver instead of replacing it with MAILBOX. gamescope's own FIFO limits commits per vblank but never blocks vkQueuePresentKHR, so lsfg-vk, which paces its generated frames on that block, free-runs and drops frames.

- **Switch:** `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1`: opt-in, set in the client environment.
- **Obsolete when:** lsfg-vk gains a pacing mode other than pacing = "none", or gamescope gives nested clients driver-level FIFO back-pressure (for example through wp_fifo_v1).
- **Standalone branch:** [`topic/fifo-passthrough`](https://github.com/KogasaPls/gamescope/tree/topic/fifo-passthrough).

## New options

### track-app-size

Patch 13, [`c907ed0e`](https://github.com/KogasaPls/gamescope/commit/c907ed0e809e1315688cc2aa32b93aac895583ac): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 19, [`6c6036ca`](https://github.com/KogasaPls/gamescope/commit/6c6036ca202a69a5c5d743bd7ef54b6c3f7ded04): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once no client window has existed for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`e021c694`](https://github.com/KogasaPls/gamescope/commit/e021c6948c1cf9549fac1214dbab3586d24b45a3): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### held-button-focus

Patch 03, [`7ef46c6c`](https://github.com/KogasaPls/gamescope/commit/7ef46c6c3b308b7a08704ae76bafe4e389c8e2ca): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 04, [`5a444712`](https://github.com/KogasaPls/gamescope/commit/5a44471249d21ea1d8d0ae6a7232cb6fe38d6ebe): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 05, [`e08994ec`](https://github.com/KogasaPls/gamescope/commit/e08994eca764933e6670cffce652ac1c0693333e): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 06, [`12b3c710`](https://github.com/KogasaPls/gamescope/commit/12b3c710dc76d20aaf3ea92e11c74a628d9fe088): Forward Wayland modifier events to wlroots seat when nested

Forwards the host's keyboard modifier state into the nested session. Without it Xwayland clients see every key press with no Shift, Ctrl or Alt held.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### format32-long

Patch 07, [`d5290068`](https://github.com/KogasaPls/gamescope/commit/d52900680e7a45554d7684d6229e91760fbad49c): steamcompmgr: write format-32 properties from long arrays

Writes every format-32 X property gamescope sets from an array of long, which is what Xlib reads. The 32-bit values it was given made Xlib read past them on 64-bit systems, publishing garbage in WM_STATE and the root window's feedback properties.

- **Standalone branch:** [`topic/format32-long`](https://github.com/KogasaPls/gamescope/tree/topic/format32-long).

### fullscreen-null-connector

Patch 08, [`089de057`](https://github.com/KogasaPls/gamescope/commit/089de0577471e09a46bebb9bb395bf489660a77f): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 09, [`8eb65a42`](https://github.com/KogasaPls/gamescope/commit/8eb65a42b6a4999717935733056c51c798afab29): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. Also fixes a sanitising guard that tested defined(__FINITE_MATH_ONLY__), which is true in every build.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 11, [`37500c84`](https://github.com/KogasaPls/gamescope/commit/37500c84098c102785f670d54c7efd1c9baa23a1): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`color-manager-gate`](#color-manager-gate).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 14, [`e4e6b2c2`](https://github.com/KogasaPls/gamescope/commit/e4e6b2c27f89f7b4220e2b5ede47261e7d28d41d): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 16, [`ab05b4a7`](https://github.com/KogasaPls/gamescope/commit/ab05b4a7ff63eae23c53738ec7ecd75f5e0a65af): WaylandBackend: present no buffer for a texture without an fb

Composites a frame whose layer has no backend framebuffer, such as an override blit image in a format the backend cannot scan out, and presents no buffer for such a texture instead of dereferencing null.

- **Depends on:** [`output-ring-gate`](#output-ring-gate).
- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 17, [`6138f084`](https://github.com/KogasaPls/gamescope/commit/6138f08425113e2a80a1264a8769c6376c3776e9): WaylandBackend: scale subsurface positions as signed values

Moves fractional-scale conversion into a tested helper with 64-bit signed arithmetic, so a negative subsurface position cannot pass through unsigned math.

- **Depends on:** [`null-fb-present`](#null-fb-present).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### image-description-leak

Patch 24, [`9da758c3`](https://github.com/KogasaPls/gamescope/commit/9da758c32880a7032284dd04f73c92757dbe88f5): WaylandBackend: release the current image description with its plane

Destroys a plane's current wp_image_description_v1 when the plane is torn down, which leaked.

- **Depends on:** [`pointer-focus-churn`](#pointer-focus-churn).
- **Standalone branch:** [`topic/image-description-leak`](https://github.com/KogasaPls/gamescope/tree/topic/image-description-leak).

### pipewire-capture-pacing

Patch 26, [`7579becb`](https://github.com/KogasaPls/gamescope/commit/7579becbab9321b97729180b81b17d25b890ec20): pipewire: pace capture to the negotiated framerate

Paces video capture to the consumer's negotiated framerate, independently of display refresh, and logs skipped capture attempts at debug severity when no buffer is free.

- **Standalone branch:** [`topic/pipewire-capture-pacing`](https://github.com/KogasaPls/gamescope/tree/topic/pipewire-capture-pacing).

## Build and tests

### patch-tests

Patch 01, [`b0333d5f`](https://github.com/KogasaPls/gamescope/commit/b0333d5f0672d00d6096bcabe366de3acb98fddb): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
