#include "Backends/HostXTestInput.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <poll.h>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>

static void Require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
int main(int argc, char **argv) try {
    if (argc != 2) return 2;
    Display *sender = XOpenDisplay(argv[1]);
    Require(sender, "private display unavailable");
    const auto key = XKeysymToKeycode(sender, XK_f);
    {
        gamescope::HostXTestInput bridge;
        Require(bridge.Init(argv[1]) && bridge.SetFocused(true), "capture initialization failed");
        constexpr unsigned sent = 2048;
        for (unsigned i = 0; i < sent; ++i)
            XTestFakeKeyEvent(sender, key, !(i % 2), CurrentTime);
        XSync(sender, False); // Synchronize the backlog; not a flush-liveness test.
        unsigned received = 0;
        auto keySink = [&](uint32_t code, bool pressed) {
            Require(code == unsigned(key - 8) && pressed == !(received % 2), "continuation lost key order");
            ++received;
        };
        bool more = bridge.Dispatch(true, keySink);
        Require(received > 0 && received <= 256 && more, "one dispatch monopolized the complete keyboard backlog");
        auto before = received;
        more = bridge.Dispatch(true, [&](uint32_t code, bool pressed) {
            keySink(code, pressed);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
        Require(received == before + 1 && more, "slow sink exceeded the dispatch time budget");

        // No further sender requests: continuations must service data already
        // buffered inside XCB even when neither socket becomes readable again.
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (received < sent && std::chrono::steady_clock::now() < end) {
            if (!more) {
                pollfd fds[]{{bridge.GetFD(), POLLIN, 0}, {bridge.CaptureWaitable()->GetFD(), POLLIN, 0}};
                poll(fds, 2, 5);
            }
            before = received;
            more = bridge.Dispatch(true, keySink);
            Require(received - before <= 256, "continuation had unbounded work");
        }
        Require(received == sent && !bridge.Failed(), "buffered key events stalled or were dropped");
        for (unsigned i = 0; more && i < 100; ++i) more = bridge.Dispatch(true, keySink);
        Require(!more && received == sent, "continuations never returned to idle or duplicated input");

        std::vector<int> order;
        bool held = false;
        double moved = 0;
        XTestFakeKeyEvent(sender, key, True, CurrentTime);
        XTestFakeRelativeMotionEvent(sender, 1, 0, CurrentTime);
        XTestFakeKeyEvent(sender, key, False, CurrentTime);
        XSync(sender, False);
        for (unsigned i = 0; i < 100 && order.size() < 3; ++i) {
            bridge.Dispatch(true, [&](uint32_t, bool pressed) {
                held = pressed; order.push_back(pressed ? 1 : 3);
            }, [&](double x, double y) {
                Require(held && x == 1 && y == 0, "movement escaped its F hold or changed displacement");
                moved += x; order.push_back(2);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        Require(order == std::vector<int>{1, 2, 3}, "mixed input lost wire order");
        for (unsigned i = 0; i < 60; ++i) {
            bridge.Dispatch(true, {}, [&](double x, double) { moved += x; });
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        Require(moved == 1, "idle dispatch invented displacement");

        // Focus loss while a reply is partly consumed must discard its tail,
        // release capture immediately, and exclude it on reacquisition.
        for (unsigned i = 0; i < sent; ++i)
            XTestFakeKeyEvent(sender, key, !(i % 2), CurrentTime);
        XSync(sender, False);
        more = bridge.Dispatch(true, keySink);
        Require(more, "focus-loss setup did not leave pending input");
        before = received;
        bridge.SetFocused(false);
        Require(bridge.SetFocused(true), "focus reacquisition failed");
        // Reacquire before draining the old reply tail: the new boundary
        // must exclude it even though Dispatch is focused again.
        for (unsigned i = 0; more && i < 100; ++i) more = bridge.Dispatch(true, keySink);
        Require(!more && received == before, "old input replayed after reacquisition");
        bridge.SetFocused(false);
    }
    // Exercise capture's own queue without RECORD consuming its events.
    {
        gamescope::HostXTestCapture capture;
        Require(capture.Init(argv[1]) && capture.SetFocused(true), "capture-only initialization failed");
        for (unsigned i = 0; i < 2048; ++i)
            XTestFakeRelativeMotionEvent(sender, 1, 0, CurrentTime);
        XSync(sender, False);
        bool more = capture.Drain();
        Require(more, "capture drain had no continuation for a large backlog");
        unsigned passes = 0;
        while (more && ++passes < 100) more = capture.Drain();
        Require(!more && !capture.Failed(), "capture continuation failed to empty its buffered events");
    }
    XCloseDisplay(sender);
    puts("PASS: bounded RECORD/capture dispatch, slow sinks, buffered continuation, exact ordered movement, focus cancellation");
} catch (const std::exception &error) {
    fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
}
