#include "Backends/HostXTestInput.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <poll.h>
#include <cstdio>
#include <chrono>
#include <vector>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    gamescope::HostXTestInput bridge;
    if (!bridge.Init(argv[1])) { puts("bridge Init failed"); return 3; }
    Display *sender = XOpenDisplay(argv[1]);
    if (!sender) return 4;
    int forwarded = 0;
    double x = 0, y = 0;
    std::vector<std::pair<uint32_t, bool>> keys;
    auto drain = [&](bool focused) {
        bridge.Dispatch(focused, [&](uint32_t key, bool pressed) { keys.emplace_back(key, pressed); },
            [&](double dx, double dy) { ++forwarded; x += dx; y += dy; });
    };
    const auto fKey = XKeysymToKeycode(sender, XK_f);
    auto sendKey = [&](unsigned code, bool pressed, bool focused = true) {
        bridge.SetFocused(focused);
        const size_t before = keys.size();
        XTestFakeKeyEvent(sender, code, pressed, CurrentTime);
        XFlush(sender); // Keyboard-only traffic must flush without a mouse event or XSync.
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
        do {
            pollfd fd{bridge.GetFD(), POLLIN, 0};
            poll(&fd, 1, 5);
            drain(focused);
            if (focused && keys.size() > before) break;
        } while (std::chrono::steady_clock::now() < end && !bridge.Failed());
    };
    sendKey(fKey, true);
    if (keys != std::vector<std::pair<uint32_t, bool>>{{uint32_t(fKey - 8), true}}) {
        puts("FAIL: XTEST F press never reached the bridge keyboard sink"); return 14;
    }
    sendKey(fKey, false);
    if (keys != std::vector<std::pair<uint32_t, bool>>{{uint32_t(fKey - 8), true}, {uint32_t(fKey - 8), false}}) {
        puts("FAIL: XTEST F release did not reach the keyboard sink"); return 15;
    }
    sendKey(fKey, true, false);
    sendKey(fKey, false, false);
    if (keys.size() != 2) { puts("FAIL: unfocused keyboard traffic was forwarded"); return 16; }
    keys.clear();
    const auto control = XKeysymToKeycode(sender, XK_Control_L);
    sendKey(control, true); sendKey(fKey, true); sendKey(fKey, false); sendKey(control, false);
    if (keys != std::vector<std::pair<uint32_t, bool>>{{uint32_t(control - 8), true}, {uint32_t(fKey - 8), true},
            {uint32_t(fKey - 8), false}, {uint32_t(control - 8), false}}) {
        puts("FAIL: XTEST keyboard chord lost ordering"); return 17;
    }
    auto send = [&](int dx, int dy, bool focused = true) {
        bridge.SetFocused(focused);
        const int before = forwarded;
        XTestFakeRelativeMotionEvent(sender, dx, dy, CurrentTime);
        // No round trip: XSync conceals RECORD buffering. A device marker
        // may wake us before the next request, so drain until motion arrives.
        XFlush(sender);
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        do {
            pollfd fd{bridge.GetFD(), POLLIN, 0};
            poll(&fd, 1, 5);
            drain(focused);
            if (focused && forwarded > before) break;
        } while (std::chrono::steady_clock::now() < end && !bridge.Failed());
    };
    send(20, 30, false);
    if (forwarded) return 5;
    send(4, -3);
    if (forwarded != 1) return 6;
    send(7, -5);
    if (forwarded != 2 || x != 11 || y != -8) {
        printf("motion mismatch: count=%d dx=%f dy=%f\n", forwarded, x, y); return 7;
    }
    XTestFakeMotionEvent(sender, 0, 400, 300, CurrentTime);
    XSync(sender, False);
    pollfd absolute{bridge.GetFD(), POLLIN, 0};
    poll(&absolute, 1, 1000);
    drain(true);
    if (forwarded != 2 || x != 11 || y != -8) {
        puts("FAIL: absolute XTEST warp was forwarded as relative movement"); return 12;
    }
    for (int i = 0; i < 300; ++i) {
        send(7, -5);
    }
    if (forwarded != 302 || x != 2111 || y != -1508) {
        printf("sustained motion mismatch at screen edge: count=%d dx=%f dy=%f\n", forwarded, x, y); return 10;
    }
    // A non-XTEST cursor warp creates a device flush marker, not input
    // for the bridge. Physical motion must never be forwarded twice.
    XWarpPointer(sender, None, DefaultRootWindow(sender), 0, 0, 0, 0, 100, 100);
    XFlush(sender);
    pollfd physical{bridge.GetFD(), POLLIN, 0};
    poll(&physical, 1, 1000);
    drain(true);
    if (bridge.Failed() || forwarded != 302 || x != 2111 || y != -1508) return 13;
    pollfd idle{bridge.GetFD(), POLLIN, 0};
    poll(&idle, 1, 50);
    drain(true);
    if (forwarded != 302 || x != 2111 || y != -1508) return 11;
    bridge.OnPollHangUp();
    if (!bridge.Failed()) return 8;
    drain(true);
    if (forwarded != 302) return 9;
    XCloseDisplay(sender);
    puts("PASS: XTEST key press/release, keyboard-only flush, chord order, relative motion, absolute rejection, sustained movement, marker exclusion, focus gate, disconnect");
}
