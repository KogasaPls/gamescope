#pragma once

#include "gamescope_shared.h"
#include "wayland_selection_helpers.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <wayland-client.h>
#include <ext-data-control-v1-client-protocol.h>
#include <wlr-data-control-unstable-v1-client-protocol.h>

struct wl_event_source;

namespace gamescope
{
    // Owns the host selection protocols and transfers. Host callbacks run on
    // the backend thread; fd callbacks run under wlserver_lock(). Source locks
    // are never held while taking wlserver_lock(). The backend calls Shutdown
    // at its existing destruction phase, accounting for an event loop that
    // has already been destroyed.
    class CWaylandSelection
    {
    public:
        void BindGlobal( wl_registry *pRegistry, uint32_t uName, const char *pInterface, uint32_t uVersion );
        void Init( wl_display *pDisplay, wl_seat *pSeat );
        void Shutdown();
        void PublishPendingSelections();
        // Returns false when no data control device carries selections, which
        // leaves the copy to the caller.
        bool SetSelection( std::shared_ptr<std::string> szContents, GamescopeSelection eSelection, bool bKeyboardEntered );

    private:
        wl_display *m_pDisplay = nullptr;
        // Upper bound on a single incoming selection transfer.
        static constexpr size_t k_uMaxSelectionSize = 4 * 1024 * 1024;

        // Which data control protocol carries selections. A data control
        // device takes no serial, is not gated on keyboard focus and sends the
        // current selection at bind, so it carries both directions whenever
        // either side changes. X11's None is a macro here, hence Unavailable.
        enum class SelectionTransport
        {
            Unavailable,
            ExtDataControl,
            WlrDataControl,
        };
        SelectionTransport m_eSelectionTransport = SelectionTransport::Unavailable;

        // Selections over ext-data-control-v1: one device carries both
        // selections in both directions.
        void Wayland_ExtDataControlDevice_DataOffer( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer );
        void Wayland_ExtDataControlDevice_Selection( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer );
        void Wayland_ExtDataControlDevice_Finished( ext_data_control_device_v1 *pDevice );
        void Wayland_ExtDataControlDevice_PrimarySelection( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer );
        static const ext_data_control_device_v1_listener s_ExtDataControlDeviceListener;

        static void Wayland_ExtDataControlOffer_Offer( void *pData, ext_data_control_offer_v1 *pOffer, const char *pMime );
        static const ext_data_control_offer_v1_listener s_ExtDataControlOfferListener;

        void Wayland_ExtDataControlSource_Send( ext_data_control_source_v1 *pSource, const char *pMime, int nFd );
        void Wayland_ExtDataControlSource_Cancelled( ext_data_control_source_v1 *pSource );
        static const ext_data_control_source_v1_listener s_ExtDataControlSourceListener;

        // The same again over wlr-data-control-v1, for a host that has only that one.
        void Wayland_WlrDataControlDevice_DataOffer( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer );
        void Wayland_WlrDataControlDevice_Selection( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer );
        void Wayland_WlrDataControlDevice_Finished( zwlr_data_control_device_v1 *pDevice );
        void Wayland_WlrDataControlDevice_PrimarySelection( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer );
        static const zwlr_data_control_device_v1_listener s_WlrDataControlDeviceListener;

        static void Wayland_WlrDataControlOffer_Offer( void *pData, zwlr_data_control_offer_v1 *pOffer, const char *pMime );
        static const zwlr_data_control_offer_v1_listener s_WlrDataControlOfferListener;

        void Wayland_WlrDataControlSource_Send( zwlr_data_control_source_v1 *pSource, const char *pMime, int nFd );
        void Wayland_WlrDataControlSource_Cancelled( zwlr_data_control_source_v1 *pSource );
        static const zwlr_data_control_source_v1_listener s_WlrDataControlSourceListener;

        // A selection or primary_selection event: discards the offer while a
        // source of ours is the host's selection, else reads it into the
        // session at once, or empties the session's selection for a null offer
        // or one carrying nothing we read. Takes the offer.
        template <typename Protocol>
        void OnHostSelection( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer );
        template <typename Protocol>
        void DestroyHostOffer( typename Protocol::OfferProxy *pOffer );
        // The host withdrew data control: drops both sources, empties the
        // session's selections and stops carrying them.
        template <typename Protocol>
        void OnDataControlFinished();

        // Creates a pipe, lets fnReceive() request pszMimeType into it, and
        // watches the read end on wlserver's event loop; the bytes are
        // committed when the host closes its end.
        void QueueSelectionRead( GamescopeSelection eSelection, const char *pszMimeType, const std::function<void( int nWriteFd )> &fnReceive );
        // The host has nothing we can carry for eSelection: drops any read in
        // flight, then commits an empty selection.
        void ClearSelection( GamescopeSelection eSelection );
        void ClearSelectionLocked( GamescopeSelection eSelection );

        // Offers the contents of eSelection to the host over whichever
        // protocol carries selections.
        void PublishSelection( GamescopeSelection eSelection );
        template <typename Protocol>
        void PublishSelectionFor( GamescopeSelection eSelection );

        // Drains one readable incoming selection pipe. Runs on the wlserver thread.
        static int OnSelectionReadable( int nFd, uint32_t uMask, void *pData );
        // Serves one writable outbound selection pipe. Runs on the wlserver thread.
        static int OnSelectionWritable( int nFd, uint32_t uMask, void *pData );

        // Takes ownership of nFd and serves the current contents of eSelection
        // over it; every type we offer is served the same bytes.
        void WriteSelectionContents( GamescopeSelection eSelection, int nFd );
        // Serves a send event for whichever selection pSource owns. Both
        // selections share a source type, so the owner is found by identity
        // rather than by the callback that delivered it.
        void SendSelectionSource( void *pSource, int nFd );
        void DisownSelectionSource( void *pSource );
        template <typename Protocol>
        void DestroySelectionSource( void *pSource );

        ext_data_control_manager_v1 *m_pExtDataControlManager = nullptr;
        ext_data_control_device_v1 *m_pExtDataControlDevice = nullptr;

        zwlr_data_control_manager_v1 *m_pWlrDataControlManager = nullptr;
        zwlr_data_control_device_v1 *m_pWlrDataControlDevice = nullptr;

        // Listener data for a host offer, owned by the offer proxy until it
        // resolves into a selection or a destroy. Main thread only.
        struct SelectionOffer
        {
            std::vector<std::string> mimeTypes;
        };

        // One host protocol's selection objects. Publish() hands the host a
        // source as eSelection.
        struct ExtDataControlProtocol
        {
            using Source = ext_data_control_source_v1;
            using OfferProxy = ext_data_control_offer_v1;
            static bool Available( CWaylandSelection *pBackend ) { return pBackend->m_pExtDataControlDevice != nullptr; }
            static Source *CreateSource( CWaylandSelection *pBackend ) { return ext_data_control_manager_v1_create_data_source( pBackend->m_pExtDataControlManager ); }
            static void AddListener( Source *pSource, CWaylandSelection *pBackend ) { ext_data_control_source_v1_add_listener( pSource, &pBackend->s_ExtDataControlSourceListener, pBackend ); }
            static void Offer( Source *pSource, const char *pMime ) { ext_data_control_source_v1_offer( pSource, pMime ); }
            static void Publish( CWaylandSelection *pBackend, Source *pSource, GamescopeSelection eSelection )
            {
                if ( eSelection == GAMESCOPE_SELECTION_PRIMARY )
                    ext_data_control_device_v1_set_primary_selection( pBackend->m_pExtDataControlDevice, pSource );
                else
                    ext_data_control_device_v1_set_selection( pBackend->m_pExtDataControlDevice, pSource );
            }
            static void DestroySource( Source *pSource ) { ext_data_control_source_v1_destroy( pSource ); }
            static SelectionOffer *OfferData( OfferProxy *pOffer ) { return (SelectionOffer *)ext_data_control_offer_v1_get_user_data( pOffer ); }
            static void ReceiveOffer( OfferProxy *pOffer, const char *pMime, int nFd ) { ext_data_control_offer_v1_receive( pOffer, pMime, nFd ); }
            static void DestroyOffer( OfferProxy *pOffer ) { ext_data_control_offer_v1_destroy( pOffer ); }
        };

        struct WlrDataControlProtocol
        {
            using Source = zwlr_data_control_source_v1;
            using OfferProxy = zwlr_data_control_offer_v1;
            static bool Available( CWaylandSelection *pBackend ) { return pBackend->m_pWlrDataControlDevice != nullptr; }
            static Source *CreateSource( CWaylandSelection *pBackend ) { return zwlr_data_control_manager_v1_create_data_source( pBackend->m_pWlrDataControlManager ); }
            static void AddListener( Source *pSource, CWaylandSelection *pBackend ) { zwlr_data_control_source_v1_add_listener( pSource, &pBackend->s_WlrDataControlSourceListener, pBackend ); }
            static void Offer( Source *pSource, const char *pMime ) { zwlr_data_control_source_v1_offer( pSource, pMime ); }
            static void Publish( CWaylandSelection *pBackend, Source *pSource, GamescopeSelection eSelection )
            {
                if ( eSelection == GAMESCOPE_SELECTION_PRIMARY )
                    zwlr_data_control_device_v1_set_primary_selection( pBackend->m_pWlrDataControlDevice, pSource );
                else
                    zwlr_data_control_device_v1_set_selection( pBackend->m_pWlrDataControlDevice, pSource );
            }
            static void DestroySource( Source *pSource ) { zwlr_data_control_source_v1_destroy( pSource ); }
            static SelectionOffer *OfferData( OfferProxy *pOffer ) { return (SelectionOffer *)zwlr_data_control_offer_v1_get_user_data( pOffer ); }
            static void ReceiveOffer( OfferProxy *pOffer, const char *pMime, int nFd ) { zwlr_data_control_offer_v1_receive( pOffer, pMime, nFd ); }
            static void DestroyOffer( OfferProxy *pOffer ) { zwlr_data_control_offer_v1_destroy( pOffer ); }
        };

        // Per-selection state, indexed by GamescopeSelection. The source handle
        // is stored as void *; m_eSelectionTransport says which protocol it is.
        struct SelectionState
        {
            // Protects everything below: SetSelection and the publish path
            // write on their caller's thread, the host's events on the main
            // thread. Never held across a wlserver_lock().
            std::mutex mutex;
            std::shared_ptr<std::string> pContents;
            // A PRIMARY change made while the host focus is elsewhere waits
            // here, so the send callback keeps serving what the host last saw
            // until Wayland_Keyboard_Enter lifts the gate. A host selection
            // arriving first supersedes it.
            std::shared_ptr<std::string> pPendingContents;
            // The source the host holds, if ours. The request cannot be
            // refused and cancelled reports every loss, so this is the whole
            // of our ownership state.
            void *pOwnedSource = nullptr;
        };
        SelectionState m_Selections[ GAMESCOPE_SELECTION_COUNT ];

        // Both transfer directions live on wlserver's wl_event_loop, which the
        // wlserver thread dispatches under wlserver_lock(); everything below is
        // touched only with that lock held, so it needs no locking of its own.
        struct SelectionRead
        {
            CWaylandSelection *pBackend = nullptr;
            wl_event_source *pSource = nullptr;
            int nFd = -1;
            std::string sData;
            GamescopeSelection eSelection = GAMESCOPE_SELECTION_CLIPBOARD;
            std::string sMimeType;
        };
        // One per selection at most: a newer host offer supersedes the read of
        // the one before it.
        std::unique_ptr<SelectionRead> m_SelectionReads[ GAMESCOPE_SELECTION_COUNT ];
        void FinishSelectionRead( SelectionRead *pRead );
        void DiscardSelectionRead( GamescopeSelection eSelection );

        static constexpr size_t k_uMaxPendingSelectionWrites = 16;
        static constexpr uint64_t k_ulSelectionWriteDeadlineNanos = 30ul * 1000000000ul;

        // A pipe write-end handed to us by a host reader, with the contents it asked for.
        struct SelectionWrite
        {
            CWaylandSelection *pBackend = nullptr;
            wl_event_source *pSource = nullptr;
            int nFd = -1;
            std::shared_ptr<const std::string> pContents;
            size_t uOffset = 0;
            uint64_t ulStartNanos = 0;
        };
        std::vector<std::unique_ptr<SelectionWrite>> m_SelectionWrites;

    };
}
