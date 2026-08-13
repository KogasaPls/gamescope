#pragma once

#include "gamescope_shared.h"
#include "wayland_selection_helpers.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <wayland-client.h>
#include <primary-selection-unstable-v1-client-protocol.h>
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
        CWaylandSelection();
        void BindGlobal( wl_registry *pRegistry, uint32_t uName, const char *pInterface, uint32_t uVersion );
        void Init( wl_display *pDisplay, wl_seat *pSeat );
        void Shutdown();
        void NoteInputSerial( uint32_t uSerial );
        void PublishPendingSelections();
        void SetSelection( std::shared_ptr<std::string> szContents, GamescopeSelection eSelection, bool bKeyboardEntered );

    private:
        wl_display *m_pDisplay = nullptr;
        wl_seat *m_pSeat = nullptr;
        std::atomic<uint32_t> m_uLatestInputSerial = { 0 };
        // Upper bound on a single incoming selection transfer.
        static constexpr size_t k_uMaxSelectionSize = 4 * 1024 * 1024;

        // Which host protocol carries selections. A data control device takes
        // no serial, is not gated on keyboard focus and sends the current
        // selection at bind, so it carries both directions whenever either
        // side changes; the seat device shows us the host's selection only
        // while we hold keyboard focus and may drop ours over a serial we
        // cannot check. X11's None is a macro here, hence Unavailable.
        enum class SelectionTransport
        {
            Unavailable,
            ExtDataControl,
            WlrDataControl,
            Seat,
        };
        SelectionTransport m_eSelectionTransport = SelectionTransport::Unavailable;

        // The seat's data device: the host's CLIPBOARD while we hold keyboard
        // focus, and drag offers, which are declined.
        void Wayland_DataDevice_DataOffer( struct wl_data_device *pDevice, struct wl_data_offer *pOffer );
        void Wayland_DataDevice_Selection( wl_data_device *pDataDevice, wl_data_offer *pOffer );
        void Wayland_DataDevice_Enter( wl_data_device *pDataDevice, uint32_t uSerial, wl_surface *pSurface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *pOffer );
        void Wayland_DataDevice_Leave( wl_data_device *pDataDevice );
        void Wayland_DataDevice_Drop( wl_data_device *pDataDevice );
        static const wl_data_device_listener s_DataDeviceListener;

        static void Wayland_DataOffer_Offer( void *pData, struct wl_data_offer *pOffer, const char *pMime );
        static const wl_data_offer_listener s_DataOfferListener;

        void Wayland_DataSource_Send( struct wl_data_source *pSource, const char *pMime, int nFd );
        void Wayland_DataSource_Cancelled( struct wl_data_source *pSource );
        static const wl_data_source_listener s_DataSourceListener;

        // The seat's primary selection device: the same again for PRIMARY
        // (middle-click paste), bound only when the seat carries selections.
        void Wayland_PrimarySelectionDevice_DataOffer( struct zwp_primary_selection_device_v1 *pDevice, struct zwp_primary_selection_offer_v1 *pOffer );
        void Wayland_PrimarySelectionDevice_Selection( zwp_primary_selection_device_v1 *pDevice, zwp_primary_selection_offer_v1 *pOffer );
        static const zwp_primary_selection_device_v1_listener s_PrimarySelectionDeviceListener;

        static void Wayland_PrimarySelectionOffer_Offer( void *pData, struct zwp_primary_selection_offer_v1 *pOffer, const char *pMime );
        static const zwp_primary_selection_offer_v1_listener s_PrimarySelectionOfferListener;

        void Wayland_PrimarySelectionSource_Send( struct zwp_primary_selection_source_v1 *pSource, const char *pMime, int nFd );
        void Wayland_PrimarySelectionSource_Cancelled( struct zwp_primary_selection_source_v1 *pSource );
        static const zwp_primary_selection_source_v1_listener s_PrimarySelectionSourceListener;

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

        // A selection or primary_selection event from any host device: decides
        // whether the offer is the host's selection, our own coming back, or
        // older than a request of ours still in flight.
        template <typename Protocol>
        void OnHostSelection( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer );
        // Reads the offer into the session at once, or empties the session's
        // selection for a null offer or one carrying nothing we read. Takes the offer.
        template <typename Protocol>
        void CommitHostOffer( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer );
        template <typename Protocol>
        void DestroyHostOffer( typename Protocol::OfferProxy *pOffer );
        // The sync behind a seat publish came back: the source stands or was
        // refused, and an offer retained meanwhile is stale or the host's.
        template <typename Protocol>
        void OnSeatPublishSynced( GamescopeSelection eSelection, void *pSource );
        void OnSeatSourceCancelled( GamescopeSelection eSelection, void *pSource );
        // The host withdrew data control: drop both sources and fall back to the seat.
        template <typename Protocol>
        void OnDataControlFinished();

        // Creates a pipe, lets fnReceive() request pszMimeType into it, and
        // watches the read end on wlserver's event loop; the bytes are
        // committed when the host closes its end.
        void QueueSelectionRead( GamescopeSelection eSelection, const char *pszMimeType, bool bSkipRepeat, const std::function<void( int nWriteFd )> &fnReceive );
        // A nested copy supersedes what the host delivered: a read still
        // running and the bytes a seat re-send is compared against.
        void ForgetHostDelivery( GamescopeSelection eSelection );
        void BindSeatPrimarySelection();
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
        // selections share a source type under data control, so the owner is
        // found by identity rather than by the callback that delivered it.
        void SendSelectionSource( void *pSource, int nFd );
        void DisownSelectionSource( void *pSource );
        template <typename Protocol>
        void DestroySelectionSource( void *pSource );
        template <typename Protocol>
        void DestroySelectionSources( const std::vector<void *> &sources );

        wl_data_device_manager *m_pDataDeviceManager = nullptr;
        wl_data_device *m_pDataDevice = nullptr;
        wl_data_offer *m_pDragOffer = nullptr;

        zwp_primary_selection_device_manager_v1 *m_pPrimarySelectionDeviceManager = nullptr;
        zwp_primary_selection_device_v1 *m_pPrimarySelectionDevice = nullptr;

        ext_data_control_manager_v1 *m_pExtDataControlManager = nullptr;
        ext_data_control_device_v1 *m_pExtDataControlDevice = nullptr;

        zwlr_data_control_manager_v1 *m_pWlrDataControlManager = nullptr;
        zwlr_data_control_device_v1 *m_pWlrDataControlDevice = nullptr;

        const std::string m_sSelectionMarkerPrefix;
        std::atomic<uint32_t> m_uNextSelectionSourceId = { 1 };

        // Listener data for a host offer, owned by the offer proxy until it
        // resolves into a selection, a drag, or a destroy. Main thread only.
        struct SelectionOffer
        {
            std::vector<std::string> mimeTypes;
        };

        // The sync behind one seat publish, as its callback's listener data.
        struct SelectionSync
        {
            CWaylandSelection *pBackend;
            GamescopeSelection eSelection;
            void *pSource;
            void ( CWaylandSelection::*pfnSynced )( GamescopeSelection eSelection, void *pSource );
        };
        static void Wayland_SelectionSync_Done( void *pData, wl_callback *pCallback, uint32_t uData );
        static const wl_callback_listener s_SelectionSyncListener;

        // One host protocol's selection objects. Publish() hands the host a
        // source as eSelection; kHostMayRefuse says whether the host can drop
        // that without a word, which decides who destroys the source it
        // replaces.
        struct ExtDataControlProtocol
        {
            using Source = ext_data_control_source_v1;
            using OfferProxy = ext_data_control_offer_v1;
            static constexpr bool kHostMayRefuse = false;
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
            static constexpr bool kHostMayRefuse = false;
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

        // The seat's two devices carry one selection each, so the source type
        // names the selection. The request carries the newest serial the host
        // has given us: it is refused with one older than the serial of the
        // selection it would replace, and a selection set through data control
        // carries a fresh one.
        struct SeatClipboardProtocol
        {
            using Source = wl_data_source;
            using OfferProxy = wl_data_offer;
            static constexpr bool kHostMayRefuse = true;
            static bool Available( CWaylandSelection *pBackend ) { return pBackend->m_pDataDevice != nullptr; }
            static Source *CreateSource( CWaylandSelection *pBackend ) { return wl_data_device_manager_create_data_source( pBackend->m_pDataDeviceManager ); }
            static void AddListener( Source *pSource, CWaylandSelection *pBackend ) { wl_data_source_add_listener( pSource, &pBackend->s_DataSourceListener, pBackend ); }
            static void Offer( Source *pSource, const char *pMime ) { wl_data_source_offer( pSource, pMime ); }
            static void Publish( CWaylandSelection *pBackend, Source *pSource, GamescopeSelection eSelection ) { wl_data_device_set_selection( pBackend->m_pDataDevice, pSource, pBackend->m_uLatestInputSerial.load() ); }
            static void DestroySource( Source *pSource ) { wl_data_source_destroy( pSource ); }
            static SelectionOffer *OfferData( OfferProxy *pOffer ) { return (SelectionOffer *)wl_data_offer_get_user_data( pOffer ); }
            static void ReceiveOffer( OfferProxy *pOffer, const char *pMime, int nFd ) { wl_data_offer_receive( pOffer, pMime, nFd ); }
            static void DestroyOffer( OfferProxy *pOffer ) { wl_data_offer_destroy( pOffer ); }
        };

        struct SeatPrimaryProtocol
        {
            using Source = zwp_primary_selection_source_v1;
            using OfferProxy = zwp_primary_selection_offer_v1;
            static constexpr bool kHostMayRefuse = true;
            static bool Available( CWaylandSelection *pBackend ) { return pBackend->m_pPrimarySelectionDevice != nullptr; }
            static Source *CreateSource( CWaylandSelection *pBackend ) { return zwp_primary_selection_device_manager_v1_create_source( pBackend->m_pPrimarySelectionDeviceManager ); }
            static void AddListener( Source *pSource, CWaylandSelection *pBackend ) { zwp_primary_selection_source_v1_add_listener( pSource, &pBackend->s_PrimarySelectionSourceListener, pBackend ); }
            static void Offer( Source *pSource, const char *pMime ) { zwp_primary_selection_source_v1_offer( pSource, pMime ); }
            static void Publish( CWaylandSelection *pBackend, Source *pSource, GamescopeSelection eSelection ) { zwp_primary_selection_device_v1_set_selection( pBackend->m_pPrimarySelectionDevice, pSource, pBackend->m_uLatestInputSerial.load() ); }
            static void DestroySource( Source *pSource ) { zwp_primary_selection_source_v1_destroy( pSource ); }
            static SelectionOffer *OfferData( OfferProxy *pOffer ) { return (SelectionOffer *)zwp_primary_selection_offer_v1_get_user_data( pOffer ); }
            static void ReceiveOffer( OfferProxy *pOffer, const char *pMime, int nFd ) { zwp_primary_selection_offer_v1_receive( pOffer, pMime, nFd ); }
            static void DestroyOffer( OfferProxy *pOffer ) { zwp_primary_selection_offer_v1_destroy( pOffer ); }
        };

        // Per-selection state, indexed by GamescopeSelection. Source and offer
        // handles are stored as void *; m_eSelectionTransport and the selection
        // say which protocol they are.
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
            // Data control: the source the host holds, if ours. The request
            // cannot be refused and cancelled reports every loss, so this is
            // the whole of our ownership state there.
            void *pOwnedSource = nullptr;
            // Seat: the sources the host may still hold, and which it does.
            wayland_selection::SeatSourceTracker<void *> seatSources;
            // Seat: a foreign offer that arrived while a sync was outstanding,
            // kept until the sync says whether it predates our request.
            // nullptr is a null offer.
            std::optional<void *> oRetainedOffer;
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
            // Not committed when it repeats what the host last delivered: the
            // seat re-sends its selection on every keyboard enter.
            bool bSkipRepeat = false;
        };
        // One per selection at most: a newer host offer supersedes the read of
        // the one before it.
        std::unique_ptr<SelectionRead> m_SelectionReads[ GAMESCOPE_SELECTION_COUNT ];
        void FinishSelectionRead( SelectionRead *pRead );
        void DiscardSelectionRead( GamescopeSelection eSelection );

        // What the host last delivered for each selection, which tells a seat
        // re-send from a new copy: committing the re-send again would take the
        // X selection from a nested client that copied in between. Emptied when
        // a nested copy publishes, so a host copy that repeats earlier bytes
        // after one still wins.
        struct HostDelivery
        {
            std::string sData;
            std::string sMimeType;
        };
        HostDelivery m_LastHostDelivery[ GAMESCOPE_SELECTION_COUNT ];
        void ReplaceHostDelivery( GamescopeSelection eSelection, const std::string &sData, const std::string &sMimeType );

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
