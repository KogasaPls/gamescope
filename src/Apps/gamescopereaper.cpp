#include "../Utils/Process.h"
#include "../log.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace gamescope
{
    static LogScope s_ReaperLog( "reaper" );

    // Watches over a PID and waits for all the children to die.
    // It sets itself up as a subreaper so any children get reparented ti oti.
    // If the primary process dies, it kills all the children.
    //
    // Gamescope can have a lot of bad things happen to it, crashes, segfaults, whatever
    // but we always want to make sure that we cleanly kill all of our children when we die.
    // This child process attempts to stay alive as long as it can in order to fulfil Gamescope's
    // dying wish -- to kill all of it's children.
    int GamescopeReaperProcess( int argc, char **argv )
    {
        pthread_setname_np( pthread_self(), "gamescope-reaper" );

        static constexpr struct option k_ReaperOptions[] =
        {
            { "label", required_argument, nullptr, 0 },
            { "new-session-id", no_argument, nullptr, 0 },
            { "respawn", no_argument, nullptr, 0 },
        };

        bool bRespawn = false;
        bool bNewSession = false;

        int nOptIndex = -1;
        int nOption = -1;
        while ( ( nOption = getopt_long(argc, argv, "", k_ReaperOptions, &nOptIndex ) ) != -1 )
        {
            if ( nOption == '?' )
            {
                s_ReaperLog.errorf( "Unknown option." );
            }
            assert( nOption == 0 );

            const char *pszOptionName = k_ReaperOptions[ nOptIndex ].name;
            if ( !strcmp( pszOptionName, "label" ) )
            {
                // Do nothing.
                continue;
            }
            else if ( !strcmp( pszOptionName, "respawn" ) )
            {
                bRespawn = true;
            }
            else if ( !strcmp( pszOptionName, "new-session-id" ) )
            {
                bNewSession = true;
            }
        }

        int nSubCommandArgc = 0;
        for ( int i = 0; i < argc; i++ )
        {
            if ( strcmp( "--", argv[ i ] ) == 0 && i + 1 < argc )
            {
                nSubCommandArgc = i + 1;
                break;
            }
        }

        if ( nSubCommandArgc == 0 )
        {
            s_ReaperLog.errorf( "No sub-command!" );
            return 1;
        }

        // Mirror some of the busy work we do in ProcessPreSpawn,
        // in case someone else wants to use this utility.
        Process::ResetSignals();

        // We typically don't make a new sid, as we want to keep the same stdin/stdout
        // Don't really care about it for pgroup reasons, as processes can leave those.
        if ( bNewSession )
            setsid();

        // Handle child exits and shutdown in the same thread. SpawnProcess
        // restores the child's signal mask before exec.
        sigset_t signals;
        sigemptyset( &signals );
        for ( int sig : { SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGCHLD } )
            sigaddset( &signals, sig );
        if ( sigprocmask( SIG_BLOCK, &signals, nullptr ) < 0 )
        {
            s_ReaperLog.errorf_errno( "Failed to block reaper signals" );
            return 1;
        }

        Process::BecomeSubreaper();
        Process::SetDeathSignal( SIGTERM );

        pid_t nPrimaryChild = Process::SpawnProcess( &argv[ nSubCommandArgc ] );
        if ( nPrimaryChild < 0 )
            return 1;

        bool stopping = false;
        int exitStatus = 0;
        auto stopChildren = [&]()
        {
            if ( stopping )
                return;
            stopping = true;
            // Launch wrappers need not forward signals themselves. This
            // reaper is the sole supervisor that signals its launched tree.
            Process::KillAllChildren( getpid(), SIGTERM );
        };
        for ( ;; )
        {
            int sig;
            const int err = sigwait( &signals, &sig );
            if ( err )
            {
                if ( err == EINTR )
                    continue;
                s_ReaperLog.errorf( "sigwait failed: %s", strerror( err ) );
                return 1;
            }

            if ( sig != SIGCHLD )
            {
                stopChildren();
                continue;
            }

            // SIGCHLD may cover multiple exits. Adopted children belong to
            // this reaper too, and can outlive the primary launcher.
            pid_t child;
            while ( ( child = waitpid( -1, nullptr, WNOHANG ) ) > 0 )
            {
                if ( child != nPrimaryChild )
                    continue;

                nPrimaryChild = -1;
                if ( !bRespawn )
                    stopChildren();
            }
            const bool noChildren = child < 0 && errno == ECHILD;
            if ( !stopping && bRespawn && nPrimaryChild < 0 )
            {
                s_ReaperLog.infof( "\"%s\" process shut down. Restarting.", argv[ nSubCommandArgc ] );
                nPrimaryChild = Process::SpawnProcess( &argv[ nSubCommandArgc ] );
                if ( nPrimaryChild < 0 )
                {
                    stopChildren();
                    exitStatus = 1;
                }
            }
            if ( stopping && noChildren )
                return exitStatus;
        }
    }

}

int main( int argc, char **argv )
{
    return gamescope::GamescopeReaperProcess( argc, argv );
}
