/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>
#include <vector>
#include <stdio.h>

#include "include/network.h"
#include "templates/socket.h"

#include "../Util/include/debug.h"

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include <sys/ioctl.h>

namespace LLP
{

    /* Constructor for Address */
    Socket::Socket(CService addrConnect)
    {
        Connect(addrConnect);
    }


    /* Returns the error of socket if any */
    int Socket::Error()
    {
        if(nSocket >= 0)
            return 0;

        return nSocket;
    }


    /* Connects the socket to an external address */
    bool Socket::Connect(CService addrDest, int nTimeout)
    {
        /* Create the Socket Object (Streaming TCP/IP). */
        nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        /* Catch failure if socket couldn't be initialized. */
        if (nSocket == INVALID_SOCKET)
            return false;

        /* Set the socket address from the CService. */
        struct sockaddr_in sockaddr;
        addrDest.GetSockAddr(&sockaddr);

        /* Copy in the new address. */
        addr = CAddress(sockaddr);

        /* Open the socket connection. */
        if (connect(nSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
        {
            // WSAEINVAL is here because some legacy version of winsock uses it
            if (GetLastError() == WSAEINPROGRESS || GetLastError() == WSAEWOULDBLOCK || GetLastError() == WSAEINVAL)
            {
                struct timeval timeout;
                timeout.tv_sec  = nTimeout / 1000;
                timeout.tv_usec = (nTimeout % 1000) * 1000;

                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(nSocket, &fdset);
                int nRet = select(nSocket + 1, NULL, &fdset, NULL, &timeout);

                /* If the connection attempt timed out with select. */
                if (nRet == 0)
                {
                    printf("***** Node Connection Timeout %s...\n", addrDest.ToString().c_str());

                    close(nSocket);

                    return false;
                }

                /* If the select failed. */
                if (nRet == SOCKET_ERROR)
                {
                    printf("***** Node Select Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());

                    close(nSocket);

                    return false;
                }

                /* Get socket options. TODO: Remove preprocessors for cross platform sockets. */
                socklen_t nRetSize = sizeof(nRet);
    #ifdef WIN32
                if (getsockopt(nSocket, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
    #else
                if (getsockopt(nSocket, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
    #endif
                {
                    printf("***** Node Get Options Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());
                    close(nSocket);

                    return false;
                }

                /* If there are no socket options set. TODO: Remove preprocessors for cross platform sockets. */
                if (nRet != 0)
                {
                    printf("***** Node Failed after Select %s (%i)\n", addrDest.ToString().c_str(), nRet);
                    close(nSocket);

                    return false;
                }
            }
    #ifdef WIN32
            else if (GetLastError() != WSAEISCONN)
    #else
            else
    #endif
            {
                printf("***** Node Connect Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());
                close(nSocket);

                return false;
            }
        }

        return true;
    }


    /* Poll the socket to check for available data */
    int Socket::Available()
    {
        int nAvailable = 0;
        #ifdef WIN32
            ioctlsocket(nSocket, FIONREAD, &nAvailable)
        #else
            ioctl(nSocket, FIONREAD, &nAvailable);
        #endif

        return nAvailable;
    }


    /* Clear resources associated with socket and return to invalid state. */
    void Socket::Close(int nError)
    {
        close(nSocket);
        nSocket = nError;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        char pchBuf[nBytes];
        int nRead = recv(nSocket, pchBuf, nBytes, 0);
        if (nRead < 0)
        {
            int nErr = GetLastError();
            if(GetArg("-verbose", 0) > 2)
                printf("***** Node Read Failed %s (%i %s)\n", addr.ToString().c_str(), nErr, strerror(nErr));

            Close(nErr);

            return nErr;
        }

        if(nRead > 0)
            memcpy(&vData[0], pchBuf, nRead);

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int Socket::Write(std::vector<uint8_t> vData, size_t nBytes)
    {
        char pchBuf[nBytes];
        memcpy(pchBuf, &vData[0], nBytes);

        int nSent = send(nSocket, pchBuf, nBytes, 0);

        /* If there were any errors, handle them gracefully. */
        if(nSent < 0)
        {
            int nErr = GetLastError();
            if(GetArg("-verbose", 0) > 2)
                printf("***** Node Write Failed %s (%i %s)\n", addr.ToString().c_str(), nErr, strerror(nErr));

            Close(nErr);

            return nErr;
        }

        return nSent;
    }
}
