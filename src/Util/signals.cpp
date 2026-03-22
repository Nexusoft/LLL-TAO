/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/signals.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <signal.h>
#include <atomic>
#include <thread>
#include <cstdlib>


std::condition_variable SHUTDOWN;

/** Shutdown the system and all its subsystems. **/
void Shutdown()
{
    config::fShutdown.store(true);
    SHUTDOWN.notify_all();
}


/** Catch Signal Handler function **/
void HandleSIGTERM(int signum)
{
#ifndef WIN32
    if(signum == SIGPIPE)
        return;  /* SIGPIPE: ignore, broken pipe on closed connection */
#else
    if(signum == SIGINT)
        return;  /* Windows: Ctrl+C handled differently, ignore SIGINT */
#endif

    /* First interrupt: initiate graceful shutdown and arm hard-exit watchdog. */
    if(!config::fShutdown.load())
    {
        Shutdown();  /* sets fShutdown=true, notifies SHUTDOWN cv */

        /* ── Hard-exit watchdog ───────────────────────────────────────────────
         * Spawn a detached background thread. If the process has not exited
         * within HARD_EXIT_TIMEOUT_SECONDS after graceful shutdown was
         * requested, force-terminate via std::_Exit().
         *
         * This handles the "muted thread" scenario where DataThread::join()
         * blocks indefinitely on a connected miner socket that the kernel
         * has not fully cleaned up (e.g. TCP TIME_WAIT), preventing LLP
         * server destructors from completing and the process from exiting.
         *
         * std::_Exit() bypasses all C++ destructors — intentional here,
         * because we are already in an unrecoverable shutdown-stall state.
         * The OS will reclaim all resources on process exit.
         */
        static constexpr int HARD_EXIT_TIMEOUT_SECONDS = 8;
        static constexpr int SLEEP_INTERVAL_MS         = 100;
        static constexpr int MAX_ITERATIONS = (HARD_EXIT_TIMEOUT_SECONDS * 1000) / SLEEP_INTERVAL_MS;

        std::thread([]()
        {
            for(int i = 0; i < MAX_ITERATIONS; ++i)
            {
                runtime::sleep(SLEEP_INTERVAL_MS);
            }
            /* If we reach here, graceful shutdown stalled. Force exit. */
            debug::error("SHUTDOWN WATCHDOG: graceful shutdown did not complete within ",
                         HARD_EXIT_TIMEOUT_SECONDS, "s — forcing process termination "
                         "(DataThread join likely blocked on miner socket cleanup)");
            std::_Exit(1);
        }).detach();

        return;
    }

    /* Second interrupt received while already shutting down: immediate hard exit. */
    debug::error("Second interrupt received during shutdown — forcing immediate exit");
    std::_Exit(1);
}


/** Setup the signal handlers.**/
void SetupSignals()
{
    /* Handle all the signals with signal handler method. */
    #ifndef WIN32
        // Clean shutdown on SIGTERM
        struct sigaction sa;
        sa.sa_handler = HandleSIGTERM;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        //catch all signals to flag fShutdown for all threads
        sigaction(SIGABRT, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGPIPE, &sa, nullptr);

    #else
        //catch all signals to flag fShutdown for all threads
        signal(SIGABRT, HandleSIGTERM);
        signal(SIGILL, HandleSIGTERM);
        signal(SIGINT, HandleSIGTERM);
        signal(SIGTERM, HandleSIGTERM);

    #ifdef SIGBREAK
        signal(SIGBREAK, HandleSIGTERM);
    #endif
    #endif
}
