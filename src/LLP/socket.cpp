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

    /* Constructor for Socket */
    Socket::Socket(NetAddr addrConnect)
    : nError(0)
    , nLastSend(runtime::timestamp())
    , nLastRecv(runtime::timestamp())
    {
        fd = -1;
        events = POLLIN; //consider using POLLOUT

        Attempt(addrConnect);
    }


    /* Returns the error of socket if any */
    int Socket::ErrorCode()
    {
        /* Check for errors with poll. */
        if(revents & POLLERR || revents & POLLHUP || revents & POLLNVAL)
            return -1;

        /* Check for errors from reads or writes. */
        if (nError == WSAEWOULDBLOCK || nError == WSAEMSGSIZE || nError == WSAEINTR || nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


    /* Connects the socket to an external address */
    bool Socket::Attempt(const NetAddr &addrDest, uint32_t nTimeout)
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
            /* Set the socket address from the NetAddr. */
            struct sockaddr_in sockaddr;
            addrDest.GetSockAddr(&sockaddr);

            /* Copy in the new address. */
            addr = NetAddr(sockaddr);

            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }
        else
        {
            /* Set the socket address from the NetAddr. */
            struct sockaddr_in6 sockaddr;
            addrDest.GetSockAddr6(&sockaddr);

            /* Copy in the new address. */
            addr = NetAddr(sockaddr);

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
                    debug::log(0, FUNCTION, "connection timeout ", addrDest.ToString(), "...");

                    close(fd);

                    return false;
                }

                /* If the select failed. */
                if (nRet == SOCKET_ERROR)
                {
                    debug::log(0, FUNCTION, "select failed ", addrDest.ToString(), " (",  GetLastError(), ")");

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
                    debug::log(0, FUNCTION, "get options failed ", addrDest.ToString(), " (", GetLastError(), ")");
                    close(fd);

                    return false;
                }

                /* If there are no socket options set. TODO: Remove preprocessors for cross platform sockets. */
                if (nRet != 0)
                {
                    debug::log(0, FUNCTION, "failed after select ", addrDest.ToString(), " (", nRet, ")");
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
                debug::log(0, FUNCTION, "connect failed ", addrDest.ToString(), " (", GetLastError(), ")");
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
            debug::log(2, FUNCTION, "read failed ", addr.ToString(), " (", nError, " ", strerror(nError), ")");

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
            debug::log(2, FUNCTION, "read failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

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
            debug::log(2, FUNCTION, "write failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

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
        /* Don't flush if buffer doesn't have any data. */
        if(vBuffer.size() == 0)
            return 0;

        /* Set the maximum bytes to flush to 2^16 or maximum socket buffers. */
        uint32_t nBytes = std::min((uint32_t)vBuffer.size(), 65535u);

        /* If there were any errors, handle them gracefully. */
        int nSent = send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
        if(nSent < 0)
        {
            nError = GetLastError();
            debug::log(2, FUNCTION, "flush failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

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
