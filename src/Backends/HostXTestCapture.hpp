#pragma once
#include <atomic>
#include <algorithm>
#include <cstring>
#include <span>
#include <cstdlib>
#include <vector>
#include <xcb/xinput.h>
#include <xcb/xcbext.h>
#include "waitable.h"
#include "xtest_input_helpers.hpp"

namespace gamescope
{
// A slave-device grab temporarily detaches only XTEST from its master. The
// server restores the attachment on ungrab or client disconnect. Never grab
// master devices: physical Wayland/X11 input must remain independent.
class HostXTestCapture final : public IWaitable
{
public:
    ~HostXTestCapture() { if (m_connection) xcb_disconnect(m_connection); }
    bool Init(const char *display)
    {
        m_connection = xcb_connect(display, nullptr);
        if (xcb_connection_has_error(m_connection)) return false;
        auto *version = xcb_input_xi_query_version_reply(m_connection,
            xcb_input_xi_query_version(m_connection, 2, 0), nullptr);
        const bool supported = version && version->major_version >= 2;
        free(version);
        if (!supported) return false;
        constexpr char property[] = "XTEST Device";
        auto *atom = xcb_intern_atom_reply(m_connection,
            xcb_intern_atom(m_connection, true, sizeof(property) - 1, property), nullptr);
        m_xtestProperty = atom ? atom->atom : xcb_atom_t{XCB_ATOM_NONE};
        free(atom);
        return m_xtestProperty != XCB_ATOM_NONE;
    }
    int GetFD() override { return xcb_get_file_descriptor(m_connection); }
    void OnPollHangUp() override { m_failed = true; }
    bool Failed() const { return m_failed || !m_connection || xcb_connection_has_error(m_connection); }
    bool Captured() const { return !m_grabbed.empty(); }
    bool IsCaptureBoundary(uint32_t client, std::span<const uint8_t> data) const
    {
        if (client != xcb_get_setup(m_connection)->resource_id_base) return false;
        // Only our tagged NoOperations are recorded on the control connection.
        for (size_t offset = 0; offset + 8 <= data.size(); offset += 8)
        {
            uint32_t fence;
            std::memcpy(&fence, data.data() + offset + 4, sizeof(fence));
            if (data[offset] == XCB_NO_OPERATION && fence == m_fence) return true;
        }
        return false;
    }
    bool Drain()
    {
        // Captured events keep RECORD output flowing but carry no forwarded
        // input. Their queue can also be flooded independently of RECORD.
        xtest_input::DispatchBudget budget;
        while (budget.Take())
        {
            auto *event = xcb_poll_for_event(m_connection);
            if (!event) return false;
            if (event->response_type == 0) m_failed = true;
            free(event);
            if (Failed()) return false;
        }
        return true; // Revisit XCB-buffered events without waiting for a socket edge.
    }
    bool SetFocused(bool focused)
    {
        if (!focused)
        {
            if (m_grabbed.empty()) return true;
            for (auto device : m_grabbed)
                xcb_input_xi_ungrab_device(m_connection, XCB_CURRENT_TIME, device);
            m_grabbed.clear();
            // Complete focus release before another client can start using
            // XTEST again. This connection is separate from RECORD's stream.
            free(xcb_get_input_focus_reply(m_connection, xcb_get_input_focus(m_connection), nullptr));
            return true;
        }
        if (Failed()) return false;
        if (!m_grabbed.empty()) return true;

        // Serialize the neutral check and device grabs against other clients'
        // FakeInput requests. A frozen-device grab alone cannot do this: a
        // queued release can be consumed by the detached slave on rollback,
        // leaving its old master down. Physical devices are never grabbed.
        xcb_grab_server(m_connection);
        struct ServerGrab {
            xcb_connection_t *connection;
            ~ServerGrab() { xcb_ungrab_server(connection); xcb_flush(connection); }
        } serverGrab{m_connection};
        auto *devices = xcb_input_xi_query_device_reply(m_connection,
            xcb_input_xi_query_device(m_connection, XCB_INPUT_DEVICE_ALL), nullptr);
        std::vector<xcb_input_device_id_t> candidates;
        if (devices)
        {
            for (auto it = xcb_input_xi_query_device_infos_iterator(devices); it.rem; xcb_input_xi_device_info_next(&it))
            {
                const auto &device = *it.data;
                if (!device.enabled || device.type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER ||
                    device.type == XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD)
                    continue;
                auto *property = xcb_input_xi_get_property_reply(m_connection,
                    xcb_input_xi_get_property(m_connection, device.deviceid, false,
                        m_xtestProperty, XCB_ATOM_INTEGER, 0, 1), nullptr);
                if (!property) { m_failed = true; break; }
                // The server's immutable property identifies XTEST; a device
                // name or numerical ID alone could identify a physical device.
                const bool xtest = property && property->type == XCB_ATOM_INTEGER &&
                    property->format == 8 && property->num_items == 1 &&
                    *static_cast<uint8_t *>(xcb_input_xi_get_property_items(property)) == 1;
                free(property);
                if (!xtest) continue;
                candidates.push_back(device.deviceid);
            }
            free(devices);
        }
        // A partial grab must never leave desktop input captured after the
        // bridge disables itself. Normally at least the core pair is present.
        if (!devices || candidates.size() < 2 || Failed())
        {
            m_failed = true;
            SetFocused(false);
            return false;
        }
        // Existing holds and their releases belong to the host. Leave the
        // devices attached and retry on later input activity until neutral.
        for (auto device : candidates)
        {
            bool neutral = true;
            auto *state = device <= UINT8_MAX ? xcb_input_query_device_state_reply(m_connection,
                xcb_input_query_device_state(m_connection, uint8_t(device)), nullptr) : nullptr;
            if (!state) m_failed = true;
            else
            {
                for (auto it = xcb_input_query_device_state_classes_iterator(state); it.rem; xcb_input_input_state_next(&it))
                {
                    const uint8_t *held = nullptr;
                    if (it.data->class_id == XCB_INPUT_INPUT_CLASS_KEY)
                        held = reinterpret_cast<xcb_input_key_state_t *>(it.data)->keys;
                    if (it.data->class_id == XCB_INPUT_INPUT_CLASS_BUTTON)
                        held = reinterpret_cast<xcb_input_button_state_t *>(it.data)->buttons;
                    if (held && std::any_of(held, held + 32, [](uint8_t bits) { return bits != 0; }))
                        neutral = false;
                }
                free(state);
            }
            if (!neutral || Failed()) return false;
        }
        const auto root = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data->root;
        for (auto device : candidates)
        {
            const uint32_t mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE |
                XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION;
            auto *reply = xcb_input_xi_grab_device_reply(m_connection,
                xcb_input_xi_grab_device(m_connection, root, XCB_CURRENT_TIME, XCB_CURSOR_NONE,
                    device, XCB_INPUT_GRAB_MODE_22_ASYNC, XCB_INPUT_GRAB_MODE_22_ASYNC, false, 1, &mask), nullptr);
            const bool grabbed = reply && reply->status == XCB_GRAB_STATUS_SUCCESS;
            free(reply);
            if (!grabbed) { m_failed = true; SetFocused(false); return false; }
            m_grabbed.push_back(device);
        }
        // RECORD is a separate ordered stream. Tag its exact capture boundary
        // before unblocking senders, so stale host input is never replayed.
        struct { uint8_t opcode, pad; uint16_t length; uint32_t fence; } marker{XCB_NO_OPERATION, 0, 2, ++m_fence};
        iovec parts[4]{};
        parts[2] = {&marker, sizeof(marker)};
        const xcb_protocol_request_t request{2, nullptr, XCB_NO_OPERATION, true};
        xcb_send_request(m_connection, 0, parts + 2, &request);
        return true;
    }
private:
    xcb_connection_t *m_connection = nullptr;
    xcb_atom_t m_xtestProperty = XCB_ATOM_NONE;
    std::vector<xcb_input_device_id_t> m_grabbed;
    std::atomic<bool> m_failed = false;
    uint32_t m_fence = 0;
};
}
