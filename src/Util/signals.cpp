/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/signals.h>
#include <Util/include/args.h>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>

namespace
{
    static_assert(std::atomic<bool>::is_always_lock_free,
                  "signal notification requires a lock-free atomic");
    std::atomic<bool> fSignalReceived{false};

    class SignalMonitor
    {
        std::atomic<bool> fStop{false};
        std::thread thread;

    public:
        SignalMonitor() : thread([this]
        {
            while(!fStop.load())
            {
                if(fSignalReceived.load())
                {
                    if(config::fShutdown.load())
                        std::_Exit(1);

                    /* Arm the watchdog before shutdown notification, which
                     * itself may block on a stalled runtime or thread. */
                    std::thread([]()
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(8));
                        std::_Exit(1);
                    }).detach();
                    Shutdown();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }) {}

        ~SignalMonitor()
        {
            fStop.store(true);
            thread.join();
        }
    };
}

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

    /* Only lock-free atomic operations and _Exit are used in this handler. */
    if(fSignalReceived.exchange(true))
        std::_Exit(1);
}

void StartSignalMonitor()
{
    static SignalMonitor monitor;
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
