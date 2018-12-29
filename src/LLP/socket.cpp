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
#include <LLP/include/address.h>
#include <LLP/templates/socket.h>

#include <Util/include/debug.h>

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include <sys/ioctl.h>

namespace LLP
{

    /* Constructor for Address */
    Socket::Socket(Service addrConnect)
    : nError(0)
    , nLastSend(runtime::timestamp())
    , nLastRecv(runtime::timestamp())
    {
        fd = -1;
        events = POLLIN | POLLOUT;

        Attempt(addrConnect);
    }


    /* Returns the error of socket if any */
    int Socket::ErrorCode()
    {
        if (nError == WSAEWOULDBLOCK || nError == WSAEMSGSIZE || nError == WSAEINTR || nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


    /* Connects the socket to an external address */
    bool Socket::Attempt(Service addrDest, int nTimeout)
    {
        /* Create the Socket Object (Streaming TCP/IP). */
        if(addrDest.IsIPv4())
            fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        else
            fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

        /* Catch failure if socket couldn't be initialized. */
        if (fd == INVALID_SOCKET)
            return false;

        /* Set the socket to non blocking. */
        fcntl(fd, F_SETFL, O_NONBLOCK);

        /* Open the socket connection for IPv4 / IPv6. */
        bool fConnected = false;
        if(addrDest.IsIPv4())
        {
            /* Set the socket address from the Service. */
            struct sockaddr_in sockaddr;
            addrDest.GetSockAddr(&sockaddr);

            /* Copy in the new address. */
            addr = Address(sockaddr);

            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }
        else
        {
            /* Set the socket address from the Service. */
            struct sockaddr_in6 sockaddr;
            addrDest.GetSockAddr6(&sockaddr);

            /* Copy in the new address. */
            addr = Address(sockaddr);

            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
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
                FD_SET(fd, &fdset);
                int nRet = select(fd + 1, nullptr, &fdset, nullptr, &timeout);

                /* If the connection attempt timed out with select. */
                if (nRet == 0)
                {
                    debug::log(0, "***** Node Connection Timeout %s...", addrDest.ToString().c_str());

                    close(fd);

                    return false;
                }

                /* If the select failed. */
                if (nRet == SOCKET_ERROR)
                {
                    debug::log(0, "***** Node Select Failed %s (%i)", addrDest.ToString().c_str(), GetLastError());

                    close(fd);

                    return false;
                }

                /* Get socket options. TODO: Remove preprocessors for cross platform sockets. */
                socklen_t nRetSize = sizeof(nRet);
    #ifdef WIN32
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
    #else
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
    #endif
                {
                    debug::log(0, "***** Node Get Options Failed %s (%i)", addrDest.ToString().c_str(), GetLastError());
                    close(fd);

                    return false;
                }

                /* If there are no socket options set. TODO: Remove preprocessors for cross platform sockets. */
                if (nRet != 0)
                {
                    debug::log(0, "***** Node Failed after Select %s (%i)", addrDest.ToString().c_str(), nRet);
                    close(fd);

                    return false;
                }
            }
    #ifdef WIN32
            else if (GetLastError() != WSAEISCONN)
    #else
            else
    #endif
            {
                debug::log(0, "***** Node Connect Failed %s (%i)", addrDest.ToString().c_str(), GetLastError());
                close(fd);

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
            ioctlsocket(fd, FIONREAD, &nAvailable)
        #else
            ioctl(fd, FIONREAD, &nAvailable);
        #endif

        return nAvailable;
    }


    /* Clear resources associated with socket and return to invalid state. */
    void Socket::Close()
    {
        close(fd);
        fd = INVALID_SOCKET;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        int nRead = recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT);
        if (nRead < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Read Failed %s (%i %s)", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }
        if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }

    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<int8_t> &vchData, size_t nBytes)
    {
        int nRead = recv(fd, (int8_t*)&vchData[0], nBytes, MSG_DONTWAIT);
        if (nRead < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Read Failed %s (%i %s)", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }
        if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int Socket::Write(std::vector<uint8_t> vData, size_t nBytes)
    {
        /* Check overflow buffer. */
        if(vBuffer.size() > 0)
        {
            nLastSend = runtime::timestamp();
            vBuffer.insert(vBuffer.end(), vData.begin(), vData.end());

            /* Flush the remaining bytes from the buffer. */
            Flush();

            return nBytes;
        }

        /* If there were any errors, handle them gracefully. */
        int nSent = send(fd, (int8_t*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
        if(nSent < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Write Failed %s (%i %s)", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent != vData.size())
        {
            nLastSend = runtime::timestamp();
            vBuffer.insert(vBuffer.end(), vData.begin() + nSent, vData.end());
        }

        return nSent;
    }


    /* Flushes data out of the overflow buffer */
    int Socket::Flush()
    {
        uint32_t nBytes = std::min((uint32_t)vBuffer.size(), 65535u);

        /* If there were any errors, handle them gracefully. */
        int nSent = send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
        if(nSent < 0)
        {
            nError = GetLastError();
            debug::log(2, "xxxxx Node Write Failed %s (%i %s)", addr.ToString().c_str(), nError, strerror(nError));

            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent > 0)
        {
            nLastSend = runtime::timestamp();
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + nSent);
        }

        return nSent;
    }
}
