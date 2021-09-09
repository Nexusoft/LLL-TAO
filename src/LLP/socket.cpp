/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <string>
#include <vector>
#include <stdio.h>

#include <LLP/include/network.h>
#include <LLP/templates/socket.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <sys/ioctl.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>

namespace LLP
{

    /* The default constructor. */
    Socket::Socket()
    : pollfd             ( )
    , SOCKET_MUTEX       ( )
    , pSSL(nullptr)
    , DATA_MUTEX         ( )
    , nLastSend          (0)
    , nLastRecv          (0)
    , nError             (0)
    , vBuffer            ( )
    , fBufferFull        (false)
    , nConsecutiveErrors (0)
    , addr               ( )
    {
        fd = INVALID_SOCKET;
        events = POLLIN;

        /* Reset the internal timers. */
        Reset();
    }


    /* Copy constructor. */
    Socket::Socket(const Socket& socket)
    : pollfd             (socket)
    , SOCKET_MUTEX       ( )
    , pSSL(nullptr)
    , DATA_MUTEX         ( )
    , nLastSend          (socket.nLastSend.load())
    , nLastRecv          (socket.nLastRecv.load())
    , nError             (socket.nError.load())
    , vBuffer            (socket.vBuffer)
    , fBufferFull        (socket.fBufferFull.load())
    , nConsecutiveErrors (socket.nConsecutiveErrors.load())
    , addr               (socket.addr)
    {
        if(socket.pSSL)
        {
            /* Assign the SSL struct and increment the reference count so that we don't destroy it twice */
            pSSL = socket.pSSL;

            /* In OpenSSL 1.0 the reference count is accessed directly.  We use CRYPTO_add to increment it so that it is threadsafe.
               From OpenSSL 1.1.0 onwards the SSL object has been made opaque so we need to incrememt it using SSL_up_ref */
            #if OPENSSL_VERSION_NUMBER < 0x10100000L
                CRYPTO_add(&pSSL->references, 1, CRYPTO_LOCK_SSL);
            #else
                SSL_up_ref(pSSL);
            #endif

            /* Set the socket file descriptor on the SSL object to this socket */
            SSL_set_fd(pSSL, fd);
        }

    }


    /** The socket constructor. **/
    Socket::Socket(int32_t nSocketIn, const BaseAddress &addrIn, const bool& fSSL)
    : pollfd             ( )
    , SOCKET_MUTEX       ( )
    , pSSL(nullptr)
    , DATA_MUTEX         ( )
    , nLastSend          (0)
    , nLastRecv          (0)
    , nError             (0)
    , vBuffer            ( )
    , fBufferFull        (false)
    , nConsecutiveErrors (0)
    , addr               (addrIn)
    {
        fd = nSocketIn;
        events = POLLIN;

        /* Determine if socket should use SSL. */
        SetSSL(fSSL);

        /* TCP connection is ready. Do server side SSL. */
        if(fSSL)
        {
            SSL_set_fd(pSSL, fd);
            SSL_set_accept_state(pSSL);

            int32_t nStatus = -1;

            fd_set fdWriteSet;
            fd_set fdReadSet;
            struct timeval tv;

            do
            {
                FD_ZERO(&fdWriteSet);
                FD_ZERO(&fdReadSet);
                tv.tv_sec = 2; // timeout after 2 seconds
                tv.tv_usec = 0;

                nStatus = SSL_accept(pSSL);

                /* Check for successful accept */
                if(nStatus == 1)
                    break;

                nError.store(SSL_get_error(pSSL, nStatus));

                switch(nError.load())
                {
                case SSL_ERROR_WANT_READ:
                    FD_SET(fd, &fdReadSet);
                    nStatus = 1; // Wait for more activity
                    break;
                case SSL_ERROR_WANT_WRITE:
                    FD_SET(fd, &fdWriteSet);
                    nStatus = 1; // Wait for more activity
                    break;
                case SSL_ERROR_NONE:
                    nStatus = 0; // success!
                    break;
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                    /* The peer has notified us that it is shutting down via the SSL "close_notify" message so we
                       need to shutdown, too. */
                    debug::log(3, FUNCTION, "SSL handshake - Peer closed connection ");
                    nError.store(ERR_get_error());
                    nStatus = -1;
                    break;
                default:
                    /* Some other error so break out */
                    nError.store(ERR_get_error());
                    nStatus = -1;
                    break;
                }

                if (nStatus == 1)
                {
                    // Must have at least one handle to wait for at this point.
                    nStatus = select(fd + 1, &fdReadSet, &fdWriteSet, NULL, &tv);

                    // 0 is timeout, so we're done.
                    // -1 is error, so we're done.
                    // Could be both handles set (same handle in both masks) so
                    // set to 1.
                    if (nStatus >= 1)
                    {
                        nStatus = 1;
                    }
                    else // Timeout or failure
                    {
                        debug::log(3, FUNCTION, "SSL handshake - peer timeout or failure");
                        nStatus = -1;
                    }
                }
            }
            while (nStatus == 1 && !SSL_is_init_finished(pSSL));

            if(nStatus >= 0)
                debug::log(3, FUNCTION, "SSL Connection using ", SSL_get_cipher(pSSL));
            else
            {
                if(nError)
                    debug::log(3, FUNCTION, "SSL Accept failed ",  addr.ToString(), " (", nError, " ", ERR_reason_error_string(nError), ")");
                else
                    debug::log(3, FUNCTION, "SSL Accept failed ",  addr.ToString());
                /* Free the SSL resources */
                SSL_free(pSSL);
                pSSL = nullptr;
            }

        }

        /* Reset the internal timers. */
        Reset();
    }


    /* Constructor for socket */
    Socket::Socket(const BaseAddress &addrConnect, const bool& fSSL)
    : pollfd             ( )
    , SOCKET_MUTEX       ( )
    , pSSL(nullptr)
    , DATA_MUTEX         ( )
    , nLastSend          (0)
    , nLastRecv          (0)
    , nError             (0)
    , vBuffer            ( )
    , fBufferFull        (false)
    , nConsecutiveErrors (0)
    , addr               ( )
    {
        fd = INVALID_SOCKET;
        events = POLLIN;

        /* Reset the internal timers. */
        Reset();

        /* Determine if socket should use SSL. */
        SetSSL(fSSL);

        /* Connect socket to external address. */
        Attempt(addrConnect);
    }


    /* Destructor for socket */
    Socket::~Socket()
    {
        /* Free the ssl object. */
        SetSSL(false);
    }


    /* Returns the address of the socket. */
    BaseAddress Socket::GetAddress() const
    {
        LOCK(DATA_MUTEX);

        return addr;
    }


    /* Resets the internal timers. */
    void Socket::Reset()
    {
        /* Atomic data types, no need for lock */
        nLastRecv = runtime::timestamp(true);
        nLastSend = runtime::timestamp(true);

        /* Reset errors. */
        nConsecutiveErrors = 0;
    }


    /* Connects the socket to an external address */
    bool Socket::Attempt(const BaseAddress &addrDest, uint32_t nTimeout)
    {
        runtime::timer nElapsed;
        nElapsed.Start();

        bool fConnected = false;

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

                /* The poll result */
                int32_t nPoll = 0;
                for(uint32_t nSeconds = 0; nSeconds < nIterators && !config::fShutdown.load() && nPoll <= 0; ++nSeconds)
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
                        fConnected = false;
                    }
                }

                /* Check for timeout. */
                if(nPoll == 0)
                {
                    fConnected = false;
                    debug::log(3, FUNCTION, "poll timeout ", addrDest.ToString(), " (", nError, ")");
                }
                else if((fds[0].revents & POLLERR) || (fds[0].revents & POLLHUP))
                {
                    fConnected = false;
                    debug::log(3, FUNCTION, "Connection failed ", addrDest.ToString());
                }
            }
            else if (nError != WSAEISCONN)
            {
                fConnected = false;
                debug::log(3, FUNCTION, "Connection failed ", addrDest.ToString(), " (", nError, ")");
            }
        }

        if(!fConnected)
        {
            /* We want to maintain a maximum of 1 connection per second. */
            while(nElapsed.ElapsedMilliseconds() < 1000)
                runtime::sleep(100);

            /* No point sending the close notify to the peer as the connection was never established, so just clean up the SSL */
            if(pSSL)
            {
                /* Free the SSL resources */
                SSL_free(pSSL);
                pSSL = nullptr;
            }

            /* Attempt to close the socket our side to clean up resources */
            closesocket(fd);

            return false;
        }

        /* If the socket connection is established and SSL is enabled then initiate the SSL handshake */
        else if(fConnected && pSSL)
        {
            /* Set up the SSL object for connecting */
            SSL_set_fd(pSSL, fd);
            SSL_set_connect_state(pSSL);

            int32_t nStatus = -1;

            fd_set fdWriteSet;
            fd_set fdReadSet;
            struct timeval tv;

            do
            {
                FD_ZERO(&fdWriteSet);
                FD_ZERO(&fdReadSet);
                tv.tv_sec = nTimeout / 1000; // convert timeout to seconds
                tv.tv_usec = 0;

                nStatus = SSL_connect(pSSL);

                switch (SSL_get_error(pSSL, nStatus))
                {
                case SSL_ERROR_WANT_READ:
                    FD_SET(fd, &fdReadSet);
                    nStatus = 1; // Wait for more activity
                    break;
                case SSL_ERROR_WANT_WRITE:
                    FD_SET(fd, &fdWriteSet);
                    nStatus = 1; // Wait for more activity
                    break;
                case SSL_ERROR_NONE:
                    nStatus = 0; // success!
                    break;
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                    /* The peer has notified us that it is shutting down via the SSL "close_notify" message so we
                       need to shutdown, too. */
                    debug::log(3, FUNCTION, "SSL handshake - Peer closed connection ");
                    nError.store(ERR_get_error());
                    nStatus = -1;
                    break;
                default:
                    /* Some other error so break out */
                    nError.store(ERR_get_error());
                    nStatus = -1;
                    break;
                }

                if (nStatus == 1)
                {
                    // Must have at least one handle to wait for at this point.
                    nStatus = select(fd + 1, &fdReadSet, &fdWriteSet, NULL, &tv);

                    // 0 is timeout, so we're done.
                    // -1 is error, so we're done.
                    // Could be both handles set (same handle in both masks) so
                    // set to 1.
                    if (nStatus >= 1)
                    {
                        nStatus = 1;
                    }
                    else // Timeout or failure
                    {
                        debug::log(3, FUNCTION, "SSL handshake - peer timeout or failure");
                        nError.store(nStatus);
                        nStatus = -1;
                    }
                }
            }
            while (nStatus == 1);

            fConnected = nStatus >= 0;

            if(fConnected)
                debug::log(3, FUNCTION, "SSL connected using ", SSL_get_cipher(pSSL));
            else
            {
                if(nError)
                    debug::log(3, FUNCTION, "SSL Accept failed ",  addr.ToString(), " (", nError, " ", ERR_reason_error_string(nError), ")");
                else
                    debug::log(3, FUNCTION, "SSL Accept failed ",  addr.ToString());

                /* Attempt to close the socket our side to clean up resources */
                closesocket(fd);

                return false;
            }

        }

        /* Reset the internal timers. */
        Reset();

        return fConnected;
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

        /* If using SSL we must also include any bytes in the pending buffer that were not read in the last call to SSL_read . */
        if(pSSL)
            nAvailable += SSL_pending(pSSL);

        return nAvailable;
    }


    /* Clear resources associated with socket and return to invalid state. */
    void Socket::Close()
    {
        LOCK(SOCKET_MUTEX);

        if(fd != INVALID_SOCKET)
        {
            if(IsSSL())
            {
                /* Before we can send the SSL_shutdown message we need to determine if the socket is still connected.  If we don't
                   check this, then SSL_shutdown throws an exception when writing to the FD.  The easiest way to check that the
                   non-blocking socket is still connected is to poll it and check for POLLERR / POLLHUP errors */

                pollfd fds[1];
                fds[0].events = POLLIN;
                fds[0].fd = fd;

                /* Poll the socket with 100ms timeout. */
#ifdef WIN32
                WSAPoll(&fds[0], 1, 100);
#else
                poll(&fds[0], 1, 100);
#endif
                /* Check for errors to be certain the socket is still connected */
                if(!(fds[0].revents & POLLERR || fds[0].revents & POLLHUP) )
                    /* Shut down a TLS/SSL connection by sending the "close notify" shutdown alert to the peer. */
                    SSL_shutdown(pSSL);

                /* Clean up the SSL object */
                SSL_free(pSSL);
                pSSL = nullptr;
            }

            closesocket(fd);
        }

        fd = INVALID_SOCKET;


    }


    /* Read data from the socket buffer non-blocking */
    int Socket::Read(std::vector<uint8_t> &vData, size_t nBytes)
    {
        LOCK(SOCKET_MUTEX);

        /* Reset the error status */
        nError.store(0);

        int32_t nRead = 0;

        if(pSSL)
            nRead = SSL_read(pSSL, (int8_t*)&vData[0], nBytes);
        else
        {
        #ifdef WIN32
            nRead = static_cast<int32_t>(recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT));
        #else
            nRead = static_cast<int32_t>(recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT));
        #endif
        }

        if (nRead <= 0)
        {
            if(pSSL)
            {
                int nSSLError = SSL_get_error(pSSL, nRead);

                switch (nSSLError)
                {
                    case SSL_ERROR_NONE:
                    {
                        // no real error, just try again...
                        break;
                    }

                    case SSL_ERROR_SSL:
                    {
                        // peer disconnected...
                        debug::error(FUNCTION, "Peer disconnected." );
                        nError.store(ERR_get_error());
                        break;
                    }

                    case SSL_ERROR_ZERO_RETURN:
                    {
                        // peer disconnected...
                        debug::error(FUNCTION, "Peer disconnected." );
                        nError.store(ERR_get_error());
                        break;
                    }

                    case SSL_ERROR_WANT_READ:
                    {
                        // no data available right now as it needs to read more from the underlying socket
                        break;
                    }

                    case SSL_ERROR_WANT_WRITE:
                    {
                        // socket not writable right now, wait and try again
                        break;
                    }

                    default:
                    {
                        nError.store(ERR_get_error());
                        break;
                    }
                }

                /* Check if an error occurred before logging */
                if(nError.load() > 0)
                    debug::log(3, FUNCTION, "SSL_read failed ",  addr.ToString(), " (", nError, " ", ERR_reason_error_string(nError), ")");

            }
            else
            {
                nError = WSAGetLastError();
                debug::log(3, FUNCTION, "read failed ", addr.ToString(), " (", nError, " ", strerror(nError), ")");
            }
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp(true);

        return nRead;
    }


    /* Read data from the socket buffer non-blocking */
    int32_t Socket::Read(std::vector<int8_t> &vData, size_t nBytes)
    {
        LOCK(SOCKET_MUTEX);

        /* Reset the error status */
        nError.store(0);

        int32_t nRead = 0;

        if(pSSL)
            nRead = SSL_read(pSSL, (int8_t*)&vData[0], nBytes);
        else
        {
        #ifdef WIN32
            nRead = static_cast<int32_t>(recv(fd, (char*)&vData[0], nBytes, MSG_DONTWAIT));
        #else
            nRead = static_cast<int32_t>(recv(fd, (int8_t*)&vData[0], nBytes, MSG_DONTWAIT));
        #endif
        }


        if (nRead < 0)
        {
            if(pSSL)
            {
                int nSSLError = SSL_get_error(pSSL, nRead);

                switch (nSSLError)
                {
                    case SSL_ERROR_NONE:
                    {
                        // no real error, just try again...
                        break;
                    }

                    case SSL_ERROR_SSL:
                    {
                        // peer disconnected...
                        debug::error(FUNCTION, "Peer disconnected." );
                        nError.store(ERR_get_error());
                        break;
                    }

                    case SSL_ERROR_ZERO_RETURN:
                    {
                        // peer disconnected...
                        debug::error(FUNCTION, "Peer disconnected." );
                        nError.store(ERR_get_error());
                        break;
                    }

                    case SSL_ERROR_WANT_READ:
                    {
                        // no data available right now as it needs to read more from the underlying socket
                        break;
                    }

                    case SSL_ERROR_WANT_WRITE:
                    {
                        // socket not writable right now, wait and try again
                        break;
                    }

                    default:
                    {
                        nError.store(ERR_get_error());
                        break;
                    }
                }

                /* Check if an error occurred before logging */
                if(nError.load() > 0)
                    debug::log(3, FUNCTION, "SSL_read failed ",  addr.ToString(), " (", nError, " ", ERR_reason_error_string(nError), ")");

            }
            else
            {
                nError = WSAGetLastError();
                debug::log(3, FUNCTION, "read failed ",  addr.ToString(), " (", nError, " ", strerror(nError), ")");
            }
        }
        else if(nRead > 0)
            nLastRecv = runtime::timestamp(true);

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
                debug::log(3, FUNCTION, "vBuffer ", vBuffer.size(), " bytes");

                vBuffer.insert(vBuffer.end(), vData.begin(), vData.end());

                return static_cast<int32_t>(nBytes);
            }
        }

        /* Write the packet. */
        {
            LOCK(SOCKET_MUTEX);

            if(pSSL)
                nSent = static_cast<int32_t>(SSL_write(pSSL, (int8_t*)&vData[0], nBytes));
            else
            {
            #ifdef WIN32
                nSent = static_cast<int32_t>(send(fd, (char*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #else
                nSent = static_cast<int32_t>(send(fd, (int8_t*)&vData[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #endif
            }
        }


        /* Handle for error state. */
        if(nSent < 0)
        {
            if(pSSL)
                nError = SSL_get_error(pSSL, nSent);
            else
                nError = WSAGetLastError();
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent != vData.size())
        {
            LOCK(DATA_MUTEX);
            vBuffer.insert(vBuffer.end(), vData.begin() + nSent, vData.end());
        }
        else //don't update last sent unless all the data was written to the buffer
            nLastSend = runtime::timestamp(true);

        return nSent;
    }


    /* Flushes data out of the overflow buffer */
    int Socket::Flush()
    {
        int32_t nSent   = 0;
        uint32_t nBytes = 0;
        uint32_t nSize  = 0;

        {
            LOCK(DATA_MUTEX);
            nSize = static_cast<uint32_t>(vBuffer.size());
        }

        /* Don't flush if buffer doesn't have any data. */
        if(nSize == 0)
            return 0;

        /* maximum transmission unit. */
        const uint32_t MTU = 16384;

        /* Set the maximum bytes to flush to 2^16 or maximum socket buffers. */
        nBytes = std::min(nSize, std::min((uint32_t)config::GetArg("-maxsendsize", MTU), MTU));

        /* If there were any errors, handle them gracefully. */
        {
            LOCK2(DATA_MUTEX);
            LOCK(SOCKET_MUTEX);

            if(pSSL)
                nSent = static_cast<int32_t>(SSL_write(pSSL, (int8_t *)&vBuffer[0], nBytes));
            else
            {
            #ifdef WIN32
                nSent = static_cast<int32_t>(send(fd, (char*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #else
                nSent = static_cast<int32_t>(send(fd, (int8_t*)&vBuffer[0], nBytes, MSG_NOSIGNAL | MSG_DONTWAIT));
            #endif
            }

        }

        /* Handle errors on flush. */
        if(nSent < 0)
        {
            if(pSSL)
                nError = SSL_get_error(pSSL, nSent);
            else
                nError = WSAGetLastError();

            ++nConsecutiveErrors;
        }

        /* If not all data was sent non-blocking, recurse until it is complete. */
        else if(nSent > 0)
        {
            LOCK(DATA_MUTEX);

            /* Erase from current buffer. */
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + nSent);

            /* Update socket timers. */
            nLastSend          = runtime::timestamp(true);
            nConsecutiveErrors = 0;

            /* Reset that buffers are full. */
            fBufferFull.store(false);
        }

        return nSent;
    }


    /*  Determines if nTime seconds have elapsed since last Read / Write. */
    bool Socket::Timeout(const uint32_t nTime, const uint8_t nFlags) const
    {
        /* Check for write flags. */
        bool fRet = true;
        if(nFlags & WRITE)
            fRet = (fRet && (runtime::timestamp(true) > uint64_t(nLastSend + nTime)));

        /* Check for read flags. */
        if(nFlags & READ)
            fRet = (fRet && (runtime::timestamp(true) > uint64_t(nLastRecv + nTime)));

        return fRet;
    }


    /* Check that the socket has data that is buffered. */
    uint64_t Socket::Buffered() const
    {
        return vBuffer.size();
    }


    /*  Checks if is in null state. */
    bool Socket::IsNull() const
    {
        return fd == -1;
    }


    /*  Checks for any flags in the Error Handle. */
    bool Socket::Errors() const
    {
        LOCK(DATA_MUTEX);

        return error_code() != 0;
    }


    /*  Give the message (c-string) of the error in the socket. */
    const char *Socket::Error() const
    {
        LOCK(DATA_MUTEX);

        if(pSSL)
            return ERR_reason_error_string(error_code());

        return strerror(error_code());
    }


    /* Returns the error of socket if any */
    int Socket::error_code() const
    {

        if(pSSL)
        {
            /* Check for errors from reads or writes. */
            if(nError == SSL_ERROR_WANT_READ || nError == SSL_ERROR_WANT_WRITE)
                return 0;

            return nError;
        }


        /* Check for errors from reads or writes. */
        if(nError == WSAEWOULDBLOCK  || nError == WSAEMSGSIZE  || nError == WSAEINTR  || nError == WSAEINPROGRESS)
            return 0;

        return nError;
    }

    /*  Creates or destroys the SSL object depending on the flag set. */
    void Socket::SetSSL(bool fSSL)
    {
        LOCK(DATA_MUTEX);

        if(fSSL && pSSL == nullptr)
        {
            pSSL = SSL_new(pSSL_CTX);
        }
        else if(pSSL)
        {
            SSL_free(pSSL);
            pSSL = nullptr;
        }
    }


    /* Determines if socket is using SSL encryption. */
    bool Socket::IsSSL() const
    {
        LOCK(DATA_MUTEX);

        if(pSSL != nullptr)
            return SSL_is_init_finished(pSSL);

        return false;
    }

}
