#include "WaylandSelection.hpp"
#include "wlserver.hpp"
#include "steamcompmgr.hpp"
#include "Utils/Defer.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <random>
#include <unistd.h>
#include <utility>

static LogScope selection_log( "wayland_selection" );
#define SELECTION_NULL() []<typename... Args> ( void *pData, Args... args ) { }
#define SELECTION_CALLBACK(type, name) []<typename... Args> ( void *pData, Args... args ) { type *pThing = (type *)pData; pThing->name( std::forward<Args>(args)... ); }

namespace gamescope
{
    static uint64_t SelectionMarkerNonce()
    {
        std::random_device device;
        return ( uint64_t( device() ) << 32 ) | device();
    }

    CWaylandSelection::CWaylandSelection()
        : m_sSelectionMarkerPrefix{ wayland_selection::SelectionSourceMarkerPrefix( SelectionMarkerNonce() ) }
    {
    }

    void CWaylandSelection::BindGlobal( wl_registry *pRegistry, uint32_t uName, const char *pInterface, uint32_t uVersion )
    {
        if ( !strcmp( pInterface, wl_data_device_manager_interface.name ) )
        {
            m_pDataDeviceManager = (wl_data_device_manager *)wl_registry_bind( pRegistry, uName, &wl_data_device_manager_interface, 3u );
        }
        else if ( !strcmp( pInterface, zwp_primary_selection_device_manager_v1_interface.name ) )
        {
            m_pPrimarySelectionDeviceManager = (zwp_primary_selection_device_manager_v1 *)wl_registry_bind( pRegistry, uName, &zwp_primary_selection_device_manager_v1_interface, 1u );
        }
        else if ( !strcmp( pInterface, ext_data_control_manager_v1_interface.name ) )
        {
            m_pExtDataControlManager = (ext_data_control_manager_v1 *)wl_registry_bind( pRegistry, uName, &ext_data_control_manager_v1_interface, 1u );
        }
        else if ( !strcmp( pInterface, zwlr_data_control_manager_v1_interface.name ) && uVersion >= 2u )
        {
            // Version 2 is the one that carries the primary selection.
            m_pWlrDataControlManager = (zwlr_data_control_manager_v1 *)wl_registry_bind( pRegistry, uName, &zwlr_data_control_manager_v1_interface, 2u );
        }
    }

    void CWaylandSelection::Init( wl_display *pDisplay, wl_seat *pSeat )
    {
        m_pDisplay = pDisplay;
        m_pSeat = pSeat;
        // The seat data device stays bound whatever carries selections: drag
        // offers arrive on it, and destroying one cancels the host's drag.
        if ( m_pDataDeviceManager && !m_pDataDevice )
        {
            m_pDataDevice = wl_data_device_manager_get_data_device( m_pDataDeviceManager, m_pSeat );
            if ( m_pDataDevice )
            {
                wl_data_device_add_listener( m_pDataDevice, &s_DataDeviceListener, this );
            }
            else
            {
                selection_log.errorf( "Failed to get wl_data_device; clipboard sync disabled." );
            }
        }

        if ( m_pExtDataControlManager )
        {
            m_pExtDataControlDevice = ext_data_control_manager_v1_get_data_device( m_pExtDataControlManager, m_pSeat );
            if ( m_pExtDataControlDevice )
            {
                ext_data_control_device_v1_add_listener( m_pExtDataControlDevice, &s_ExtDataControlDeviceListener, this );
                m_eSelectionTransport = SelectionTransport::ExtDataControl;
            }
        }

        if ( m_eSelectionTransport == SelectionTransport::Unavailable && m_pWlrDataControlManager )
        {
            m_pWlrDataControlDevice = zwlr_data_control_manager_v1_get_data_device( m_pWlrDataControlManager, m_pSeat );
            if ( m_pWlrDataControlDevice )
            {
                zwlr_data_control_device_v1_add_listener( m_pWlrDataControlDevice, &s_WlrDataControlDeviceListener, this );
                m_eSelectionTransport = SelectionTransport::WlrDataControl;
            }
        }

        if ( m_eSelectionTransport == SelectionTransport::Unavailable && m_pDataDevice )
        {
            m_eSelectionTransport = SelectionTransport::Seat;
            BindSeatPrimarySelection();
        }

        switch ( m_eSelectionTransport )
        {
            case SelectionTransport::ExtDataControl:
                selection_log.infof( "Syncing selections over ext-data-control-v1." );
                break;

            case SelectionTransport::WlrDataControl:
                selection_log.infof( "Syncing selections over wlr-data-control-v1." );
                break;

            case SelectionTransport::Seat:
                selection_log.infof( "Syncing selections over the seat data device, as far as its serial check allows." );
                break;

            case SelectionTransport::Unavailable:
                selection_log.infof( "No host selection protocol; clipboard sync disabled." );
                break;
        }

    }

    void CWaylandSelection::Shutdown()
    {
        // wlserver_run() destroys the display, and with it the event loop, then
        // nulls wlserver.display under the lock; the sources are already gone by
        // then and removing one would touch freed memory. The fds are ours
        // either way.
        wlserver_lock();
        const bool bEventLoopAlive = wlserver.display != nullptr;

        if ( bEventLoopAlive )
        {
            for ( uint32_t uSelection = 0; uSelection < GAMESCOPE_SELECTION_COUNT; uSelection++ )
                ClearSelectionLocked( GamescopeSelection( uSelection ) );
        }

        for ( std::unique_ptr<SelectionRead> &pRead : m_SelectionReads )
        {
            if ( !pRead )
                continue;

            if ( bEventLoopAlive && pRead->pSource )
                wl_event_source_remove( pRead->pSource );
            pRead->pSource = nullptr;
            if ( pRead->nFd >= 0 )
                close( pRead->nFd );
            pRead->nFd = -1;
            pRead.reset();
        }

        for ( std::unique_ptr<SelectionWrite> &pWrite : m_SelectionWrites )
        {
            if ( bEventLoopAlive && pWrite->pSource )
                wl_event_source_remove( pWrite->pSource );
            pWrite->pSource = nullptr;
            if ( pWrite->nFd >= 0 )
                close( pWrite->nFd );
            pWrite->nFd = -1;
        }
        m_SelectionWrites.clear();

        wlserver_unlock( bEventLoopAlive );
    }

    const wl_data_device_listener CWaylandSelection::s_DataDeviceListener =
    {
        .data_offer = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataDevice_DataOffer ),
        .enter      = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataDevice_Enter ),
        .leave      = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataDevice_Leave ),
        .motion     = SELECTION_NULL(),
        .drop       = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataDevice_Drop ),
        .selection  = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataDevice_Selection ),
    };
    const wl_data_offer_listener CWaylandSelection::s_DataOfferListener =
    {
        .offer          = Wayland_DataOffer_Offer,
        .source_actions = SELECTION_NULL(),
        .action         = SELECTION_NULL(),
    };
    const wl_data_source_listener CWaylandSelection::s_DataSourceListener =
    {
        .target             = SELECTION_NULL(),
        .send               = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataSource_Send ),
        .cancelled          = SELECTION_CALLBACK( CWaylandSelection, Wayland_DataSource_Cancelled ),
        .dnd_drop_performed = SELECTION_NULL(),
        .dnd_finished       = SELECTION_NULL(),
        .action             = SELECTION_NULL(),
    };
    const zwp_primary_selection_device_v1_listener CWaylandSelection::s_PrimarySelectionDeviceListener =
    {
        .data_offer = SELECTION_CALLBACK( CWaylandSelection, Wayland_PrimarySelectionDevice_DataOffer ),
        .selection  = SELECTION_CALLBACK( CWaylandSelection, Wayland_PrimarySelectionDevice_Selection ),
    };
    const zwp_primary_selection_offer_v1_listener CWaylandSelection::s_PrimarySelectionOfferListener =
    {
        .offer = Wayland_PrimarySelectionOffer_Offer,
    };
    const zwp_primary_selection_source_v1_listener CWaylandSelection::s_PrimarySelectionSourceListener =
    {
        .send      = SELECTION_CALLBACK( CWaylandSelection, Wayland_PrimarySelectionSource_Send ),
        .cancelled = SELECTION_CALLBACK( CWaylandSelection, Wayland_PrimarySelectionSource_Cancelled ),
    };
    const ext_data_control_device_v1_listener CWaylandSelection::s_ExtDataControlDeviceListener =
    {
        .data_offer        = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlDevice_DataOffer ),
        .selection         = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlDevice_Selection ),
        .finished          = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlDevice_Finished ),
        .primary_selection = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlDevice_PrimarySelection ),
    };
    const ext_data_control_offer_v1_listener CWaylandSelection::s_ExtDataControlOfferListener =
    {
        .offer = Wayland_ExtDataControlOffer_Offer,
    };
    const ext_data_control_source_v1_listener CWaylandSelection::s_ExtDataControlSourceListener =
    {
        .send      = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlSource_Send ),
        .cancelled = SELECTION_CALLBACK( CWaylandSelection, Wayland_ExtDataControlSource_Cancelled ),
    };
    const zwlr_data_control_device_v1_listener CWaylandSelection::s_WlrDataControlDeviceListener =
    {
        .data_offer        = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlDevice_DataOffer ),
        .selection         = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlDevice_Selection ),
        .finished          = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlDevice_Finished ),
        .primary_selection = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlDevice_PrimarySelection ),
    };
    const zwlr_data_control_offer_v1_listener CWaylandSelection::s_WlrDataControlOfferListener =
    {
        .offer = Wayland_WlrDataControlOffer_Offer,
    };
    const zwlr_data_control_source_v1_listener CWaylandSelection::s_WlrDataControlSourceListener =
    {
        .send      = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlSource_Send ),
        .cancelled = SELECTION_CALLBACK( CWaylandSelection, Wayland_WlrDataControlSource_Cancelled ),
    };
    const wl_callback_listener CWaylandSelection::s_SelectionSyncListener =
    {
        .done = Wayland_SelectionSync_Done,
    };

    void CWaylandSelection::SetSelection( std::shared_ptr<std::string> szContents, GamescopeSelection eSelection, bool bKeyboardEntered )
    {
        // A game that rewrites PRIMARY as the user drags a mouse would otherwise
        // stomp the host's middle-click buffer from the background.
        const bool bHoldForFocus = eSelection == GAMESCOPE_SELECTION_PRIMARY && !bKeyboardEntered;

        SelectionState &selection = m_Selections[eSelection];
        {
            std::scoped_lock lock( selection.mutex );
            if ( bHoldForFocus )
            {
                selection.pPendingContents = szContents;
            }
            else
            {
                selection.pContents = szContents;
                selection.pPendingContents = nullptr;
            }
        }

        // A host transfer still running would land on top of this newer copy.
        ForgetHostDelivery( eSelection );

        if ( !bHoldForFocus )
            PublishSelection( eSelection );

        // The nested client's take already emptied what steamcompmgr serves in
        // its own server; the other servers, and a reclaim once that client
        // exits, serve these bytes.
        gamescope_set_selection_contents( szContents ? *szContents : std::string{}, wayland_selection::k_szUtf8MimeType, eSelection );
    }

    // Selections

    // A pasted secret should not sit in freed heap. Growing to the capacity
    // first is what reaches a short string that has been moved out of: the move
    // copies the bytes into the new string and leaves them behind, and only
    // [0, size()) is ours to write through data().
    static void ScrubString( std::string &sData )
    {
        sData.resize( sData.capacity(), '\0' );
        explicit_bzero( sData.data(), sData.size() );
        sData.clear();
    }

    // std::string::append would free the old block with the plaintext still in
    // it, so grow by hand and scrub what we are leaving behind.
    static void AppendScrubbingGrowth( std::string &sData, const char *pBytes, size_t uLength )
    {
        if ( sData.size() + uLength > sData.capacity() )
        {
            std::string sGrown;
            sGrown.reserve( std::max( sData.capacity() * 2, sData.size() + uLength ) );
            sGrown.assign( sData );
            ScrubString( sData );
            sData = std::move( sGrown );
        }

        sData.append( pBytes, uLength );
    }

    void CWaylandSelection::BindSeatPrimarySelection()
    {
        if ( !m_pPrimarySelectionDeviceManager || m_pPrimarySelectionDevice )
            return;

        m_pPrimarySelectionDevice = zwp_primary_selection_device_manager_v1_get_device( m_pPrimarySelectionDeviceManager, m_pSeat );
        if ( m_pPrimarySelectionDevice )
            zwp_primary_selection_device_v1_add_listener( m_pPrimarySelectionDevice, &s_PrimarySelectionDeviceListener, this );
    }

    // Host-to-session

    template <typename Protocol>
    void CWaylandSelection::OnHostSelection( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer )
    {
        SelectionState &selection = m_Selections[eSelection];
        SelectionOffer *pSelectionOffer = pOffer ? Protocol::OfferData( pOffer ) : nullptr;

        enum class Disposition { Commit, Discard, Retain };
        Disposition eDisposition = Disposition::Commit;
        std::vector<void *> unheld;
        std::optional<void *> oSuperseded;
        {
            std::scoped_lock lock( selection.mutex );
            if constexpr ( !Protocol::kHostMayRefuse )
            {
                // A live data control source of ours is the host's selection:
                // the request cannot be refused, and a host copy cancels the
                // source before the offer for that copy arrives. So this offer
                // is our own coming back, or was queued before the host saw our
                // request; either way the host holds ours.
                if ( selection.pOwnedSource )
                    eDisposition = Disposition::Discard;
            }
            else
            {
                const std::optional<uint32_t> oSourceId = pSelectionOffer
                    ? wayland_selection::SelectionSourceMarkerId( m_sSelectionMarkerPrefix, pSelectionOffer->mimeTypes )
                    : std::nullopt;
                if ( oSourceId )
                {
                    // One of ours coming back, naming which the host holds.
                    selection.seatSources.OnEcho( *oSourceId, unheld );
                    eDisposition = Disposition::Discard;
                }
                else if ( selection.seatSources.OnForeign( unheld ) )
                {
                    // A request of ours is in flight and this may have been
                    // queued before the host saw it; OnSeatPublishSynced decides.
                    oSuperseded = std::exchange( selection.oRetainedOffer, std::optional<void *>( pOffer ) );
                    eDisposition = Disposition::Retain;
                }
            }
        }

        // The host holds none of these, and will never cancel them.
        DestroySelectionSources<Protocol>( unheld );

        switch ( eDisposition )
        {
            case Disposition::Discard:
                DestroyHostOffer<Protocol>( pOffer );
                return;

            case Disposition::Retain:
                if ( oSuperseded )
                    DestroyHostOffer<Protocol>( (typename Protocol::OfferProxy *)*oSuperseded );
                return;

            case Disposition::Commit:
                break;
        }

        CommitHostOffer<Protocol>( eSelection, pOffer );
    }

    template <typename Protocol>
    void CWaylandSelection::CommitHostOffer( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer )
    {
        SelectionState &selection = m_Selections[eSelection];
        SelectionOffer *pSelectionOffer = pOffer ? Protocol::OfferData( pOffer ) : nullptr;
        defer( DestroyHostOffer<Protocol>( pOffer ) );

        // Newest wins: a PRIMARY change the session made while the host focus
        // was elsewhere is older than what the host has now.
        {
            std::scoped_lock lock( selection.mutex );
            selection.pPendingContents = nullptr;
        }

        const char *pMimeType = pSelectionOffer ? wayland_selection::FirstSupportedMimeType( wayland_selection::k_SupportedMimeTypes, pSelectionOffer->mimeTypes ) : nullptr;
        if ( !pMimeType )
        {
            // A host selection carrying nothing we can read, or none at all,
            // empties the session's.
            ClearSelection( eSelection );
            return;
        }

        QueueSelectionRead( eSelection, pMimeType, Protocol::kHostMayRefuse, [&]( int nWriteFd ) { Protocol::ReceiveOffer( pOffer, pMimeType, nWriteFd ); } );
    }

    template <typename Protocol>
    void CWaylandSelection::DestroyHostOffer( typename Protocol::OfferProxy *pOffer )
    {
        if ( !pOffer )
            return;

        SelectionOffer *pSelectionOffer = Protocol::OfferData( pOffer );
        Protocol::DestroyOffer( pOffer );
        delete pSelectionOffer;
    }

    template <typename Protocol>
    void CWaylandSelection::OnSeatPublishSynced( GamescopeSelection eSelection, void *pSource )
    {
        SelectionState &selection = m_Selections[eSelection];
        std::optional<void *> oRefused;
        std::optional<void *> oRetained;
        bool bHeld = false;
        {
            std::scoped_lock lock( selection.mutex );
            oRefused = selection.seatSources.OnSyncDone( pSource );
            // A later publish still in flight gets to decide instead.
            if ( !selection.seatSources.SyncOutstanding() )
            {
                oRetained = std::exchange( selection.oRetainedOffer, std::nullopt );
                bHeld = selection.seatSources.Held().has_value();
            }
        }

        if ( oRefused )
            DestroySelectionSource<Protocol>( *oRefused );

        if ( !oRetained )
            return;

        // Queued before the host saw our request: stale while ours stands,
        // and the host's selection when it was refused.
        typename Protocol::OfferProxy *pOffer = (typename Protocol::OfferProxy *)*oRetained;
        if ( bHeld )
            DestroyHostOffer<Protocol>( pOffer );
        else
            CommitHostOffer<Protocol>( eSelection, pOffer );
    }

    void CWaylandSelection::Wayland_SelectionSync_Done( void *pData, wl_callback *pCallback, uint32_t uData )
    {
        std::unique_ptr<SelectionSync> pSync( (SelectionSync *)pData );
        wl_callback_destroy( pCallback );
        ( pSync->pBackend->*pSync->pfnSynced )( pSync->eSelection, pSync->pSource );
    }

    void CWaylandSelection::OnSeatSourceCancelled( GamescopeSelection eSelection, void *pSource )
    {
        std::scoped_lock lock( m_Selections[eSelection].mutex );
        m_Selections[eSelection].seatSources.OnCancelled( pSource );
    }

    template <typename Protocol>
    void CWaylandSelection::OnDataControlFinished()
    {
        for ( SelectionState &selection : m_Selections )
        {
            void *pOwned = nullptr;
            {
                std::scoped_lock lock( selection.mutex );
                pOwned = std::exchange( selection.pOwnedSource, nullptr );
            }
            DestroySelectionSource<Protocol>( pOwned );
        }

        for ( uint32_t uSelection = 0; uSelection < GAMESCOPE_SELECTION_COUNT; uSelection++ )
            ClearSelection( GamescopeSelection( uSelection ) );

        if ( m_pDataDevice )
        {
            m_eSelectionTransport = SelectionTransport::Seat;
            BindSeatPrimarySelection();
            selection_log.infof( "Host withdrew data control; falling back to the seat device." );
        }
        else
        {
            m_eSelectionTransport = SelectionTransport::Unavailable;
            selection_log.infof( "Host withdrew data control; selection sync is off." );
        }
    }

    void CWaylandSelection::QueueSelectionRead( GamescopeSelection eSelection, const char *pszMimeType, bool bSkipRepeat, const std::function<void( int nWriteFd )> &fnReceive )
    {
        int nFds[2];
        if ( pipe2( nFds, O_CLOEXEC ) < 0 )
        {
            selection_log.errorf( "Failed to create pipe for selection data." );
            return;
        }

        // Only our read end goes non-blocking: SCM_RIGHTS hands the host the
        // same open file description as our write end, so pipe2( O_NONBLOCK )
        // would make the sending client's writes fail with EAGAIN too.
        const int nFlags = fcntl( nFds[0], F_GETFL, 0 );
        if ( nFlags < 0 || fcntl( nFds[0], F_SETFL, nFlags | O_NONBLOCK ) < 0 )
        {
            selection_log.errorf( "Failed to make a selection pipe non-blocking; dropping the transfer." );
            close( nFds[0] );
            close( nFds[1] );
            return;
        }

        // Close our write end so the read end reaches EOF once the source finishes.
        fnReceive( nFds[1] );
        close( nFds[1] );
        wl_display_flush( m_pDisplay );

        wlserver_lock();

        DiscardSelectionRead( eSelection );

        auto pRead = std::make_unique<SelectionRead>();
        pRead->pBackend = this;
        pRead->eSelection = eSelection;
        pRead->sMimeType = pszMimeType;
        pRead->bSkipRepeat = bSkipRepeat;
        pRead->nFd = nFds[0];
        pRead->pSource = wl_event_loop_add_fd( wlserver.event_loop, nFds[0], WL_EVENT_READABLE, OnSelectionReadable, pRead.get() );
        if ( !pRead->pSource )
        {
            selection_log.errorf( "Failed to watch an incoming selection pipe; dropping the transfer." );
            close( nFds[0] );
            wlserver_unlock( false );
            return;
        }

        m_SelectionReads[eSelection] = std::move( pRead );

        wlserver_unlock( false );
    }

    void CWaylandSelection::FinishSelectionRead( SelectionRead *pRead )
    {
        assert( m_SelectionReads[pRead->eSelection].get() == pRead );

        if ( pRead->pSource )
            wl_event_source_remove( pRead->pSource );
        pRead->pSource = nullptr;

        if ( pRead->nFd >= 0 )
            close( pRead->nFd );
        pRead->nFd = -1;

        ScrubString( pRead->sData );

        m_SelectionReads[pRead->eSelection].reset();
    }

    void CWaylandSelection::DiscardSelectionRead( GamescopeSelection eSelection )
    {
        if ( SelectionRead *pRead = m_SelectionReads[eSelection].get() )
            FinishSelectionRead( pRead );
    }

    void CWaylandSelection::ForgetHostDelivery( GamescopeSelection eSelection )
    {
        wlserver_lock();
        DiscardSelectionRead( eSelection );
        ReplaceHostDelivery( eSelection, std::string{}, std::string{} );
        wlserver_unlock( false );
    }

    void CWaylandSelection::ReplaceHostDelivery( GamescopeSelection eSelection, const std::string &sData, const std::string &sMimeType )
    {
        HostDelivery &delivery = m_LastHostDelivery[eSelection];
        ScrubString( delivery.sData );
        delivery.sData = sData;
        delivery.sMimeType = sMimeType;
    }

    void CWaylandSelection::ClearSelection( GamescopeSelection eSelection )
    {
        wlserver_lock();
        ClearSelectionLocked( eSelection );
        wlserver_unlock( false );
    }

    void CWaylandSelection::ClearSelectionLocked( GamescopeSelection eSelection )
    {
        DiscardSelectionRead( eSelection );
        ReplaceHostDelivery( eSelection, std::string{}, std::string{} );
        gamescope_set_selection_locked( std::string{}, std::string{}, eSelection );
    }

    // Runs on the wlserver thread with the wlserver lock held, so it makes no
    // Wayland client calls; only the pipe and the committed selection.
    int CWaylandSelection::OnSelectionReadable( int nFd, uint32_t uMask, void *pData )
    {
        SelectionRead *pRead = (SelectionRead *)pData;
        CWaylandSelection *pBackend = pRead->pBackend;

        if ( uMask & ( WL_EVENT_READABLE | WL_EVENT_HANGUP ) )
        {
            char chBuf[ 64 * 1024 ];
            const ssize_t nRead = read( nFd, chBuf, sizeof( chBuf ) );
            if ( nRead > 0 )
            {
                if ( pRead->sData.size() + size_t( nRead ) > k_uMaxSelectionSize )
                {
                    selection_log.errorf( "Incoming selection exceeds %zu bytes; dropping.", k_uMaxSelectionSize );
                    pBackend->FinishSelectionRead( pRead );
                }
                else
                {
                    AppendScrubbingGrowth( pRead->sData, chBuf, size_t( nRead ) );
                }

                explicit_bzero( chBuf, size_t( nRead ) );
                return 0;
            }

            if ( nRead == 0 )
            {
                std::string sData = std::move( pRead->sData );
                std::string sMimeType = std::move( pRead->sMimeType );
                const GamescopeSelection eSelection = pRead->eSelection;
                const bool bSkipRepeat = pRead->bSkipRepeat;
                pBackend->FinishSelectionRead( pRead );

                // wlroots closes the pipe with nothing in it when the host
                // selection changed between its event and our receive.
                const HostDelivery &last = pBackend->m_LastHostDelivery[eSelection];
                const bool bRepeat = bSkipRepeat && sData == last.sData && sMimeType == last.sMimeType;
                if ( !sData.empty() && !bRepeat )
                {
                    pBackend->ReplaceHostDelivery( eSelection, sData, sMimeType );
                    gamescope_set_selection_locked( std::move( sData ), std::move( sMimeType ), eSelection );
                }

                // A short string moves by copy, so the plaintext is still here.
                ScrubString( sData );
                return 0;
            }

            if ( errno == EAGAIN || errno == EINTR )
                return 0;

            pBackend->FinishSelectionRead( pRead );
            return 0;
        }

        if ( uMask & WL_EVENT_ERROR )
            pBackend->FinishSelectionRead( pRead );

        return 0;
    }

    // Session-to-host

    void CWaylandSelection::PublishSelection( GamescopeSelection eSelection )
    {
        switch ( m_eSelectionTransport )
        {
            case SelectionTransport::ExtDataControl:
                PublishSelectionFor<ExtDataControlProtocol>( eSelection );
                break;

            case SelectionTransport::WlrDataControl:
                PublishSelectionFor<WlrDataControlProtocol>( eSelection );
                break;

            case SelectionTransport::Seat:
                if ( eSelection == GAMESCOPE_SELECTION_PRIMARY )
                    PublishSelectionFor<SeatPrimaryProtocol>( eSelection );
                else
                    PublishSelectionFor<SeatClipboardProtocol>( eSelection );
                break;

            case SelectionTransport::Unavailable:
                break;
        }
    }

    template <typename Protocol>
    void CWaylandSelection::PublishSelectionFor( GamescopeSelection eSelection )
    {
        if ( !Protocol::Available( this ) )
            return;

        const uint32_t uSourceId = m_uNextSelectionSourceId++;
        const std::string sMarker = wayland_selection::SelectionSourceMarker( m_sSelectionMarkerPrefix, uSourceId );

        typename Protocol::Source *pSource = Protocol::CreateSource( this );
        Protocol::AddListener( pSource, this );
        for ( const char *pMimeType : wayland_selection::k_SupportedMimeTypes )
            Protocol::Offer( pSource, pMimeType );
        Protocol::Offer( pSource, sMarker.c_str() );

        SelectionState &selection = m_Selections[eSelection];
        void *pReplaced = nullptr;
        {
            // Held across the request, so that an offer the main thread
            // dispatches after it finds the record.
            std::scoped_lock lock( selection.mutex );
            if constexpr ( Protocol::kHostMayRefuse )
            {
                // The seat may drop the request without a word, and destroying
                // a source the host still holds clears its selection. The sync
                // behind the request comes back after whatever it caused, so by
                // then an echo has named this source or the host refused it,
                // and a replaced source has been cancelled if it was held.
                selection.seatSources.Publish( uSourceId, pSource );
                Protocol::Publish( this, pSource, eSelection );
                wl_callback *pSync = wl_display_sync( m_pDisplay );
                wl_callback_add_listener( pSync, &s_SelectionSyncListener,
                    new SelectionSync{ this, eSelection, pSource, &CWaylandSelection::OnSeatPublishSynced<Protocol> } );
            }
            else
            {
                // Data control takes no serial and cannot refuse, so the new
                // source is already the selection and the one it replaced is
                // ours to destroy.
                Protocol::Publish( this, pSource, eSelection );
                pReplaced = std::exchange( selection.pOwnedSource, pSource );
            }
        }
        DestroySelectionSource<Protocol>( pReplaced );

        // The host connection is only dispatched while we are painting, so a
        // copy made with nothing on screen would otherwise sit unsent.
        wl_display_flush( m_pDisplay );
    }

    void CWaylandSelection::NoteInputSerial( uint32_t uSerial )
    {
        uint32_t uLatest = m_uLatestInputSerial.load();
        while ( ( uLatest == 0 || wayland_selection::SerialIsNewer( uSerial, uLatest ) ) &&
            !m_uLatestInputSerial.compare_exchange_weak( uLatest, uSerial ) )
        {
        }
    }

    void CWaylandSelection::PublishPendingSelections()
    {
        for ( uint32_t uSelection = 0; uSelection < GAMESCOPE_SELECTION_COUNT; uSelection++ )
        {
            SelectionState &selection = m_Selections[uSelection];
            {
                std::scoped_lock lock( selection.mutex );
                if ( !selection.pPendingContents )
                    continue;

                selection.pContents = std::move( selection.pPendingContents );
            }

            PublishSelection( GamescopeSelection( uSelection ) );
        }
    }

    void CWaylandSelection::WriteSelectionContents( GamescopeSelection eSelection, int nFd )
    {
        std::shared_ptr<const std::string> pContents;
        {
            std::scoped_lock lock( m_Selections[eSelection].mutex );
            pContents = m_Selections[eSelection].pContents;
        }

        // Closing the write end is what tells the host reader the transfer is over.
        if ( !pContents || pContents->empty() )
        {
            close( nFd );
            return;
        }

        const int nFlags = fcntl( nFd, F_GETFL, 0 );
        if ( nFlags < 0 || fcntl( nFd, F_SETFL, nFlags | O_NONBLOCK ) < 0 )
        {
            selection_log.errorf( "Failed to make a selection pipe non-blocking; dropping the transfer." );
            close( nFd );
            return;
        }

        wlserver_lock();

        const uint64_t ulNow = get_time_in_nanos();
        std::erase_if( m_SelectionWrites, [ ulNow ]( const std::unique_ptr<SelectionWrite> &pEntry )
        {
            if ( ulNow < pEntry->ulStartNanos + k_ulSelectionWriteDeadlineNanos )
                return false;

            selection_log.debugf( "Dropping an outbound selection transfer the host never drained." );

            if ( pEntry->pSource )
                wl_event_source_remove( pEntry->pSource );
            if ( pEntry->nFd >= 0 )
                close( pEntry->nFd );
            return true;
        } );

        if ( m_SelectionWrites.size() >= k_uMaxPendingSelectionWrites )
        {
            selection_log.errorf( "More than %zu outbound selection transfers in flight; dropping one.", k_uMaxPendingSelectionWrites );
            wlserver_unlock( false );
            close( nFd );
            return;
        }

        auto pWrite = std::make_unique<SelectionWrite>();
        pWrite->pBackend = this;
        pWrite->nFd = nFd;
        pWrite->pContents = std::move( pContents );
        pWrite->ulStartNanos = ulNow;
        pWrite->pSource = wl_event_loop_add_fd( wlserver.event_loop, nFd, WL_EVENT_WRITABLE, OnSelectionWritable, pWrite.get() );
        if ( !pWrite->pSource )
        {
            selection_log.errorf( "Failed to watch an outbound selection pipe; dropping the transfer." );
            wlserver_unlock( false );
            close( nFd );
            return;
        }

        m_SelectionWrites.push_back( std::move( pWrite ) );
        wlserver_unlock( false );
    }

    void CWaylandSelection::SendSelectionSource( void *pSource, int nFd )
    {
        for ( uint32_t uSelection = 0; uSelection < GAMESCOPE_SELECTION_COUNT; uSelection++ )
        {
            bool bOwner = false;
            {
                std::scoped_lock lock( m_Selections[uSelection].mutex );
                bOwner = m_Selections[uSelection].pOwnedSource == pSource;
            }

            if ( bOwner )
            {
                WriteSelectionContents( GamescopeSelection( uSelection ), nFd );
                return;
            }
        }

        close( nFd );
    }

    void CWaylandSelection::DisownSelectionSource( void *pSource )
    {
        for ( SelectionState &selection : m_Selections )
        {
            std::scoped_lock lock( selection.mutex );
            if ( selection.pOwnedSource == pSource )
                selection.pOwnedSource = nullptr;
        }
    }

    template <typename Protocol>
    void CWaylandSelection::DestroySelectionSource( void *pSource )
    {
        if ( !pSource )
            return;

        Protocol::DestroySource( (typename Protocol::Source *)pSource );
    }

    template <typename Protocol>
    void CWaylandSelection::DestroySelectionSources( const std::vector<void *> &sources )
    {
        for ( void *pSource : sources )
            DestroySelectionSource<Protocol>( pSource );
    }

    // Runs on the wlserver thread with the wlserver lock held.
    int CWaylandSelection::OnSelectionWritable( int nFd, uint32_t uMask, void *pData )
    {
        SelectionWrite *pWrite = (SelectionWrite *)pData;
        CWaylandSelection *pBackend = pWrite->pBackend;

        auto fnFinish = [ pBackend, pWrite ]()
        {
            wl_event_source_remove( pWrite->pSource );
            pWrite->pSource = nullptr;
            close( pWrite->nFd );
            pWrite->nFd = -1;
            std::erase_if( pBackend->m_SelectionWrites,
                [ pWrite ]( const std::unique_ptr<SelectionWrite> &pEntry ) { return pEntry.get() == pWrite; } );
        };

        if ( uMask & ( WL_EVENT_HANGUP | WL_EVENT_ERROR ) )
        {
            fnFinish();
            return 0;
        }

        const size_t uRemaining = pWrite->pContents->size() - pWrite->uOffset;
        const ssize_t nWritten = write( nFd, pWrite->pContents->data() + pWrite->uOffset, uRemaining );
        if ( nWritten < 0 )
        {
            if ( errno == EINTR || errno == EAGAIN )
                return 0;

            fnFinish();
            return 0;
        }

        pWrite->uOffset += size_t( nWritten );
        if ( pWrite->uOffset >= pWrite->pContents->size() )
            fnFinish();

        return 0;
    }

    // Seat data device

    void CWaylandSelection::Wayland_DataDevice_DataOffer( struct wl_data_device *pDevice, struct wl_data_offer *pOffer )
    {
        SelectionOffer *pSelectionOffer = new SelectionOffer{};
        wl_data_offer_add_listener( pOffer, &s_DataOfferListener, pSelectionOffer );
    }

    void CWaylandSelection::Wayland_DataOffer_Offer( void *pData, struct wl_data_offer *pOffer, const char *pMime )
    {
        ( (SelectionOffer *)pData )->mimeTypes.emplace_back( pMime );
    }

    // Drag-and-drop is not accepted, but destroying a v3 drag offer while its
    // source is attached makes wlroots destroy the source (data_offer_destroy
    // in wlr_data_offer.c), cancelling the host's drag. Hold the offer until
    // leave or drop.
    void CWaylandSelection::Wayland_DataDevice_Enter( wl_data_device *pDataDevice, uint32_t uSerial, wl_surface *pSurface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *pOffer )
    {
        // wl_data_device.enter carries a null offer when the drag has no source.
        if ( pOffer )
        {
            delete (SelectionOffer *)wl_data_offer_get_user_data( pOffer );
            wl_data_offer_set_user_data( pOffer, nullptr );
        }

        if ( m_pDragOffer )
            wl_data_offer_destroy( m_pDragOffer );
        m_pDragOffer = pOffer;

        // A null mime type declines every type, which stops the host's drag
        // source showing a copy action over the gamescope window. wlroots then
        // never sends drop, which it only does for an offer that accepted a
        // type; Wayland_DataDevice_Drop is there for hosts that send it anyway.
        if ( pOffer )
            wl_data_offer_accept( pOffer, uSerial, nullptr );
    }

    void CWaylandSelection::Wayland_DataDevice_Leave( wl_data_device *pDataDevice )
    {
        if ( m_pDragOffer )
        {
            delete (SelectionOffer *)wl_data_offer_get_user_data( m_pDragOffer );
            wl_data_offer_destroy( m_pDragOffer );
        }
        m_pDragOffer = nullptr;
    }

    void CWaylandSelection::Wayland_DataDevice_Drop( wl_data_device *pDataDevice )
    {
        Wayland_DataDevice_Leave( pDataDevice );
    }

    void CWaylandSelection::Wayland_DataDevice_Selection( wl_data_device *pDataDevice, wl_data_offer *pOffer )
    {
        // The data control device is authoritative when we have one; the seat
        // only sees a selection while we hold keyboard focus, so following both
        // would race.
        if ( m_eSelectionTransport != SelectionTransport::Seat )
        {
            if ( pOffer )
            {
                delete (SelectionOffer *)wl_data_offer_get_user_data( pOffer );
                wl_data_offer_destroy( pOffer );
            }
            return;
        }

        OnHostSelection<SeatClipboardProtocol>( GAMESCOPE_SELECTION_CLIPBOARD, pOffer );
    }

    // The seat's source types each carry one selection, so a send names its
    // selection by arriving here, whichever of our sources the host still holds.
    void CWaylandSelection::Wayland_DataSource_Send( struct wl_data_source *pSource, const char *pMime, int nFd )
    {
        WriteSelectionContents( GAMESCOPE_SELECTION_CLIPBOARD, nFd );
    }

    void CWaylandSelection::Wayland_DataSource_Cancelled( struct wl_data_source *pSource )
    {
        OnSeatSourceCancelled( GAMESCOPE_SELECTION_CLIPBOARD, pSource );
        wl_data_source_destroy( pSource );
    }

    void CWaylandSelection::Wayland_PrimarySelectionDevice_DataOffer( struct zwp_primary_selection_device_v1 *pDevice, struct zwp_primary_selection_offer_v1 *pOffer )
    {
        SelectionOffer *pSelectionOffer = new SelectionOffer{};
        zwp_primary_selection_offer_v1_add_listener( pOffer, &s_PrimarySelectionOfferListener, pSelectionOffer );
    }

    void CWaylandSelection::Wayland_PrimarySelectionOffer_Offer( void *pData, struct zwp_primary_selection_offer_v1 *pOffer, const char *pMime )
    {
        ( (SelectionOffer *)pData )->mimeTypes.emplace_back( pMime );
    }

    void CWaylandSelection::Wayland_PrimarySelectionDevice_Selection( zwp_primary_selection_device_v1 *pDevice, zwp_primary_selection_offer_v1 *pOffer )
    {
        OnHostSelection<SeatPrimaryProtocol>( GAMESCOPE_SELECTION_PRIMARY, pOffer );
    }

    void CWaylandSelection::Wayland_PrimarySelectionSource_Send( struct zwp_primary_selection_source_v1 *pSource, const char *pMime, int nFd )
    {
        WriteSelectionContents( GAMESCOPE_SELECTION_PRIMARY, nFd );
    }

    void CWaylandSelection::Wayland_PrimarySelectionSource_Cancelled( struct zwp_primary_selection_source_v1 *pSource )
    {
        OnSeatSourceCancelled( GAMESCOPE_SELECTION_PRIMARY, pSource );
        zwp_primary_selection_source_v1_destroy( pSource );
    }

    // Data control

    void CWaylandSelection::Wayland_ExtDataControlDevice_DataOffer( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer )
    {
        SelectionOffer *pSelectionOffer = new SelectionOffer{};
        ext_data_control_offer_v1_add_listener( pOffer, &s_ExtDataControlOfferListener, pSelectionOffer );
    }

    void CWaylandSelection::Wayland_ExtDataControlOffer_Offer( void *pData, ext_data_control_offer_v1 *pOffer, const char *pMime )
    {
        ( (SelectionOffer *)pData )->mimeTypes.emplace_back( pMime );
    }

    void CWaylandSelection::Wayland_ExtDataControlDevice_Selection( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer )
    {
        OnHostSelection<ExtDataControlProtocol>( GAMESCOPE_SELECTION_CLIPBOARD, pOffer );
    }

    void CWaylandSelection::Wayland_ExtDataControlDevice_PrimarySelection( ext_data_control_device_v1 *pDevice, ext_data_control_offer_v1 *pOffer )
    {
        OnHostSelection<ExtDataControlProtocol>( GAMESCOPE_SELECTION_PRIMARY, pOffer );
    }

    void CWaylandSelection::Wayland_ExtDataControlDevice_Finished( ext_data_control_device_v1 *pDevice )
    {
        OnDataControlFinished<ExtDataControlProtocol>();

        ext_data_control_device_v1_destroy( pDevice );
        m_pExtDataControlDevice = nullptr;
    }

    void CWaylandSelection::Wayland_ExtDataControlSource_Send( ext_data_control_source_v1 *pSource, const char *pMime, int nFd )
    {
        SendSelectionSource( pSource, nFd );
    }

    void CWaylandSelection::Wayland_ExtDataControlSource_Cancelled( ext_data_control_source_v1 *pSource )
    {
        DisownSelectionSource( pSource );
        ext_data_control_source_v1_destroy( pSource );
    }

    void CWaylandSelection::Wayland_WlrDataControlDevice_DataOffer( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer )
    {
        SelectionOffer *pSelectionOffer = new SelectionOffer{};
        zwlr_data_control_offer_v1_add_listener( pOffer, &s_WlrDataControlOfferListener, pSelectionOffer );
    }

    void CWaylandSelection::Wayland_WlrDataControlOffer_Offer( void *pData, zwlr_data_control_offer_v1 *pOffer, const char *pMime )
    {
        ( (SelectionOffer *)pData )->mimeTypes.emplace_back( pMime );
    }

    void CWaylandSelection::Wayland_WlrDataControlDevice_Selection( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer )
    {
        OnHostSelection<WlrDataControlProtocol>( GAMESCOPE_SELECTION_CLIPBOARD, pOffer );
    }

    void CWaylandSelection::Wayland_WlrDataControlDevice_PrimarySelection( zwlr_data_control_device_v1 *pDevice, zwlr_data_control_offer_v1 *pOffer )
    {
        OnHostSelection<WlrDataControlProtocol>( GAMESCOPE_SELECTION_PRIMARY, pOffer );
    }

    void CWaylandSelection::Wayland_WlrDataControlDevice_Finished( zwlr_data_control_device_v1 *pDevice )
    {
        OnDataControlFinished<WlrDataControlProtocol>();

        zwlr_data_control_device_v1_destroy( pDevice );
        m_pWlrDataControlDevice = nullptr;
    }

    void CWaylandSelection::Wayland_WlrDataControlSource_Send( zwlr_data_control_source_v1 *pSource, const char *pMime, int nFd )
    {
        SendSelectionSource( pSource, nFd );
    }

    void CWaylandSelection::Wayland_WlrDataControlSource_Cancelled( zwlr_data_control_source_v1 *pSource )
    {
        DisownSelectionSource( pSource );
        zwlr_data_control_source_v1_destroy( pSource );
    }

}
