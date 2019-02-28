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
    : PACKET_MUTEX()
    , DATA_MUTEX()
    , nError(0)
    , nLastSend(0)
    , nLastRecv(0)
    , vBuffer()
    , addr()
    {
        fd = INVALID_SOCKET;
        events = POLLIN;

        /* Reset the internal timers. */
        Reset();
    }


    /** Copy constructor. **/
    Socket::Socket(const Socket& socket)
    : pollfd(socket)
    , PACKET_MUTEX()
    , DATA_MUTEX()
    , nError(socket.nError.load())
    , nLastSend(socket.nLastSend.load())
    , nLastRecv(socket.nLastRecv.load())
    , vBuffer(socket.vBuffer)
    , addr(socket.addr)
    {
    }


    /** The socket constructor. **/
    Socket::Socket(int32_t nSocketIn, const BaseAddress &addrIn)
    : PACKET_MUTEX()
    , DATA_MUTEX()
    , nError(0)
    , nLastSend(0)
    , nLastRecv(0)
    , vBuffer()
    , addr(addrIn)
    {
        fd = nSocketIn;
        events = POLLIN;

        /* Reset the internal timers. */
        Reset();
    }


    /* Constructor for socket */
    Socket::Socket(const BaseAddress &addrConnect)
    : PACKET_MUTEX()
    , DATA_MUTEX()
    , nError(0)
    , nLastSend(0)
    , nLastRecv(0)
    , vBuffer()
    , addr()
    {
        fd = INVALID_SOCKET;
        events = POLLIN;

        /* Reset the internal timers. */
        Reset();

        /* Connect socket to external address. */
        Attempt(addrConnect);
    }


    /* Destructor for socket */
    Socket::~Socket()
    {
    }


    /*  Returns the address of the socket. */
    BaseAddress Socket::GetAddress() const
    {
        LOCK(DATA_MUTEX);

        return addr;
    }


    /*  Resets the internal timers. */
    void Socket::Reset()
    {
        /* Atomic data types, no need for lock */
        nLastRecv = runtime::timestamp();
        nLastSend = runtime::timestamp();
    }


    /* Connects the socket to an external address */
    bool Socket::Attempt(const BaseAddress &addrDest, uint32_t nTimeout)
    {
        bool fConnected = false;
        SOCKET nFile = INVALID_SOCKET;

        /* Create the Socket Object (Streaming TCP/IP). */
        {
            LOCK(DATA_MUTEX);

            if(addrDest.IsIPv4())
                fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            else
                fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

            /* Catch failure if socket couldn't be initialized. */
            if (fd == INVALID_SOCKET)
                return false;

            nFile = fd;
        }

        /* Set the socket to non blocking. */
    #ifdef WIN32
        long unsigned int nonBlocking = 1; //any non-zero is nonblocking
        ioctlsocket(nFile, FIONBIO, &nonBlocking);
    #else
        fcntl(nFile, F_SETFL, O_NONBLOCK);
    #endif

        /* Open the socket connection for IPv4 / IPv6. */
        if(addrDest.IsIPv4())
        {
            /* Set the socket address from the BaseAddress. */
            struct sockaddr_in sockaddr;
            addrDest.GetSockAddr(&sockaddr);

            {
                LOCK(DATA_MUTEX);
                /* Copy in the new address. */
                addr = BaseAddress(sockaddr);
            }

            /* Connect for non-blocking socket should return SOCKET_ERROR
             * (with last error WSAEWOULDBLOCK/Windows, WSAEINPROGRESS/Linux normally).
             * Then we have to use select below to check if connection was made.
             * If it doesn't return that, it means it connected immediately and connection was successful. (very unusual, but possible)
             */
            fConnected = (connect(nFile, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }
        else
        {
            /* Set the socket address from the BaseAddress. */
            struct sockaddr_in6 sockaddr;
            addrDest.GetSockAddr6(&sockaddr);

            {
                LOCK(DATA_MUTEX);
                /* Copy in the new address. */
                addr = BaseAddress(sockaddr);
            }

            fConnected = (connect(nFile, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }

        /* Handle final socket checks if connection established with no errors. */
        if (fConnected)
        {
            /* We would expect to get WSAEWOULDBLOCK/WSAEINPROGRESS here in the normal case of attempting a connection. */
            nError = WSAGetLastError();

            if (nError == WSAEWOULDBLOCK || nError == WSAEALREADY || nError == WSAEINPROGRESS)
            {
                struct timeval timeout;
                timeout.tv_sec  = nTimeout / 1000;
                timeout.tv_usec = (nTimeout % 1000) * 1000;

                /* Create an fd_set with our current socket (and only it) */
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(nFile, &fdset);

                /* select returns the number of descriptors that have successfully established connection and are writeable.
                 * We only pass one descriptor in the fd_set, so this will return 1 if connect attempt succeeded, 0 if it timed out, or SOCKET_ERROR on error
                 */
                int nRet = select(nFile + 1, nullptr, &fdset, nullptr, &timeout);

                /* If the connection attempt timed out with select. */
                if (nRet == 0)
                {
                    debug::log(2, FUNCTION, "Connection timeout ", addrDest.ToString());
                    closesocket(nFile);

                    return false;
                }

                /* If the select failed. */
                else if (nRet == SOCKET_ERROR)
                {
                    debug::log(3, FUNCTION, "Select failed ", addrDest.ToString(), " (",  WSAGetLastError(), ")");
                    closesocket(nFile);

                    return false;
                }

                /* Get socket options. TODO: Remove preprocessors for cross platform sockets. */
                socklen_t nRetSize = sizeof(nRet);
    #ifdef WIN32
                if (getsockopt(nFile, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
    #else
                if (getsockopt(nFile, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
    #endif
                {
                    debug::log(3, FUNCTION, "Get options failed ", addrDest.ToString(), " (", WSAGetLastError(), ")");
                    closesocket(nFile);

                    return false;
                }

                /* If there are no socket options set. */
                if (nRet != 0)
                {
                    debug::log(3, FUNCTION, "Failed after select ", addrDest.ToString(), " (", nRet, ")");

                    closesocket(nFile);

                    return false;
                }
            }
            else if (nError != WSAEISCONN)
            {
                debug::log(3, FUNCTION, "Connect failed ", addrDest.ToString(), " (", nError, ")");

                closesocket(nFile);

                return false;
            }
        }

        /* Reset the internal timers. */
        Reset();

        return true;
    }


    /* Poll the socket to check for available data */
    int Socket::Available() const
    {
        LOCK(DATA_MUTEX);

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
        LOCK(DATA_MUTEX);

        if(fd != INVALID_SOCKET)
            closesocket(fd);

        fd = INVALID_SOCKET;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        int nRead = 0;

    #ifdef WIN32
        nRead = recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT);
    #else
        nRead = recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT);
    #endif

        if (nRead < 0)
        {
            nError = WSAGetLastError();
            debug::log(2, FUNCTION, "read failed ", addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }

    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<int8_t> &vData, size_t nBytes)
    {
        int nRead = 0;

    #ifdef WIN32
        nRead = recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT);
    #else
        nRead = recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT);
    #endif

        if (nRead < 0)
        {
            nError = WSAGetLastError();
            debug::log(2, FUNCTION, "read failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int Socket::Write(const std::vector<uint8_t>& vData, size_t nBytes)
    {
        int nSent = 0;

        {
            LOCK(DATA_MUTEX);

            /* Check overflow buffer. */
            if(vBuffer.size() > 0)
            {
                nLastSend = runtime::timestamp();
                vBuffer.insert(vBuffer.end(), vData.begin(), vData.end());

                return nBytes;
            }
        }

        /* If there were any errors, handle them gracefully. */
        {
            LOCK(PACKET_MUTEX);

            #ifdef WIN32
                nSent = send(fd, (char*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT);
            #else
                nSent = send(fd, (int8_t*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT);
            #endif
        }


        if(nSent < 0)
        {
            nError = WSAGetLastError();
            debug::log(2, FUNCTION, "write failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent != vData.size())
        {
            LOCK(DATA_MUTEX);
            vBuffer.insert(vBuffer.end(), vData.begin() + nSent, vData.end());
        }

        nLastSend = runtime::timestamp();

        return nSent;
    }


    /* Flushes data out of the overflow buffer */
    int Socket::Flush()
    {
        int nSent = 0;


        uint32_t nBytes = 0;
        uint32_t nSize = 0;

        {
            LOCK(DATA_MUTEX);

            /* Create a thread-safe copy of the file descriptor */
            nSize = static_cast<uint32_t>(vBuffer.size());
        }


        /* Don't flush if buffer doesn't have any data. */
        if(nSize == 0)
            return 0;

        /* Set the maximum bytes to flush to 2^16 or maximum socket buffers. */
        nBytes = std::min(nSize, std::min((uint32_t)config::GetArg("-maxsendsize", 65535u), 65535u));

        /* If there were any errors, handle them gracefully. */
        {
            LOCK(PACKET_MUTEX);

            #ifdef WIN32
                nSent = send(fd, (char*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT);
            #else
                nSent = send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT);
            #endif
        }


        /* Handle errors on flush. */
        if(nSent < 0)
        {
            nError = WSAGetLastError();
            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent > 0)
        {
            LOCK(DATA_MUTEX);
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + nSent);
        }

        nLastSend = runtime::timestamp();

        return nSent;
    }


    /*  Determines if nTime seconds have elapsed since last Read / Write. */
    bool Socket::Timeout(uint32_t nTime) const
    {
        return (runtime::timestamp() > (uint64_t)(nLastSend + nTime) &&
                runtime::timestamp() > (uint64_t)(nLastRecv + nTime));
    }


    /*  Checks if is in null state. */
    bool Socket::IsNull() const
    {
        LOCK(DATA_MUTEX);

        return fd == -1;
    }


    /*  Checks for any flags in the Error Handle. */
    bool Socket::Errors() const
    {
        return error_code() != 0;
    }


    /*  Give the message (c-string) of the error in the socket. */
    char *Socket::Error() const
    {
        return strerror(error_code());
    }


    /* Returns the error of socket if any */
    int Socket::error_code() const
    {
        /* Check for errors from reads or writes. */
        if (nError == WSAEWOULDBLOCK ||
            nError == WSAEMSGSIZE ||
            nError == WSAEINTR ||
            nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


}
