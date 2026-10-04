#pragma once
#include <xcb/record.h>
#include <xcb/xcbext.h>
#include <cstdlib>
#include "waitable.h"
#include "xtest_input_helpers.hpp"
#include "HostXTestCapture.hpp"

namespace gamescope
{
// Input-thread owned. Forward only XTEST FakeInput requests. Device events
// are flush markers, never forwarded as physical mouse or keyboard input.
// XI2 RawMotion loses the relative/absolute flag.
class HostXTestInput final : public IWaitable
{
public:
    ~HostXTestInput() { if (m_connection) xcb_disconnect(m_connection); }
    bool Init(const char *display)
    {
        if (!display || !*display)
            return false;
        if (!m_capture.Init(display))
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
        range.core_requests = {XCB_NO_OPERATION, XCB_NO_OPERATION}; // capture boundary, never input
        // RECORD buffers requests until ordinary server output is flushed.
        // Native Wayland focus / a cursor at the X root edge may produce no
        // such output. Device events force critical output pending and a
        // category boundary, including key-only chords without mouse motion.
        range.device_events = {2, 6}; // KeyPress through MotionNotify, discarded below
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
    IWaitable *CaptureWaitable() { return &m_capture; }
    bool SetFocused(bool focused)
    {
        if (!focused || !m_capture.Captured()) m_acceptInput = false;
        return m_capture.SetFocused(focused && !Failed());
    }
    void OnPollHangUp() override { m_failed = true; }
    bool Failed() const { return m_failed || m_capture.Failed() || !m_connection || xcb_connection_has_error(m_connection); }

    // True requests an immediate continuation after the caller services
    // Wayland. Socket readiness alone cannot expose XCB-buffered leftovers.
    bool Dispatch(bool focused, const xtest_input::KeySink &onKey = {},
        const xtest_input::MotionSink &onMotion = {})
    {
        SetFocused(focused);
        if (Failed()) return false;
        bool more = m_capture.Drain();
        focused = focused && m_capture.Captured();
        xtest_input::DispatchBudget budget;
        while (!Failed())
        {
            if (!budget.Take()) { more = true; break; }
            if (!m_packet)
            {
                void *reply = nullptr;
                xcb_generic_error_t *error = nullptr;
                if (!xcb_poll_for_reply(m_connection, m_sequence, &reply, &error))
                    break;
                m_packet.reset(static_cast<xcb_record_enable_context_reply_t *>(reply));
                if (error || !m_packet || m_packet->element_header != 0)
                {
                    free(error);
                    m_failed = true;
                    break;
                }
                m_data = {reinterpret_cast<const uint8_t *>(m_packet.get() + 1), size_t(m_packet->length) * 4};
                if (m_packet->category == 1 && m_capture.IsCaptureBoundary(m_packet->xid_base, m_data))
                    m_acceptInput = focused;
            }
            // Both sinks run in wire order. No paced movement may overtake a
            // modifier transition or survive after its sender stops moving.
            if (!xtest_input::DispatchRecordData(m_packet->category, m_data, m_opcode,
                    m_packet->client_swapped, focused && m_acceptInput, budget, onKey, onMotion))
                m_failed = true;
            if (!m_data.empty()) { more = true; break; }
            m_packet.reset();
        }
        if (Failed()) { SetFocused(false); return false; }
        return more;
    }

private:
    xcb_connection_t *m_connection = nullptr;
    unsigned m_sequence = 0;
    uint8_t m_opcode = 0;
    std::atomic<bool> m_failed = false;
    HostXTestCapture m_capture;
    bool m_acceptInput = false;
    std::unique_ptr<xcb_record_enable_context_reply_t, decltype(&std::free)> m_packet{nullptr, std::free};
    std::span<const uint8_t> m_data;

};
}
