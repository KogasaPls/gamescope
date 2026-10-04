#include "Backends/HostXTestMouse.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <poll.h>
#include <cstdio>
#include <chrono>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    gamescope::HostXTestMouse bridge;
    if (!bridge.Init(argv[1])) { puts("bridge Init failed"); return 3; }
    Display *sender = XOpenDisplay(argv[1]);
    if (!sender) return 4;
    int forwarded = 0;
    double x = 0, y = 0;
    auto drain = [&](bool focused) {
        for (const auto &motion : bridge.Dispatch(focused).Motions()) { ++forwarded; x += motion[0]; y += motion[1]; }
    };
    auto send = [&](int dx, int dy, bool focused = true) {
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
    puts("PASS: relative XTEST delivery, absolute warp rejection, unbuffered sustained movement, device marker exclusion, focus gate, release, disconnect");
}
