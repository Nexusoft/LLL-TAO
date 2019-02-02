/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/signals.h>
#include <signal.h>
#include <Util/include/args.h>

std::condition_variable SHUTDOWN;

/** Shutdown the system and all its subsystems. **/
void Shutdown()
{
    config::fShutdown = true;
    SHUTDOWN.notify_all();
}


/** Catch Signal Handler function **/
void HandleSIGTERM(int signum)
{
#ifndef WIN32
    if(signum != SIGPIPE)
        Shutdown();
#else
    Shutdown();
#endif
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
        /* UNIX signal, not applicable to Windows */
        //signal(SIGPIPE, HandleSIGTERM);

    #ifdef SIGBREAK
        signal(SIGBREAK, HandleSIGTERM);
    #endif
    #endif
}
