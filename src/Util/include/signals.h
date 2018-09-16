/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_SIGNALS_H
#define NEXUS_UTIL_INCLUDE_SIGNALS_H

#include <signal.h>
#include <Util/include/args.h>

/* Catch Signal Handler functio */
void HandleSIGTERM(int signum)
{
    if(signum != SIGPIPE)
    {
        printf("Shutting Down %d\n", signum);
        fShutdown = true;
    }
}


/* Setup the signal handlers. */
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
        sigaction(SIGABRT, &sa, NULL);
        sigaction(SIGILL, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGPIPE, &sa, NULL);

    #else
        //catch all signals to flag fShutdown for all threads
        signal(SIGABRT, HandleSIGTERM);
        signal(SIGILL, HandleSIGTERM);
        signal(SIGINT, HandleSIGTERM);
        signal(SIGTERM, HandleSIGTERM);
        signal(SIGPIPE, HandleSIGTERM);

    #ifdef SIGBREAK
        signal(SIGBREAK, HandleSIGTERM);
    #endif
    #endif
}

#endif
