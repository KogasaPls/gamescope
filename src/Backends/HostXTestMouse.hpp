#pragma once
#include <xcb/record.h>
#include <xcb/xcbext.h>
#include <cstdlib>
#include "waitable.h"
#include "xtest_mouse_helpers.hpp"

namespace gamescope
{
// Input-thread owned. Forward only XTEST FakeInput requests. Mouse device
// events are flush markers, never forwarded; no keyboard traffic is recorded.
// XI2 RawMotion loses the relative/absolute flag.
class HostXTestMouse final : public IWaitable
{
public:
    ~HostXTestMouse() { if (m_connection) xcb_disconnect(m_connection); }
    bool Init(const char *display)
    {
        if (!display || !*display)
            return false;
        m_connection = xcb_connect(display, nullptr);
        if (xcb_connection_has_error(m_connection))
            return false;
        static xcb_extension_t xtestExtension{"XTEST", 0};
        const auto *xtest = xcb_get_extension_data(m_connection, &xtestExtension);
        const auto *record = xcb_get_extension_data(m_connection, &xcb_record_id);
        if (!xtest || !xtest->present || !record || !record->present)
            return false;
        m_opcode = xtest->major_opcode;
        auto *version = xcb_record_query_version_reply(m_connection,
            xcb_record_query_version(m_connection, 1, 13), nullptr);
        const bool supported = version && version->major_version == 1 && version->minor_version >= 13;
        free(version);
        if (!supported)
            return false;
        const xcb_record_context_t context = xcb_generate_id(m_connection);
        const xcb_record_client_spec_t clients = XCB_RECORD_CS_ALL_CLIENTS;
        xcb_record_range_t range{};
        range.ext_requests.major = {m_opcode, m_opcode};
        range.ext_requests.minor = {2, 2}; // XTEST FakeInput
        // RECORD buffers requests until ordinary server output is flushed.
        // Native Wayland focus / a cursor at the X root edge may produce no
        // such output. Motion device events force critical output pending
        // and a category boundary, flushing each request without polling.
        range.device_events = {6, 6}; // MotionNotify, discarded below
        auto *error = xcb_request_check(m_connection,
            xcb_record_create_context_checked(m_connection, context, 0, 1, 1, &clients, &range));
        if (error) { free(error); return false; }
        m_sequence = xcb_record_enable_context(m_connection, context).sequence;
        auto *start = static_cast<xcb_record_enable_context_reply_t *>(
            xcb_wait_for_reply(m_connection, m_sequence, &error));
        const bool started = !error && start && start->category == 4; // StartOfData
        free(error);
        free(start);
        // No requests may follow EnableContext on the recording connection.
        // Disconnecting its owner frees the context and ends recording.
        return started && !xcb_connection_has_error(m_connection);
    }
    int GetFD() override { return xcb_get_file_descriptor(m_connection); }
    void OnPollHangUp() override { m_failed = true; }
    bool Failed() const { return m_failed || !m_connection || xcb_connection_has_error(m_connection); }

    xtest_mouse::MotionBatch Dispatch(bool focused)
    {
        xtest_mouse::MotionBatch batch;
        if (Failed())
            return batch;
        void *reply = nullptr;
        xcb_generic_error_t *error = nullptr;
        while (xcb_poll_for_reply(m_connection, m_sequence, &reply, &error))
        {
            auto *packet = static_cast<xcb_record_enable_context_reply_t *>(reply);
            if (error || !packet || packet->element_header != 0)
                m_failed = true;
            else
            {
                const size_t bytes = size_t(packet->length) * 4;
                auto *data = reinterpret_cast<const uint8_t *>(packet + 1);
                // Variable XI requests must be skipped without losing framing.
                if (!xtest_mouse::DispatchRecordData(packet->category, {data, bytes}, m_opcode,
                        packet->client_swapped, focused, batch))
                    m_failed = true;
            }
            free(error);
            free(reply);
            error = nullptr;
            reply = nullptr;
            if (m_failed)
                break;
        }
        return batch;
    }
private:
    xcb_connection_t *m_connection = nullptr;
    unsigned m_sequence = 0;
    uint8_t m_opcode = 0;
    bool m_failed = false;
};
}
