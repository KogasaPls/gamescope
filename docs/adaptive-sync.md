# Adaptive sync with nested Wayland Gamescope

On hosts supporting `xdg-toplevel-tag-v1`, Gamescope gives its outer window
the tag `adaptive-sync` when adaptive sync is enabled at window creation,
normally by launching with `--adaptive-sync`. The app ID remains `gamescope`
and the window title continues to follow the game. Without the flag, no tag
is sent. Hosts without the tagging protocol continue to work without a tag.

With the patched Sway `allow_adaptive_sync` command, use:

```sway
output DP-1 adaptive_sync auto
for_window [app_id="^gamescope$" tag="^adaptive-sync$"] allow_adaptive_sync yes
```

Replace any broader gamescope opt-in rule with this rule. Otherwise the
broader rule can still opt untagged launches in. Restart already-running
Gamescope instances to apply the launch tag.

The flag in each game's launch options now selects VRR without a separate
per-game title rule. Sway still requires a fullscreen view and an unlocked
session, and the display must support VRR. Output `adaptive_sync on` bypasses
the per-window policy.

The tag records the preference at window creation. Runtime `adaptive_sync`
changes do not update or clear it; this is deliberately a launch-time hint.
It does not report whether VRR is actually active, and it does not change
tearing policy. This applies to the native Wayland backend, not SDL/X11 or DRM.

## Regression test

After building Gamescope and the patched Sway, run through `build-run`:

```sh
SWAY_BIN="$HOME/git/sway/build/sway/sway" build-run tests/adaptive-sync-launch.sh
```

The test starts private headless Sway and actual Gamescope instances with and
without `--adaptive-sync`. It checks the protocol request and Sway's tag and
per-window permission over IPC, then flips the runtime preference and checks
that further configures do not retag the window. It needs working DRM/DMABUF
rendering, Sway with the opt-in patch, swaymsg, jq, setsid, timeout, Xwayland,
and xterm. No
windows appear in the running session and no audio is played. Physical VRR
transitions are not tested. Failure logs are retained in the printed directory.
