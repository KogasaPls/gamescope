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

The stack has 28 patches on upstream commit `c50ddfa9 (2026-09-17, WaylandBackend: don't attach a buffer before the toplevel is reconfigured)`.

## Index

| # | Patch | Kind | Source |
|---|-------|------|--------|
| 01 | [`patch-tests`](#patch-tests) | build | this fork |
| 02 | [`rendervulkan-private-booleans`](#rendervulkan-private-booleans) | fix | [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen) |
| 03 | [`refresh-cycle-resend`](#refresh-cycle-resend) | fix | this fork |
| 04 | [`keep-deferred-xdg-commits`](#keep-deferred-xdg-commits) | fix | this fork |
| 05 | [`held-button-focus`](#held-button-focus) | fix | this fork |
| 06 | [`confine-region`](#confine-region) | fix | this fork |
| 07 | [`presentation-feedback-cancel`](#presentation-feedback-cancel) | fix | this fork |
| 08 | [`modifier-forwarding`](#modifier-forwarding) | fix | [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes |
| 09 | [`wm-state-long`](#wm-state-long) | fix | this fork |
| 10 | [`fullscreen-null-connector`](#fullscreen-null-connector) | fix | this fork |
| 11 | [`fast-math-scope`](#fast-math-scope) | fix | [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked |
| 12 | [`color-manager-gate`](#color-manager-gate) | nested | this fork |
| 13 | [`icon-validation`](#icon-validation) | fix | this fork |
| 14 | [`content-driven-hdr`](#content-driven-hdr) | nested | this fork |
| 15 | [`track-app-size`](#track-app-size) | feature | this fork |
| 16 | [`atomic-output-globals`](#atomic-output-globals) | fix | this fork |
| 17 | [`output-ring-gate`](#output-ring-gate) | nested | this fork |
| 18 | [`null-fb-present`](#null-fb-present) | fix | this fork |
| 19 | [`subsurface-scale`](#subsurface-scale) | fix | this fork |
| 20 | [`keep-toplevel-mapped`](#keep-toplevel-mapped) | nested | this fork |
| 21 | [`exit-when-empty`](#exit-when-empty) | feature | this fork |
| 22 | [`tearing-hints`](#tearing-hints) | nested | this fork |
| 23 | [`content-type`](#content-type) | nested | this fork |
| 24 | [`fifo-passthrough`](#fifo-passthrough) | compat | this fork |
| 25 | [`pointer-focus-churn`](#pointer-focus-churn) | nested | this fork |
| 26 | [`plane-teardown-lock`](#plane-teardown-lock) | fix | this fork |
| 27 | [`host-selection-sync`](#host-selection-sync) | nested | this fork |
| 28 | [`wayland-display-unset`](#wayland-display-unset) | compat | this fork |

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

Patch 12, [`d072f35e`](https://github.com/KogasaPls/gamescope/commit/d072f35e87c87dea57ede5d1971f97170241b79e): WaylandBackend: gate wp_color_manager features individually

Checks each wp_color_manager_v1 feature where it is used instead of requiring all of them up front, which disables nested HDR on hosts such as sway 1.13 that implement only part of the protocol. Also guards two requests that are fatal protocol errors on such hosts and fixes an integer overflow in the mastering primaries.

- **Obsolete when:** wlroots implements set_primaries, set_luminances, extended_target_volume and windows_scrgb, and sway enables them.
- **Standalone branch:** [`topic/color-manager-gate`](https://github.com/KogasaPls/gamescope/tree/topic/color-manager-gate).

### content-driven-hdr

Patch 14, [`546a1e35`](https://github.com/KogasaPls/gamescope/commit/546a1e35409035e56df850d356bff3afe5414a66): steamcompmgr: drive the nested output's HDR from content without a remake

Opts the nested Wayland output into upstream's content-driven HDR, and switches HDR there by forcing a frame rather than reallocating the output images.

- **Standalone branch:** [`topic/content-driven-hdr`](https://github.com/KogasaPls/gamescope/tree/topic/content-driven-hdr).

### output-ring-gate

Patch 17, [`494fb859`](https://github.com/KogasaPls/gamescope/commit/494fb8592c804833802516f95c313e82297ac865): WaylandBackend: gate the output ring on host buffer ownership

Composites only into output buffers the host has released. Without it a nested session under GPU load renders into a buffer the host is still displaying, which shows as coloured corruption on AMD hardware with DCC.

- **Upstream issues:** [#1636](https://github.com/ValveSoftware/gamescope/issues/1636), [#1739](https://github.com/ValveSoftware/gamescope/issues/1739), [#1820](https://github.com/ValveSoftware/gamescope/issues/1820), [#2033](https://github.com/ValveSoftware/gamescope/issues/2033).
- **Switch:** `wayland_output_ring_backpressure=0`: disables the gate.
- **Depends on:** [`content-driven-hdr`](#content-driven-hdr), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/output-ring-gate`](https://github.com/KogasaPls/gamescope/tree/topic/output-ring-gate).

### keep-toplevel-mapped

Patch 20, [`009f37d0`](https://github.com/KogasaPls/gamescope/commit/009f37d0f7995b3a53f09b69bdf6749a0be0a2f1): WaylandBackend: keep the toplevel mapped when there is nothing to show

Shows a black frame when no window has focus instead of unmapping the nested window, so a tiling host does not reflow its layout on every hide and show.

- **Depends on:** [`subsurface-scale`](#subsurface-scale).
- **Standalone branch:** [`topic/keep-toplevel-mapped`](https://github.com/KogasaPls/gamescope/tree/topic/keep-toplevel-mapped).

### tearing-hints

Patch 22, [`78a45f19`](https://github.com/KogasaPls/gamescope/commit/78a45f19a969cf5aa7db313b947b3b2df1caa30f): WaylandBackend: forward tearing hints to the host

Forwards gamescope's tearing decision to the host through wp_tearing_control_v1, so --immediate-flips works when nested. Tearing is reported only while the window is fullscreen, because sway honours the hint only then.

- **Depends on:** [`icon-validation`](#icon-validation), [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/tearing-hints`](https://github.com/KogasaPls/gamescope/tree/topic/tearing-hints).

### content-type

Patch 23, [`b846d981`](https://github.com/KogasaPls/gamescope/commit/b846d9817037dd0b78b972eb9e3294535b209f4a): WaylandBackend: label the toplevel as game content

Marks the nested window as game content through wp_content_type_v1, which a host may use for scheduling or VRR policy.

- **Depends on:** [`tearing-hints`](#tearing-hints).
- **Standalone branch:** [`topic/content-type`](https://github.com/KogasaPls/gamescope/tree/topic/content-type).

### pointer-focus-churn

Patch 25, [`aa799383`](https://github.com/KogasaPls/gamescope/commit/aa799383b26ce8379c48f172d1ae1c3d32b3c31b): WaylandBackend: keep pointer input alive across host focus churn

Keeps pointer motion flowing after the host releases the pointer lock on focus loss, and releases held buttons and keys when the host pointer or keyboard leaves. Without it the cursor froze until the lock was re-established.

- **Depends on:** [`modifier-forwarding`](#modifier-forwarding), [`output-ring-gate`](#output-ring-gate).
- **Standalone branch:** [`topic/pointer-focus-churn`](https://github.com/KogasaPls/gamescope/tree/topic/pointer-focus-churn).

### host-selection-sync

Patch 27, [`9dc79fb8`](https://github.com/KogasaPls/gamescope/commit/9dc79fb8752b4d082ebc41898ca29b41c473eb80): WaylandBackend: sync host selections into the nested session

Syncs the clipboard and primary selection between the host and the nested session through ext-data-control-v1 or wlr-data-control, with an inbound-only fallback through wl_data_device. Text only. Raises the wayland-protocols requirement to 1.39.

- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped), [`content-type`](#content-type).
- **Standalone branch:** [`topic/host-selection-sync`](https://github.com/KogasaPls/gamescope/tree/topic/host-selection-sync).

## Compatibility with specific software

### fifo-passthrough

Patch 24, [`b00c3fe7`](https://github.com/KogasaPls/gamescope/commit/b00c3fe7aecaa3ae0d6e0b3f3da1cee27a207a7f): layer: opt-in present mode passthrough for FIFO

Adds an opt-in that passes a client's FIFO present mode through to the driver instead of replacing it with MAILBOX. gamescope's own FIFO limits commits per vblank but never blocks vkQueuePresentKHR, so lsfg-vk, which paces its generated frames on that block, free-runs and drops frames. Also fixes a dangling pointer the layer left in the app's present-mode chain.

- **Switch:** `GAMESCOPE_WSI_PRESENT_MODE_PASSTHROUGH=1`: opt-in, set in the client environment.
- **Obsolete when:** lsfg-vk gains a pacing mode other than pacing = "none", or gamescope gives nested clients driver-level FIFO back-pressure (for example through wp_fifo_v1).
- **Standalone branch:** [`topic/fifo-passthrough`](https://github.com/KogasaPls/gamescope/tree/topic/fifo-passthrough).

### wayland-display-unset

Patch 28, [`adfc23b5`](https://github.com/KogasaPls/gamescope/commit/adfc23b5d58b4ce90158e07b1f3bf3d5eb888610): main: keep WAYLAND_DISPLAY unset for our clients

Unsets WAYLAND_DISPLAY for child processes instead of setting it to an empty string. Proton with its Wayland driver enabled treats any WAYLAND_DISPLAY as a usable display, disables X11, and then fails to create a window.

- **Obsolete when:** upstream blocks libwayland's wayland-0 fallback without setting WAYLAND_DISPLAY, or Proton checks the variable's value rather than its presence.
- **Standalone branch:** [`topic/wayland-display-unset`](https://github.com/KogasaPls/gamescope/tree/topic/wayland-display-unset).

## New options

### track-app-size

Patch 15, [`3544d8c9`](https://github.com/KogasaPls/gamescope/commit/3544d8c98c21fd13c3abd47f2134e43063c254e3): WaylandBackend: add --track-app-size

Adds --track-app-size, which resizes the nested window to follow the focused app's window instead of letterboxing the app inside a fixed output. Meant for apps whose window changes size, such as RuneLite, on a host that lets the window float. Implies --max-scale 1.

- **Switch:** `--track-app-size`: opt-in.
- **Switch:** `track_app_size_debug=1`: logs viewport changes.
- **Standalone branch:** [`topic/track-app-size`](https://github.com/KogasaPls/gamescope/tree/topic/track-app-size).

### exit-when-empty

Patch 21, [`2558187c`](https://github.com/KogasaPls/gamescope/commit/2558187cd93ed843818081c9c5eabfea7e2dc0e0): steamcompmgr: exit after a grace period with no client window

Adds --exit-when-empty, which exits once no client window has existed for a grace period. Launchers such as Battle.net keep helper processes running after their window closes, which otherwise leaves an empty gamescope window open.

- **Switch:** `--exit-when-empty[=seconds]`: opt-in; the default grace period is 30 seconds.
- **Depends on:** [`keep-toplevel-mapped`](#keep-toplevel-mapped).
- **Standalone branch:** [`topic/exit-when-empty`](https://github.com/KogasaPls/gamescope/tree/topic/exit-when-empty).

## Bug fixes

### rendervulkan-private-booleans

Patch 02, [`74bf9d57`](https://github.com/KogasaPls/gamescope/commit/74bf9d5714a63b5be17c9592c12167b3de8ad6b3): rendervulkan: Add missing private booleans.

Declares the fields Mesa reads from its private WSI structs and fills them member by member, so RADV no longer reads uninitialised bytes that can turn on implicit sync for imported buffers.

- **Source:** [#2247](https://github.com/ValveSoftware/gamescope/issues/2247) (Hans-Kristian Arntzen).
- **Standalone branch:** [`topic/rendervulkan-private-booleans`](https://github.com/KogasaPls/gamescope/tree/topic/rendervulkan-private-booleans).

### refresh-cycle-resend

Patch 03, [`bbaf800c`](https://github.com/KogasaPls/gamescope/commit/bbaf800c81931d8087eed28aa41ce249e558917c): wlserver: re-send refresh_cycle to a swapchain recreated on an existing surface

Re-sends the refresh cycle to a swapchain recreated on an existing surface. Without it the new swapchain answers vkGetRefreshCycleDurationGOOGLE with 60 Hz for its whole lifetime, whatever the real refresh rate (seen with Helldivers 2 at 240 Hz).

- **Standalone branch:** [`topic/refresh-cycle-resend`](https://github.com/KogasaPls/gamescope/tree/topic/refresh-cycle-resend).

### keep-deferred-xdg-commits

Patch 04, [`f67f8ebf`](https://github.com/KogasaPls/gamescope/commit/f67f8ebf6cfef22c677e0d2278e50df5b065bcbe): steamcompmgr: do not drop deferred xdg commits

Requeues every deferred commit from a native Wayland client, as the Xwayland path already does. The xdg path kept only the first commit that was not yet due and dropped the rest with their buffers.

- **Standalone branch:** [`topic/keep-deferred-xdg-commits`](https://github.com/KogasaPls/gamescope/tree/topic/keep-deferred-xdg-commits).

### held-button-focus

Patch 05, [`21b102b5`](https://github.com/KogasaPls/gamescope/commit/21b102b5e18cc9293994bbc30fd414eb28b13436): wlserver: hold pointer focus while a button is held

Defers a pointer focus change until the last held mouse button is released, as an implicit grab. wlroots forgets held buttons when focus moves and drops their release, which left clients with a stuck button.

- **Standalone branch:** [`topic/held-button-focus`](https://github.com/KogasaPls/gamescope/tree/topic/held-button-focus).

### confine-region

Patch 06, [`b4a80cd5`](https://github.com/KogasaPls/gamescope/commit/b4a80cd52658c58198c500d1ef3d6c9b16d8cbdd): wlserver: canonicalize the confine region and clamp inside it

Initialises the pointer confine region and ignores zero-area regions, which pixman reports as non-empty and which froze the pointer until the window was resized. Also keeps the activation warp and motion that starts outside the region inside it.

- **Standalone branch:** [`topic/confine-region`](https://github.com/KogasaPls/gamescope/tree/topic/confine-region).

### presentation-feedback-cancel

Patch 07, [`c91379f9`](https://github.com/KogasaPls/gamescope/commit/c91379f938b538a22d3d7dd7f30a11d1d22d5f41): WaylandBackend: cancel presentation feedback with its plane

Destroys a plane's pending presentation feedback with the plane, so late feedback events no longer reference freed memory.

- **Standalone branch:** [`topic/presentation-feedback-cancel`](https://github.com/KogasaPls/gamescope/tree/topic/presentation-feedback-cancel).

### modifier-forwarding

Patch 08, [`312d9208`](https://github.com/KogasaPls/gamescope/commit/312d920875a8ca66694c47dbac58d776a36983e0): Forward Wayland modifier events to wlroots seat when nested

Forwards the host's keyboard modifier state into the nested session. Without it Xwayland clients see every key press with no Shift, Ctrl or Alt held.

- **Source:** [#2273](https://github.com/ValveSoftware/gamescope/issues/2273) (bjornmp), with follow-up fixes.
- **Upstream issues:** [#1740](https://github.com/ValveSoftware/gamescope/issues/1740), [#2032](https://github.com/ValveSoftware/gamescope/issues/2032).
- **Standalone branch:** [`topic/modifier-forwarding`](https://github.com/KogasaPls/gamescope/tree/topic/modifier-forwarding).

### wm-state-long

Patch 09, [`df135b96`](https://github.com/KogasaPls/gamescope/commit/df135b9626b60c6b413b03f63128711dec9e6df0): steamcompmgr: write WM_STATE from a long array

Writes WM_STATE from an array of long, which is what Xlib reads for a format-32 property. The previous uint32_t pair made Xlib read past the array on 64-bit systems.

- **Standalone branch:** [`topic/wm-state-long`](https://github.com/KogasaPls/gamescope/tree/topic/wm-state-long).

### fullscreen-null-connector

Patch 10, [`4ae42fb1`](https://github.com/KogasaPls/gamescope/commit/4ae42fb125273f0f43463200e9ff409acd480420): WaylandBackend: skip the fullscreen toggle with no current connector

Ignores the fullscreen hotkey when no connector is current, instead of dereferencing null on the input thread.

- **Standalone branch:** [`topic/fullscreen-null-connector`](https://github.com/KogasaPls/gamescope/tree/topic/fullscreen-null-connector).

### fast-math-scope

Patch 11, [`acc2414c`](https://github.com/KogasaPls/gamescope/commit/acc2414cb8273ab47a59c9054c97250306c07025): build: scope fast math to color_helpers.cpp

Builds only color_helpers.cpp with -ffast-math, and without LTO, instead of the whole project, so the compositor and renderer use IEEE floating point without slowing the colour LUT rebuild. Also fixes a sanitising guard that tested defined(__FINITE_MATH_ONLY__), which is true in every build.

- **Source:** [#1494](https://github.com/ValveSoftware/gamescope/issues/1494) (sharkautarch), reworked.
- **Standalone branch:** [`topic/fast-math-scope`](https://github.com/KogasaPls/gamescope/tree/topic/fast-math-scope).

### icon-validation

Patch 13, [`51fa9509`](https://github.com/KogasaPls/gamescope/commit/51fa95096a358abcb2f7e9ce4ad5a1497d370161): backends: validate _NET_WM_ICON records before uploading an icon

Checks _NET_WM_ICON dimensions against the property's length and format before reading it, so a malformed icon cannot make a backend read past the buffer. Picks the largest icon up to 1024x1024 rather than the first.

- **Depends on:** [`color-manager-gate`](#color-manager-gate).
- **Standalone branch:** [`topic/icon-validation`](https://github.com/KogasaPls/gamescope/tree/topic/icon-validation).

### atomic-output-globals

Patch 16, [`3f9182b9`](https://github.com/KogasaPls/gamescope/commit/3f9182b9a1b4112924e05f5b42a4b78375672d5c): main: make the output size and HDR flag atomic

Makes the output size, refresh rate and HDR flag atomic. They are written on one thread and read on the input, capture and vblank threads, which is a data race.

- **Depends on:** [`track-app-size`](#track-app-size).
- **Standalone branch:** [`topic/atomic-output-globals`](https://github.com/KogasaPls/gamescope/tree/topic/atomic-output-globals).

### null-fb-present

Patch 18, [`48be7d16`](https://github.com/KogasaPls/gamescope/commit/48be7d16b53f158e9732c5cd402a4b9f0591a265): WaylandBackend: present no buffer for a texture without an fb

Presents no buffer for a layer whose texture has no backend framebuffer, such as a wl_shm buffer or a failed dmabuf import, instead of dereferencing null.

- **Depends on:** [`output-ring-gate`](#output-ring-gate).
- **Standalone branch:** [`topic/null-fb-present`](https://github.com/KogasaPls/gamescope/tree/topic/null-fb-present).

### subsurface-scale

Patch 19, [`2043d2eb`](https://github.com/KogasaPls/gamescope/commit/2043d2eb5c531bd5ce9c70d94d0ba8756da051d9): WaylandBackend: scale subsurface positions as signed values

Moves fractional-scale conversion into a tested helper with 64-bit signed arithmetic, so a negative subsurface position cannot pass through unsigned math.

- **Depends on:** [`null-fb-present`](#null-fb-present).
- **Standalone branch:** [`topic/subsurface-scale`](https://github.com/KogasaPls/gamescope/tree/topic/subsurface-scale).

### plane-teardown-lock

Patch 26, [`4548091c`](https://github.com/KogasaPls/gamescope/commit/4548091c0b8abac28dbe2ca990cf190d25523295): WaylandBackend: tear down planes under the input dispatch lock

Destroys a connector's planes only between input-thread dispatches, so event handlers never see a freed surface, and frees the plane's image description, which leaked.

- **Depends on:** [`presentation-feedback-cancel`](#presentation-feedback-cancel), [`content-type`](#content-type), [`pointer-focus-churn`](#pointer-focus-churn).
- **Standalone branch:** [`topic/plane-teardown-lock`](https://github.com/KogasaPls/gamescope/tree/topic/plane-teardown-lock).

## Build and tests

### patch-tests

Patch 01, [`2633934c`](https://github.com/KogasaPls/gamescope/commit/2633934ce9d20cc04ddec9760ee5228736312848): tests: add the patch-stack test target

Adds a gamescope_patch_tests executable built from every tests/patch/test_*.cpp, so the header-only helpers the other patches add are unit tested without any patch editing tests/meson.build.

- **Standalone branch:** [`topic/patch-tests`](https://github.com/KogasaPls/gamescope/tree/topic/patch-tests).
