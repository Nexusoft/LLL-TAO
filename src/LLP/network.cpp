/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/network.h>

#include <Util/include/debug.h>

namespace LLP
{

    /* Perform any necessary processing to initialize the underlying network
     * resources such as sockets, etc.
     */
    bool NetworkStartup()
    {

    #ifdef WIN32
        /* Initialize Windows Sockets */
        WSADATA wsaData;
        int32_t ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (ret != NO_ERROR)
        {
            debug::error("TCP/IP socket library failed to start (WSAStartup returned error ", ret, ") ");
            return false;
        }
        else if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
        {
            /* Winsock version incorrect */
            debug::error("Windows sockets does not support requested version 2.2");
            WSACleanup();
            return false;
        }

        debug::log(3, FUNCTION, "Windows sockets initialized for Winsock version 2.2");

    #else
    {
        struct rlimit lim;
        lim.rlim_cur = 4096;
        lim.rlim_max = 4096;
        if(setrlimit(RLIMIT_NOFILE, &lim) == -1)
            debug::error("Failed to set max file descriptors");
    }

    {
        struct rlimit lim;
        getrlimit(RLIMIT_NOFILE, &lim);

        debug::log(0, FUNCTION " File descriptor limit set to ", lim.rlim_cur, " and maximum ", lim.rlim_max);
    }


    #endif

        debug::log(2, FUNCTION, "Network resource initialization complete");

        return true;
    }


    /* Perform any necessary processing to shutdown and release underlying network resources.*/
    bool NetworkShutdown()
    {

    #ifdef WIN32
        /* Clean up Windows Sockets */
        int32_t ret = WSACleanup();

        if (ret != NO_ERROR)
        {
            debug::error("Windows socket cleanup failed (WSACleanup returned error ", ret, ") ");
            return false;
        }

    #endif

        debug::log(2, FUNCTION, "Network resource cleanup complete");

        return true;
    }


    /* Asynchronously invokes the lispers.net API to cache the EIDs and RLOCs used by this node
    *  and caches them for future use */
    void CacheEIDsAndRLOCs()
    {

    }

}
