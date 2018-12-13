/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>
#include <vector>
#include <stdio.h>

#include <LLP/include/network.h>
#include <LLP/templates/socket.h>

#include <Util/include/debug.h>

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
        if (nError == WSAEWOULDBLOCK || nError == WSAEMSGSIZE || nError == WSAEINTR || nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


    /* Connects the socket to an external address */
    bool Socket::Connect(CService addrDest, int nTimeout)
    {
        /* Create the Socket Object (Streaming TCP/IP). */
        if(addrDest.IsIPv4())
            nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        else
            nSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

        /* Catch failure if socket couldn't be initialized. */
        if (nSocket == INVALID_SOCKET)
            return false;

        /* Set the socket to non blocking. */
        fcntl(nSocket, F_SETFL, O_NONBLOCK);

        /* Open the socket connection for IPv4 / IPv6. */
        bool fConnected = false;
        if(addrDest.IsIPv4())
        {
            /* Set the socket address from the CService. */
            struct sockaddr_in sockaddr;
            addrDest.GetSockAddr(&sockaddr);

            /* Copy in the new address. */
            addr = CAddress(sockaddr);

            fConnected = (connect(nSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }
        else
        {
            /* Set the socket address from the CService. */
            struct sockaddr_in6 sockaddr;
            addrDest.GetSockAddr6(&sockaddr);

            /* Copy in the new address. */
            addr = CAddress(sockaddr);

            fConnected = (connect(nSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }

        /* Handle final socket checks if connection established with no errors. */
        if (fConnected)
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
                int nRet = select(nSocket + 1, nullptr, &fdset, nullptr, &timeout);

                /* If the connection attempt timed out with select. */
                if (nRet == 0)
                {
                    debug::log(0, "***** Node Connection Timeout %s...\n", addrDest.ToString().c_str());

                    close(nSocket);

                    return false;
                }

                /* If the select failed. */
                if (nRet == SOCKET_ERROR)
                {
                    debug::log(0, "***** Node Select Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());

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
                    debug::log(0, "***** Node Get Options Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());
                    close(nSocket);

                    return false;
                }

                /* If there are no socket options set. TODO: Remove preprocessors for cross platform sockets. */
                if (nRet != 0)
                {
                    debug::log(0, "***** Node Failed after Select %s (%i)\n", addrDest.ToString().c_str(), nRet);
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
                debug::log(0, "***** Node Connect Failed %s (%i)\n", addrDest.ToString().c_str(), GetLastError());
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
    void Socket::Close()
    {
        close(nSocket);
        nSocket = INVALID_SOCKET;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        int8_t pchBuf[nBytes];
        int nRead = recv(nSocket, pchBuf, nBytes, MSG_DONTWAIT);
        if (nRead < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Read Failed %s (%i %s)\n", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }

        if(nRead > 0)
            std::copy(&pchBuf[0], &pchBuf[0] + nRead, vData.begin());

        return nRead;
    }

    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<int8_t> &vchData, size_t nBytes)
    {
        int8_t pchBuf[nBytes];
        int nRead = recv(nSocket, pchBuf, nBytes, MSG_DONTWAIT);
        if (nRead < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Read Failed %s (%i %s)\n", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }

        if(nRead > 0)
            std::copy(&pchBuf[0], &pchBuf[0] + nRead, vchData.begin());

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int Socket::Write(std::vector<uint8_t> vData, size_t nBytes)
    {
        char pchBuf[nBytes];
        std::copy(&vData[0], &vData[0] + nBytes, &pchBuf[0]);

        /* If there were any errors, handle them gracefully. */
        int nSent = send(nSocket, pchBuf, nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
        if(nSent < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Write Failed %s (%i %s)\n", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent != vData.size())
        {
            vData.erase(vData.begin(), vData.begin() + nSent);

            return Write(vData, vData.size());
        }

        return nSent;
    }
}
