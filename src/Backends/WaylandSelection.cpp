#include "WaylandSelection.hpp"
#include "wlserver.hpp"
#include "steamcompmgr.hpp"
#include "Utils/Defer.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

static LogScope selection_log( "wayland_selection" );
#define SELECTION_CALLBACK(type, name) []<typename... Args> ( void *pData, Args... args ) { type *pThing = (type *)pData; pThing->name( std::forward<Args>(args)... ); }

namespace gamescope
{
    void CWaylandSelection::BindGlobal( wl_registry *pRegistry, uint32_t uName, const char *pInterface, uint32_t uVersion )
    {
        if ( !strcmp( pInterface, ext_data_control_manager_v1_interface.name ) )
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

        if ( m_pExtDataControlManager )
        {
            m_pExtDataControlDevice = ext_data_control_manager_v1_get_data_device( m_pExtDataControlManager, pSeat );
            if ( m_pExtDataControlDevice )
            {
                ext_data_control_device_v1_add_listener( m_pExtDataControlDevice, &s_ExtDataControlDeviceListener, this );
                m_eSelectionTransport = SelectionTransport::ExtDataControl;
            }
        }

        if ( m_eSelectionTransport == SelectionTransport::Unavailable && m_pWlrDataControlManager )
        {
            m_pWlrDataControlDevice = zwlr_data_control_manager_v1_get_data_device( m_pWlrDataControlManager, pSeat );
            if ( m_pWlrDataControlDevice )
            {
                zwlr_data_control_device_v1_add_listener( m_pWlrDataControlDevice, &s_WlrDataControlDeviceListener, this );
                m_eSelectionTransport = SelectionTransport::WlrDataControl;
            }
        }

        switch ( m_eSelectionTransport )
        {
            case SelectionTransport::ExtDataControl:
                selection_log.infof( "Syncing selections over ext-data-control-v1." );
                break;

            case SelectionTransport::WlrDataControl:
                selection_log.infof( "Syncing selections over wlr-data-control-v1." );
                break;

            case SelectionTransport::Unavailable:
                selection_log.infof( "Host has no data control; nested copies go out over the seat devices and host copies are not synced in." );
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

        if ( bEventLoopAlive && m_eSelectionTransport != SelectionTransport::Unavailable )
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

    bool CWaylandSelection::SetSelection( std::shared_ptr<std::string> szContents, GamescopeSelection eSelection, bool bKeyboardEntered )
    {
        if ( m_eSelectionTransport == SelectionTransport::Unavailable )
            return false;

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
        wlserver_lock();
        DiscardSelectionRead( eSelection );
        wlserver_unlock( false );

        if ( !bHoldForFocus )
            PublishSelection( eSelection );

        // The nested client's take already emptied what steamcompmgr serves in
        // its own server; the other servers, and a reclaim once that client
        // exits, serve these bytes.
        gamescope_set_selection_contents( szContents ? *szContents : std::string{}, wayland_selection::k_szUtf8MimeType, eSelection );
        return true;
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

    // Host-to-session

    template <typename Protocol>
    void CWaylandSelection::OnHostSelection( GamescopeSelection eSelection, typename Protocol::OfferProxy *pOffer )
    {
        SelectionState &selection = m_Selections[eSelection];
        SelectionOffer *pSelectionOffer = pOffer ? Protocol::OfferData( pOffer ) : nullptr;
        defer( DestroyHostOffer<Protocol>( pOffer ) );

        {
            std::scoped_lock lock( selection.mutex );
            // A live source of ours is the host's selection: the request
            // cannot be refused, and a host copy cancels the source before the
            // offer for that copy arrives. So this offer is our own coming
            // back, or was queued before the host saw our request; either way
            // the host holds ours.
            if ( selection.pOwnedSource )
                return;

            // Newest wins: a PRIMARY change the session made while the host
            // focus was elsewhere is older than what the host has now.
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

        QueueSelectionRead( eSelection, pMimeType, [&]( int nWriteFd ) { Protocol::ReceiveOffer( pOffer, pMimeType, nWriteFd ); } );
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

        m_eSelectionTransport = SelectionTransport::Unavailable;
        selection_log.infof( "Host withdrew data control; nested copies go out over the seat devices from now on." );
    }

    void CWaylandSelection::QueueSelectionRead( GamescopeSelection eSelection, const char *pszMimeType, const std::function<void( int nWriteFd )> &fnReceive )
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

    void CWaylandSelection::ClearSelection( GamescopeSelection eSelection )
    {
        wlserver_lock();
        ClearSelectionLocked( eSelection );
        wlserver_unlock( false );
    }

    void CWaylandSelection::ClearSelectionLocked( GamescopeSelection eSelection )
    {
        DiscardSelectionRead( eSelection );
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
                pBackend->FinishSelectionRead( pRead );

                // wlroots closes the pipe with nothing in it when the host
                // selection changed between its event and our receive.
                if ( !sData.empty() )
                    gamescope_set_selection_locked( std::move( sData ), std::move( sMimeType ), eSelection );

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

            case SelectionTransport::Unavailable:
                break;
        }
    }

    template <typename Protocol>
    void CWaylandSelection::PublishSelectionFor( GamescopeSelection eSelection )
    {
        if ( !Protocol::Available( this ) )
            return;

        typename Protocol::Source *pSource = Protocol::CreateSource( this );
        Protocol::AddListener( pSource, this );
        for ( const char *pMimeType : wayland_selection::k_SupportedMimeTypes )
            Protocol::Offer( pSource, pMimeType );

        SelectionState &selection = m_Selections[eSelection];
        void *pReplaced = nullptr;
        {
            // Held across the request, so that an offer the main thread
            // dispatches after it finds the record.
            std::scoped_lock lock( selection.mutex );
            // Data control takes no serial and cannot refuse, so the new
            // source is already the selection and the one it replaced is ours
            // to destroy.
            Protocol::Publish( this, pSource, eSelection );
            pReplaced = std::exchange( selection.pOwnedSource, pSource );
        }
        DestroySelectionSource<Protocol>( pReplaced );

        // The host connection is only dispatched while we are painting, so a
        // copy made with nothing on screen would otherwise sit unsent.
        wl_display_flush( m_pDisplay );
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
