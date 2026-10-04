#include "Backends/HostXTestInput.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <poll.h>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>

static void Require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char **argv) try {
    if (argc != 2) return 2;
    Display *host = XOpenDisplay(argv[1]), *sender = XOpenDisplay(argv[1]);
    Require(host && sender, "private display unavailable");
    int major = 2, minor = 0;
    Require(XIQueryVersion(host, &major, &minor) == Success, "XI2 unavailable");
    const Window root = DefaultRootWindow(host);
    const Window window = XCreateSimpleWindow(host, root, 0, 0, 640, 480, 0, 0, 0);
    XSelectInput(host, window, PointerMotionMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);
    XMapWindow(host, window);
    XSetInputFocus(host, window, RevertToPointerRoot, CurrentTime);
    XWarpPointer(host, None, root, 0, 0, 0, 0, 200, 200);
    XSync(host, False);
    struct Counts { unsigned motion = 0, keys = 0, buttons = 0; } received, forwarded;
    auto readHost = [&]() {
        XSync(host, False);
        while (XPending(host)) {
            XEvent e; XNextEvent(host, &e);
            received.motion += e.type == MotionNotify;
            received.keys += e.type == KeyPress || e.type == KeyRelease;
            received.buttons += e.type == ButtonPress || e.type == ButtonRelease;
        }
    };
    auto position = [&]() {
        Window rr, child; int rx, ry, wx, wy; unsigned mask;
        XQueryPointer(host, window, &rr, &child, &rx, &ry, &wx, &wy, &mask);
        return std::pair{rx, ry};
    };
    int count, physicalPointer = 0, physicalKeyboard = 0, xtestKeyboard = 0;
    auto *devices = XIQueryDevice(host, XIAllDevices, &count);
    const Atom property = XInternAtom(host, "XTEST Device", True);
    for (int i = 0; i < count; ++i) {
        const auto &device = devices[i];
        if (device.use != XISlavePointer && device.use != XISlaveKeyboard) continue;
        Atom type; int format; unsigned long items, remaining; unsigned char *value = nullptr;
        XIGetProperty(host, device.deviceid, property, 0, 1, False, XA_INTEGER, &type, &format, &items, &remaining, &value);
        const bool xtest = value && format == 8 && items == 1 && *value;
        XFree(value);
        if (xtest && device.use == XISlaveKeyboard) xtestKeyboard = device.deviceid;
        if (!xtest && device.use == XISlavePointer) physicalPointer = device.deviceid;
        if (!xtest && device.use == XISlaveKeyboard) physicalKeyboard = device.deviceid;
    }
    XIFreeDeviceInfo(devices);
    Require(physicalPointer && physicalKeyboard && xtestKeyboard, "private server device fixture incomplete");
    auto bridge = std::make_unique<gamescope::HostXTestInput>();
    Require(bridge->Init(argv[1]), "bridge initialization failed");
    auto pump = [&](bool focused) {
        // No sender round trip: receiving the captured key-only events must
        // flush RECORD independently of the host observer below.
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        do {
            pollfd fds[]{{bridge->GetFD(), POLLIN, 0}, {bridge->CaptureWaitable()->GetFD(), POLLIN, 0}};
            poll(fds, 2, 5);
            bridge->Dispatch(focused, [&](uint32_t, bool) { ++forwarded.keys; },
                [&](double, double) { ++forwarded.motion; });
        } while (std::chrono::steady_clock::now() < end);
        readHost();
    };
    const auto f = XKeysymToKeycode(sender, XK_f);
    auto send = [&]() {
        XTestFakeRelativeMotionEvent(sender, 9, -7, CurrentTime);
        XTestFakeKeyEvent(sender, f, True, CurrentTime);
        XTestFakeKeyEvent(sender, f, False, CurrentTime);
        XTestFakeButtonEvent(sender, 1, True, CurrentTime);
        XTestFakeButtonEvent(sender, 1, False, CurrentTime);
        XFlush(sender);
    };
    readHost(); received = {};
    send(); pump(false);
    Require(received.motion && received.keys == 2 && received.buttons == 2, "unfocused desktop input missing");
    Require(!forwarded.motion && !forwarded.keys, "unfocused input reached game");

    const auto control = XKeysymToKeycode(sender, XK_Control_L);
    auto requireHostReleased = [&]() {
        char keys[32]; XQueryKeymap(host, keys);
        Window rr, child; int rx, ry, wx, wy; unsigned mask;
        XQueryPointer(host, window, &rr, &child, &rx, &ry, &wx, &wy, &mask);
        Require(!(keys[control / 8] & (1 << (control % 8))) && !(mask & Button1Mask),
            "capture handover stranded a desktop modifier or button");
    };
    // Holds that began outside the game retain their host release. Include
    // releases racing acquisition, which a synchronous slave grab loses.
    for (int trial = 0; trial < 32; ++trial) {
        bridge->SetFocused(false);
        XTestFakeKeyEvent(sender, control, True, CurrentTime);
        XTestFakeButtonEvent(sender, 1, True, CurrentTime);
        XSync(sender, False);
        std::thread release([&]() {
            std::this_thread::sleep_for(std::chrono::microseconds(trial * 10));
            XTestFakeKeyEvent(sender, control, False, CurrentTime);
            XTestFakeButtonEvent(sender, 1, False, CurrentTime);
            XSync(sender, False);
        });
        bridge->SetFocused(true);
        release.join();
        bridge->Dispatch(true);
        bridge->SetFocused(false);
        Require(!bridge->Failed(), "busy devices permanently disabled capture");
        requireHostReleased();
    }
    pump(false);
    // Stale host requests and capture-boundary markers must never be replayed
    // when focus is reacquired before the RECORD stream was drained.
    bridge->SetFocused(true); bridge->SetFocused(false);
    send(); XSync(sender, False);
    Require(bridge->SetFocused(true), "stale stream setup failed");
    forwarded = {}; pump(true);
    Require(!forwarded.motion && !forwarded.keys, "pre-capture host input replayed into game");
    bridge->SetFocused(false); readHost();
    received = {}; forwarded = {};
    Require(bridge->SetFocused(true), "focused capture failed");
    const auto before = position();
    send(); pump(true);
    Require(forwarded.motion == 1 && forwarded.keys == 2, "focused input did not reach game exactly once");
    Require(!received.motion && !received.keys && !received.buttons && position() == before,
        "focused XTEST input leaked to desktop cursor or controls");
    forwarded = {};
    XTestFakeKeyEvent(sender, f, True, CurrentTime);
    XFlush(sender); pump(true);
    Require(forwarded.keys == 1, "captured key-only press buffered");
    XTestFakeKeyEvent(sender, f, False, CurrentTime);
    XFlush(sender); pump(true);
    Require(forwarded.keys == 2 && !received.keys, "captured key-only release buffered or leaked");

    // Inject through Xvfb's non-XTEST devices to simulate normal host input.
    // Extended FakeInput requests are intentionally ignored by the bridge.
    auto *pointer = XOpenDevice(sender, physicalPointer);
    auto *keyboard = XOpenDevice(sender, physicalKeyboard);
    Require(pointer && keyboard, "could not open non-XTEST fixture devices");
    int axes[]{5, 3};
    XTestFakeDeviceMotionEvent(sender, pointer, True, 0, axes, 2, CurrentTime);
    XTestFakeDeviceKeyEvent(sender, keyboard, f, True, nullptr, 0, CurrentTime);
    XTestFakeDeviceKeyEvent(sender, keyboard, f, False, nullptr, 0, CurrentTime);
    XFlush(sender); forwarded = {}; pump(true);
    Require(received.motion && received.keys == 2, "capture interfered with normal host input");
    Require(!forwarded.motion && !forwarded.keys, "normal input was duplicated by bridge");
    bridge->SetFocused(false);
    XTestFakeDeviceKeyEvent(sender, keyboard, control, True, nullptr, 0, CurrentTime);
    XSync(sender, False);
    Require(bridge->SetFocused(true), "a physical modifier incorrectly prevented XTEST capture");
    XTestFakeDeviceKeyEvent(sender, keyboard, control, False, nullptr, 0, CurrentTime);
    XSync(sender, False);
    requireHostReleased();
    XCloseDevice(sender, pointer); XCloseDevice(sender, keyboard);
    readHost();

    bridge->SetFocused(false); received = {}; forwarded = {};
    send(); pump(false);
    Require(received.motion && received.keys == 2 && received.buttons == 2, "focus loss failed to restore desktop input");
    Require(!forwarded.motion && !forwarded.keys, "focus loss continued forwarding");
    Require(bridge->SetFocused(true), "focus reacquisition failed");
    bridge->OnPollHangUp();
    bridge->Dispatch(true); // production failure path must release capture
    Require(bridge->Failed(), "recording failure not detected");
    received = {}; send(); pump(false);
    Require(received.motion && received.keys == 2 && received.buttons == 2, "recording failure retained capture");
    bridge.reset();

    bridge = std::make_unique<gamescope::HostXTestInput>();
    Require(bridge->Init(argv[1]) && bridge->SetFocused(true), "capture failure setup failed");
    bridge->CaptureWaitable()->OnPollHangUp();
    bridge->Dispatch(true);
    Require(bridge->Failed(), "capture failure not detected");
    received = {}; send(); pump(false);
    Require(received.motion && received.keys == 2 && received.buttons == 2, "capture failure retained grab");
    bridge.reset();

    // Force the second grab to fail, after the pointer was already grabbed.
    unsigned char mask[1]{};
    XIEventMask grabMask{xtestKeyboard, 1, mask};
    Require(XIGrabDevice(host, xtestKeyboard, root, CurrentTime, None, GrabModeAsync, GrabModeAsync,
        False, &grabMask) == GrabSuccess, "conflicting grab setup failed");
    bridge = std::make_unique<gamescope::HostXTestInput>();
    Require(bridge->Init(argv[1]), "conflict bridge initialization failed");
    bridge->SetFocused(true);
    Require(bridge->Failed(), "conflicting capture silently accepted");
    received = {}; XTestFakeRelativeMotionEvent(sender, 9, -7, CurrentTime); XFlush(sender); pump(false);
    Require(received.motion, "partial failure left pointer captured");
    XIUngrabDevice(host, xtestKeyboard, CurrentTime); XSync(host, False);
    bridge.reset();
    bridge = std::make_unique<gamescope::HostXTestInput>();
    Require(bridge->Init(argv[1]) && bridge->SetFocused(true), "destructor setup failed");
    bridge.reset();
    // Synchronize with the server observing the closed capture connection.
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    bool restored = false;
    do {
        devices = XIQueryDevice(host, xtestKeyboard, &count);
        restored = devices && count == 1 && devices[0].use == XISlaveKeyboard;
        XIFreeDeviceInfo(devices);
    } while (!restored && std::chrono::steady_clock::now() < end);
    Require(restored, "destruction did not reattach XTEST keyboard");
    received = {}; send(); XSync(sender, False); readHost();
    Require(received.motion && received.keys == 2 && received.buttons == 2, "destruction did not restore desktop input");
    XCloseDisplay(sender); XCloseDisplay(host);
    puts("PASS: focused XTEST isolation, key-only flush, normal host input, focus release, failure rollback, disconnect restoration");
} catch (const std::exception &error) {
    fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
