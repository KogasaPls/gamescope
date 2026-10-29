#!/usr/bin/env bash
# Real Gamescope -> private DRM-rendered headless Sway launch-tag regression.
# Run through build-run. Never connects to the desktop compositor or its IPC.
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
gs_bin=${GAMESCOPE_BIN:-$repo/build/src/gamescope}
ctl_bin=${GAMESCOPE_CTL_BIN:-$repo/build/src/gamescopectl}
sway_bin=${SWAY_BIN:-$(command -v sway)}
client_bin=${XTERM_BIN:-/usr/bin/xterm}
work=$(mktemp -d /tmp/gamescope-vrr-launch-tag.XXXXXX)
sway_pid=
gs_pid=
passed=false

stop_group() {
	local pid=$1 i
	[[ -n $pid ]] || return 0
	kill -TERM -- "-$pid" 2>/dev/null || true
	for ((i=0; i<100; i++)); do
		kill -0 -- "-$pid" 2>/dev/null || break
		sleep 0.05
	done
	kill -KILL -- "-$pid" 2>/dev/null || true
	wait "$pid" 2>/dev/null || true
}
cleanup() {
	local status=$?
	trap - EXIT INT TERM
	stop_group "$gs_pid"
	stop_group "$sway_pid"
	if [[ $passed == true && ${KEEP_TEST_LOGS:-0} != 1 ]]; then
		rm -rf -- "$work"
	else
		printf 'Test logs retained: %s\n' "$work" >&2
	fi
	exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }

for tool in setsid swaymsg jq timeout; do command -v "$tool" >/dev/null || fail "missing $tool"; done
[[ -x $gs_bin && -x $ctl_bin && -x $sway_bin && -x $client_bin ]] || fail 'Gamescope, gamescopectl, Sway or Xterm binary missing'
mkdir -m 700 "$work/host" "$work/off" "$work/on" "$work/config-home" "$work/cache-home" "$work/data-home"
cat > "$work/sway.conf" <<'CONF'
xwayland disable
font monospace 10
focus_follows_mouse no
output * adaptive_sync auto
for_window [app_id="^gamescope$" tag="^adaptive-sync$"] allow_adaptive_sync yes
CONF

# Clean environment prevents inherited desktop sockets, overlays and startup
# service clients. Only a test-owned Wayland socket is supplied to Gamescope.
clean_env=(env -i "PATH=$PATH" "HOME=$work" LANG=C.UTF-8
	"XDG_CONFIG_HOME=$work/config-home" "XDG_CACHE_HOME=$work/cache-home"
	"XDG_DATA_HOME=$work/data-home" "DBUS_SESSION_BUS_ADDRESS=unix:path=$work/no-session-bus"
	"PIPEWIRE_REMOTE=$work/no-pipewire" "PULSE_SERVER=unix:$work/no-pulse")
host_env=("XDG_RUNTIME_DIR=$work/host" WLR_BACKENDS=headless
	WLR_RENDERER=gles2 WLR_HEADLESS_OUTPUTS=1 WLR_LIBINPUT_NO_DEVICES=1)
if [[ -n ${GAMESCOPE_TEST_RENDER_NODE:-} ]]; then
	host_env+=("WLR_RENDER_DRM_DEVICE=$GAMESCOPE_TEST_RENDER_NODE")
fi
printf 'Starting an isolated headless Sway and two silent Gamescope launches; no desktop windows will appear.\n'
setsid "${clean_env[@]}" "${host_env[@]}" "$sway_bin" -d -c "$work/sway.conf" >"$work/sway.log" 2>&1 &
sway_pid=$!
sock=$work/host/sway-ipc.$(id -u).$sway_pid.sock
host_display=
for ((i=0; i<200; i++)); do
	if ! kill -0 "$sway_pid" 2>/dev/null; then fail 'private Sway exited; inspect sway.log (DRM/DMABUF rendering is required)'; fi
	if [[ -S $sock ]]; then
		for path in "$work/host"/wayland-*; do
			if [[ -S $path ]]; then host_display=$path; break; fi
		done
		[[ -n $host_display ]] && break
	fi
	sleep 0.05
done
[[ -S $sock && -S $host_display ]] || fail 'private Sway did not expose its sockets'
ipc() { timeout 5s swaymsg -s "$sock" -r "$@"; }
view() {
	ipc -t get_tree | jq -c '[.. | objects | select(.app_id? == "gamescope")] | if length == 1 then .[0] else empty end'
}
expect() {
	local label=$1 expected=$2 current
	current=$(view)
	printf '%s\n' "$current" >"$work/$label-view.json"
	[[ -n $current ]] || fail "$label: expected exactly one mapped Gamescope view"
	if [[ $expected == true ]]; then
		jq -e '.tag == "adaptive-sync" and .allow_adaptive_sync == true and .fullscreen_mode == 1' <<<"$current" >/dev/null ||
			fail "$label: --adaptive-sync did not set the tag and opt the fullscreen view in"
	else
		jq -e '.tag == null and .allow_adaptive_sync == false and .fullscreen_mode == 1' <<<"$current" >/dev/null ||
			fail "$label: launch without --adaptive-sync must stay untagged and opted out"
	fi
}
run_launch() {
	local name=$1 expected=$2 i current
	shift 2
	setsid "${clean_env[@]}" "XDG_RUNTIME_DIR=$work/$name" "WAYLAND_DISPLAY=$host_display" WAYLAND_DEBUG=client \
		"$gs_bin" --backend wayland -W 320 -H 240 -r 30 -f "$@" -- "$client_bin" -title launch-tag-test -e sleep 60 >"$work/$name.log" 2>&1 &
	gs_pid=$!
	for ((i=0; i<300; i++)); do
		kill -0 "$gs_pid" 2>/dev/null || fail "$name: Gamescope exited before its outer view mapped; inspect $name.log"
		current=$(view)
		if [[ -n $current ]] && jq -e ".fullscreen_mode == 1" <<<"$current" >/dev/null; then break; fi
		sleep 0.05
	done
	expect "$name" "$expected"
	if [[ $expected == true ]]; then
		grep -Eq 'xdg_toplevel_tag_manager_v1.*\.set_toplevel_tag\(.*"adaptive-sync"' "$work/$name.log" ||
			fail "$name: no actual adaptive-sync protocol request was observed"
	else
		if grep -Eq 'xdg_toplevel_tag_manager_v1.*\.set_toplevel_tag\(' "$work/$name.log"; then
			fail "$name: an unrequested launch sent a toplevel tag"
		fi
	fi
	printf 'PASS: %s launch has expected protocol tag and Sway opt-in\n' "$name"

	# Exercise the real runtime control on this test instance, then cause new
	# host configures. A launch tag must neither appear nor disappear later.
	local nested_display='' value=1 requests
	[[ $expected == true ]] && value=0
	for path in "$work/$name"/gamescope-*; do
		if [[ -S $path ]]; then nested_display=$path; break; fi
	done
	[[ -n $nested_display ]] || fail "$name: private Gamescope control socket missing"
	timeout 5s "${clean_env[@]}" "XDG_RUNTIME_DIR=$work/$name" \
		"GAMESCOPE_WAYLAND_DISPLAY=$nested_display" "$ctl_bin" adaptive_sync "$value" \
		>"$work/$name-control.log" 2>&1 || fail "$name: runtime preference change failed"
	for mode in disable enable; do
		ipc "[app_id=\"^gamescope$\"] fullscreen $mode" | jq -e 'all(.[]; .success)' >/dev/null
		local wanted=0
		[[ $mode == enable ]] && wanted=1
		for ((i=0; i<100; i++)); do
			current=$(view)
			if [[ -n $current ]] && jq -e --argjson wanted "$wanted" '.fullscreen_mode == $wanted' <<<"$current" >/dev/null; then break; fi
			sleep 0.05
		done
		jq -e --argjson wanted "$wanted" '.fullscreen_mode == $wanted' <<<"$current" >/dev/null || fail "$name: private fullscreen transition failed"
	done
	expect "$name-runtime" "$expected"
	requests=$(grep -Ec 'xdg_toplevel_tag_manager_v1.*\.set_toplevel_tag\(' "$work/$name.log" || true)
	if [[ $expected == true ]]; then
		[[ $requests == 1 ]] || fail "$name: launch tag must be sent exactly once"
	else
		[[ $requests == 0 ]] || fail "$name: runtime change unexpectedly added a launch tag"
	fi
	printf 'PASS: %s launch tag stays unchanged after runtime preference change and reconfigure\n' "$name"
	stop_group "$gs_pid"
	gs_pid=
	for ((i=0; i<100; i++)); do
		current=$(ipc -t get_tree | jq '[.. | objects | select(.app_id? == "gamescope")] | length')
		[[ $current == 0 ]] && return 0
		sleep 0.05
	done
	fail "$name: test-owned view survived process-group cleanup"
}

run_launch off false
run_launch on true --adaptive-sync
kill -0 "$sway_pid" || fail 'private Sway did not survive both launches'
passed=true
printf 'PASS: requesting launch opts in; default launch stays opted out\n'
