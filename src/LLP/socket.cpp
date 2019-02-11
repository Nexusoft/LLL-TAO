/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>
#include <vector>
#include <stdio.h>

#include <LLP/templates/socket.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <sys/ioctl.h>
#endif

namespace LLP
{

    /** The default constructor. **/
    Socket::Socket()
    : nError(0)
    , nLastSend(runtime::timestamp())
    , nLastRecv(runtime::timestamp())
    , vBuffer()
    , addr()
    {
        fd = INVALID_SOCKET;

        events = POLLIN;
    }


    /** The socket constructor. **/
    Socket::Socket(int32_t nSocketIn, const BaseAddress &addrIn)
    : nError(0)
    , nLastSend(runtime::timestamp())
    , nLastRecv(runtime::timestamp())
    , vBuffer()
    , addr(addrIn)
    {
        fd = nSocketIn;

        events = POLLIN;
    }


    /* Constructor for socket */
    Socket::Socket(const BaseAddress &addrConnect)
    : nError(0)
    , nLastSend(runtime::timestamp())
    , nLastRecv(runtime::timestamp())
    , vBuffer()
    , addr()
    {
        fd = INVALID_SOCKET;
        events = POLLIN;

        Attempt(addrConnect);
    }


    /* Destructor for socket */
    Socket::~Socket()
    {
    }


    /* Returns the error of socket if any */
    int Socket::ErrorCode() const
    {
        /* Check for errors from reads or writes. */
        if (nError == WSAEWOULDBLOCK ||
            nError == WSAEMSGSIZE ||
            nError == WSAEINTR ||
            nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


    /* Connects the socket to an external address */
    bool Socket::Attempt(const BaseAddress &addrDest, uint32_t nTimeout)
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
    #ifdef WIN32
        long unsigned int nonBlocking = 1; //any non-zero is nonblocking
        ioctlsocket(fd, FIONBIO, &nonBlocking);
    #else
        fcntl(fd, F_SETFL, O_NONBLOCK);
    #endif

        /* Open the socket connection for IPv4 / IPv6. */
        bool fConnected = false;
        if(addrDest.IsIPv4())
        {
            /* Set the socket address from the BaseAddress. */
            struct sockaddr_in sockaddr;
            addrDest.GetSockAddr(&sockaddr);

            /* Copy in the new address. */
            addr = BaseAddress(sockaddr);

            /* Connect for non-blocking socket should return SOCKET_ERROR (with last error WSAEWOULDBLOCK normally).
             * Then we have to use select below to check if connection was made.
             * If it doesn't return that, it means it connected immediately and connection was successful. (very unusual, but possible)
             */
            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }
        else
        {
            /* Set the socket address from the BaseAddress. */
            struct sockaddr_in6 sockaddr;
            addrDest.GetSockAddr6(&sockaddr);

            /* Copy in the new address. */
            addr = BaseAddress(sockaddr);

            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }

        /* Handle final socket checks if connection established with no errors. */
        if (fConnected)
        {
            /* We would expect to get WSAEWOULDBLOCK here in the normal case of attempting a connection.
             * WSAEINVAL is here because some legacy version of winsock uses it
             */
            nError = WSAGetLastError();

            if (nError == WSAEWOULDBLOCK || nError == WSAEINPROGRESS || nError == WSAEINVAL)
            {
                struct timeval timeout;
                timeout.tv_sec  = nTimeout / 1000;
                timeout.tv_usec = (nTimeout % 1000) * 1000;

                /* Create an fd_set with our current socket (and only it) */
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(fd, &fdset);

                /* select returns the number of descriptors that have successfully established connection and are writeable.
                 * We only pass one descriptor in the fd_set, so this will return 1 if connect attempt succeeded, 0 if it timed out, or SOCKET_ERROR on error
                 */
                int nRet = select(fd + 1, nullptr, &fdset, nullptr, &timeout);

                /* If the connection attempt timed out with select. */
                if (nRet == 0)
                {
                    debug::log(3, FUNCTION, "connection timeout ", addrDest.ToString(), "...");

                    if(fd != INVALID_SOCKET)
                         closesocket(fd);

                    return false;
                }

                /* If the select failed. */
                else if (nRet == SOCKET_ERROR)
                {
                    debug::log(3, FUNCTION, "select failed ", addrDest.ToString(), " (",  WSAGetLastError(), ")");


                    if(fd != INVALID_SOCKET)
                        closesocket(fd);

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
                    debug::log(3, FUNCTION, "get options failed ", addrDest.ToString(), " (", WSAGetLastError(), ")");

                    if(fd != INVALID_SOCKET)
                    {
                        closesocket(fd);
                    }

                    return false;
                }

                /* If there are no socket options set. TODO: Remove preprocessors for cross platform sockets. */
                if (nRet != 0)
                {
                    debug::log(3, FUNCTION, "failed after select ", addrDest.ToString(), " (", nRet, ")");

                    if(fd != INVALID_SOCKET)
                    {
                        closesocket(fd);
                    }

                    return false;
                }
            }
            else if (nError != WSAEISCONN)
            {
                debug::log(3, FUNCTION, "connect failed ", addrDest.ToString(), " (", nError, ")");

                if(fd != INVALID_SOCKET)
                {
                    closesocket(fd);
                }

                return false;
            }
        }

        nLastRecv = runtime::timestamp();
        nLastSend = runtime::timestamp();

        return true;
    }


    /* Poll the socket to check for available data */
    int Socket::Available() const
    {
    #ifdef WIN32
        long unsigned int nAvailable = 0;
        ioctlsocket(fd, FIONREAD, &nAvailable);
    #else
        uint32_t nAvailable = 0;
        ioctl(fd, FIONREAD, &nAvailable);
    #endif

        return nAvailable;
    }


    /* Clear resources associated with socket and return to invalid state. */
    void Socket::Close()
    {

        if(fd != INVALID_SOCKET)
        {
          closesocket(fd);
        }

        fd = INVALID_SOCKET;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
    #ifdef WIN32
        int nRead = recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT);
    #else
        int nRead = recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT);
    #endif

        if (nRead < 0)
        {
            nError = WSAGetLastError();
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
    #ifdef WIN32
        int nRead = recv(fd, (char*)&vchData[0], nBytes, MSG_DONTWAIT);
    #else
        int nRead = recv(fd, (int8_t*)&vchData[0], nBytes, MSG_DONTWAIT);
    #endif
        if (nRead < 0)
        {
            nError = WSAGetLastError();
            debug::log(2, FUNCTION, "read failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }
        if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int Socket::Write(const std::vector<uint8_t>& vData, size_t nBytes)
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
    #ifdef WIN32
        int nSent = send(fd, (char*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
    #else
        int nSent = send(fd, (int8_t*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
    #endif
        if(nSent < 0)
        {
            nError = WSAGetLastError();
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
    #ifdef WIN32
        int nSent = send(fd, (char*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
    #else
        int nSent = send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT );
    #endif

        /* Handle errors on flush. */
        if(nSent < 0)
            return WSAGetLastError();

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent > 0)
        {
            nLastSend = runtime::timestamp();
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + nSent);
        }

        return nSent;
    }
}
