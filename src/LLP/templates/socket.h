/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_SOCKET_H
#define NEXUS_LLP_TEMPLATES_SOCKET_H


#include <LLP/include/base_address.h>

#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>


typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace LLP
{

    /** Max send buffer size. **/
    const uint64_t MAX_SEND_BUFFER = 17 * 1024 * 1024; //17MB max send buffer


    /** Socket
     *
     *  Base Template class to handle outgoing / incoming LLP data for both
     *  Client and Server.
     *
     **/
    class Socket : public pollfd
    {

        /** Mutex for thread synchronization. **/
        mutable std::recursive_mutex SOCKET_MUTEX;


        /* SSL object */
        SSL* pSSL;

    protected:

        /** Mutex to protect buffered data. **/
        mutable std::mutex ADDRESS_MUTEX;


        /** Keep track of last time data was sent. **/
        std::atomic<uint64_t> nLastSend;


        /** Keep track of last time data was received. **/
        std::atomic<uint64_t> nLastRecv;


        /** The error codes for socket. **/
        std::atomic<int32_t> nError;


        /** Oversize buffer for low-priority packets (templates / pushes). **/
        std::vector<uint8_t> vBuffer;


        /** Read offset into vBuffer.
         *
         *  Instead of erasing sent bytes from the front of vBuffer (O(n) memmove),
         *  Flush() advances this offset past the bytes it has sent.  vBuffer is
         *  compacted (cleared and offset reset) only when all bytes have been
         *  consumed, keeping every Flush() iteration O(1) for the common case.
         *
         *  Protected by SOCKET_MUTEX (same as vBuffer).
         *  Always satisfies: m_nFlushOffset <= vBuffer.size(). **/
        size_t m_nFlushOffset;

        /** Oversize buffer for high-priority control packets (ACKs / round replies). **/
        std::vector<uint8_t> vPriorityBuffer;


        /** Read offset into vPriorityBuffer.
         *
         *  Mirrors m_nFlushOffset for the high-priority control-plane buffer.
         *  Protected by SOCKET_MUTEX and always satisfies:
         *  m_nPriorityFlushOffset <= vPriorityBuffer.size().
         **/
        size_t m_nPriorityFlushOffset;


        /** Keep track of total buffered bytes across both priority classes. */
        std::atomic<uint64_t> nBufferSize;


        /** Flag to catch if buffer write failed. **/
        std::atomic<bool> fBufferFull;


    public:


        /** Flag to detect consecutive errors. **/
        std::atomic<uint32_t> nConsecutiveErrors;


        /** Timeout flags. **/
        enum
        {
            READ  = (1 << 1),
            WRITE = (1 << 2),
            ALL   = (READ | WRITE)
        };


        /** The address of this connection. */
        BaseAddress addr;


        /** The default constructor. **/
        Socket();


        /** Copy constructor. **/
        Socket(const Socket& socket);


        /** The socket constructor. **/
        Socket(int32_t nSocketIn, const BaseAddress &addrIn, const bool& fSSL = false);


        /** Constructor for socket
         *
         *  @param[in] addrConnect The address to connect socket to
         *
         **/
        Socket(const BaseAddress &addrConnect, const bool& fSSL = false);


        /** Destructor for socket **/
        virtual ~Socket();


        /** GetAddress
         *
         *  Returns the address of the socket.
         *
         **/
        BaseAddress GetAddress() const;


        /** Reset
        *
        *  Resets the internal timers.
        *
        **/
        void Reset();


        /** Attempts
         *
         *  Attempts to connect the socket to an external address
         *
         *  @param[in] addrConnect The address to connect to
         *
         *  @return true if the socket is in a valid state.
         *
         **/
        bool Attempt(const BaseAddress &addrDest, uint32_t nTimeout = 3000);


        /** Available
         *
         *  Poll the socket to check for available data
         *
         *  @return the total bytes available for read
         *
         **/
        int32_t Available() const;


        /** Close
         *
         *  Clear resources associated with socket and return to invalid state.
         *
         **/
        void Close();


        /** Read
         *
         *  Read data from the socket buffer non-blocking
         *
         *  @param[out] vData The byte vector to read into
         *  @param[in] nBytes The total bytes to read
         *
         *  @return the total bytes that were read
         *
         **/
        int32_t Read(std::vector<uint8_t>& vData, size_t nBytes);


        /** Read
         *
         *  Read data from the socket buffer non-blocking
         *
         *  @param[out] vchData The char vector to read into
         *  @param[in] nBytes The total bytes to read
         *
         *  @return the total bytes that were read
         *
         **/
        int32_t Read(std::vector<int8_t>& vchData, size_t nBytes);


        /** Write
         *
         *  Write data into the socket buffer non-blocking
         *
         *  @param[in] vData The byte vector of data to be written
         *  @param[in] nBytes The total bytes to write
         *
         *  @return the total bytes that were written
         *
         **/
        int32_t Write(const std::vector<uint8_t>& vData, size_t nBytes);

        int32_t Write(const std::vector<uint8_t>& vData, size_t nBytes, bool fPriority);


        /** Flush
         *
         *  Flushes data out of the overflow buffer
         *
         *  @return the total bytes that were written
         *
         **/
        int32_t Flush();


        /** Timeout
        *
        *  Determines if nTime seconds have elapsed since last Read / Write.
        *
        *  @param[in] nTime The time in seconds.
        *  @param[in] nFlags Flags to determine if checking reading or writing timeouts.
        *
        **/
        bool Timeout(const uint32_t nTime, const uint8_t nFlags = ALL) const;


        /** Buffered
         *
         *  Get the amount of data buffered.
         *
         **/
        uint64_t Buffered() const;


        /** IsNull
         *
         *  Checks if is in null state.
         *
         **/
        bool IsNull() const;


        /** Errors
          *
          *  Checks for any flags in the Error Handle.
          *
          **/
        bool Errors() const;


        /** RefreshSocketError
         *
         *  Query the kernel's pending SO_ERROR state for this socket and cache
         *  any non-zero result into nError so the DataThread can remove the
         *  connection on its next pass.
         *
         *  @return the pending socket error code, or 0 if none is pending.
         *
         **/
        int32_t RefreshSocketError();


        /** Error
         *
         *  Give the message (c-string) of the error in the socket.
         *
         **/
        const char* Error() const;


        /** SetSSL
         *
         *  Creates or destroys the SSL object depending on the flag set.
         *
         *  @param[in] fSSL_ The flag to set SSL on or off.
         *
         **/
         void SetSSL(bool fSSL);


         /** IsSSL
          *
          * Determines if socket is using SSL encryption.
          *
          **/
          bool IsSSL() const;


    private:

        /** error_code
         *
         *  Returns the error of socket if any
         *
         *  @return error code of the socket
         *
         **/
        int32_t error_code() const;

    };

}

#endif
