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
    : SOCKET_MUTEX()
    , DATA_MUTEX()
    , nLastSend(0)
    , nLastRecv(0)
    , nError(0)
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
    , SOCKET_MUTEX()
    , DATA_MUTEX()
    , nLastSend(socket.nLastSend.load())
    , nLastRecv(socket.nLastRecv.load())
    , nError(socket.nError.load())
    , vBuffer(socket.vBuffer)
    , addr(socket.addr)
    {
    }


    /** The socket constructor. **/
    Socket::Socket(int32_t nSocketIn, const BaseAddress &addrIn)
    : SOCKET_MUTEX()
    , DATA_MUTEX()
    , nLastSend(0)
    , nLastRecv(0)
    , nError(0)
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
    : SOCKET_MUTEX()
    , DATA_MUTEX()
    , nLastSend(0)
    , nLastRecv(0)
    , nError(0)
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

        /* Create the Socket Object (Streaming TCP/IP). */
        {
            LOCK(DATA_MUTEX);

            if(addrDest.IsIPv4())
                fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            else
                fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

            /* Catch failure if socket couldn't be initialized. */
            if(fd == INVALID_SOCKET)
                return false;
        }

        /* Set the socket to non blocking. */
    #ifdef WIN32
        long unsigned int nonBlocking = 1; //any non-zero is nonblocking
        ioctlsocket(fd, FIONBIO, &nonBlocking);
    #else
        fcntl(fd, F_SETFL, O_NONBLOCK);
    #endif

#ifndef WIN32
        /* Set the MSS to a lower than default value to support the increased bytes required for LISP */
        int nMaxSeg = 1300;
        if(setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &nMaxSeg, sizeof(nMaxSeg)) == SOCKET_ERROR)
        { //TODO: this fails on OSX systems. Need to find out why
            //debug::error("setsockopt() MSS for connection failed: ", WSAGetLastError());
            //closesocket(nFile);

            //return false;
        }
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
            LOCK(SOCKET_MUTEX);
            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
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

            LOCK(SOCKET_MUTEX);
            fConnected = (connect(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR);
        }

        /* Handle final socket checks if connection established with no errors. */
        if(fConnected)
        {
            /* Check for errors. */
            int32_t nError = WSAGetLastError();

            /* We would expect to get WSAEWOULDBLOCK/WSAEINPROGRESS here in the normal case of attempting a connection. */
            if(nError == WSAEWOULDBLOCK || nError == WSAEALREADY || nError == WSAEINPROGRESS)
            {
                /* Setup poll objects. */
                pollfd fds[1];
                fds[0].events = POLLOUT;
                fds[0].fd     = fd;

                /* Get the total sleeps to cycle. */
                uint32_t nIterators = (nTimeout / 100) - 1;

#ifdef WIN32
                int32_t nPoll = WSAPoll(&fds[0], 1, 100);
#else
                int32_t nPoll = poll(&fds[0], 1, 100);
#endif

                for(uint32_t nSeconds = 0; nSeconds < nIterators && !config::fShutdown.load(); ++nSeconds)
                {
#ifdef WIN32
                    nPoll = WSAPoll(&fds[0], 1, 100);
#else
                    nPoll = poll(&fds[0], 1, 100);
#endif

                    /* Check poll for errors. */
                    if(nPoll < 0)
                    {
                        debug::log(3, FUNCTION, "poll failed ", addrDest.ToString(), " (", nError, ")");
                        closesocket(fd);

                        return false;
                    }
                }

                /* Check for timeout. */
                if(nPoll == 0)
                {
                    debug::log(3, FUNCTION, "poll timeout ", addrDest.ToString(), " (", nError, ")");
                    closesocket(fd);

                    return false;
                }
            }
            else if(nError != WSAEISCONN)
            {
                debug::log(3, FUNCTION, "connect failed ", addrDest.ToString(), " (", nError, ")");

                closesocket(fd);

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
        LOCK(SOCKET_MUTEX);

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
        LOCK(SOCKET_MUTEX);

        if(fd != INVALID_SOCKET)
            closesocket(fd);

        fd = INVALID_SOCKET;
    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        LOCK(SOCKET_MUTEX);

        int32_t nRead = 0;

    #ifdef WIN32
        nRead = static_cast<int32_t>(recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT));
    #else
        nRead = static_cast<int32_t>(recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT));
    #endif

        if(nRead < 0)
        {
            nError = WSAGetLastError();
            debug::log(3, FUNCTION, "read failed ", addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }

    /* Read data from the socket buffer non-blocking */
    int32_t Socket::Read(std::vector<int8_t> &vData, size_t nBytes)
    {
        LOCK(SOCKET_MUTEX);

        int32_t nRead = 0;

    #ifdef WIN32
        nRead = static_cast<int32_t>(recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT));
    #else
        nRead = static_cast<int32_t>(recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT));
    #endif

        if(nRead < 0)
        {
            nError = WSAGetLastError();
            debug::log(3, FUNCTION, "read failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");

            return nError;
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp();

        return nRead;
    }


    /* Write data into the socket buffer non-blocking */
    int32_t Socket::Write(const std::vector<uint8_t>& vData, size_t nBytes)
    {
        int32_t nSent = 0;

        {
            LOCK(DATA_MUTEX);

            /* Check overflow buffer. */
            if(vBuffer.size() > 0)
            {
                nLastSend = runtime::timestamp();
                vBuffer.insert(vBuffer.end(), vData.begin(), vData.end());

                return static_cast<int32_t>(nBytes);
            }
        }

        /* If there were any errors, handle them gracefully. */
        {
            LOCK(SOCKET_MUTEX);

            #ifdef WIN32
                nSent = static_cast<int32_t>(send(fd, (char*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #else
                nSent = static_cast<int32_t>(send(fd, (int8_t*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #endif
        }


        if(nSent < 0)
        {
            nError = WSAGetLastError();
            return nError;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent != vData.size())
        {
            LOCK(DATA_MUTEX);
            vBuffer.insert(vBuffer.end(), vData.begin() + nSent, vData.end());
        }
        else //don't update last sent unless all the data was written to the buffer
            nLastSend = runtime::timestamp();

        return nSent;
    }


    /* Flushes data out of the overflow buffer */
    int Socket::Flush()
    {
        LOCK(DATA_MUTEX);

        int32_t nSent = 0;
        uint32_t nBytes = 0;
        uint32_t nSize  = static_cast<uint32_t>(vBuffer.size());


        /* Don't flush if buffer doesn't have any data. */
        if(nSize == 0)
            return 0;

        /* Set the maximum bytes to flush to 2^16 or maximum socket buffers. */
        nBytes = std::min(nSize, std::min((uint32_t)config::GetArg("-maxsendsize", 65535u), 65535u));

        /* If there were any errors, handle them gracefully. */
        {
            LOCK(SOCKET_MUTEX);

            #ifdef WIN32
                nSent = static_cast<int32_t>(send(fd, (char*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #else
                nSent = static_cast<int32_t>(send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
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
            /* Erase from current buffer. */
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + nSent);

            /* Update socket timers. */
            nLastSend = runtime::timestamp();
        }

        return nSent;
    }


    /*  Determines if nTime seconds have elapsed since last Read / Write. */
    bool Socket::Timeout(const uint32_t nTime, const uint8_t nFlags) const
    {
        /* Check for write flags. */
        bool fRet = true;
        if(nFlags & WRITE)
            fRet = (fRet && runtime::timestamp() > (uint64_t)(nLastSend + nTime));

        /* Check for read flags. */
        if(nFlags & READ)
            fRet = (fRet && runtime::timestamp() > (uint64_t)(nLastRecv + nTime));

        return fRet;
    }


    /* Check that the socket has data that is buffered. */
    uint64_t Socket::Buffered() const
    {
        LOCK(DATA_MUTEX);

        return vBuffer.size();
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
        if(nError == WSAEWOULDBLOCK ||
            nError == WSAEMSGSIZE ||
            nError == WSAEINTR ||
            nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }


}
