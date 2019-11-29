/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_SOCKET_H
#define NEXUS_LLP_TEMPLATES_SOCKET_H


#include <LLP/include/base_address.h>
#include <LLP/include/network.h>

#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>

namespace LLP
{

    /** Max send buffer size. **/
    const uint64_t MAX_SEND_BUFFER = 1024 * 1024; //1MB max send buffer


    /** Socket
     *
     *  Base Template class to handle outgoing / incoming LLP data for both
     *  Client and Server.
     *
     **/
    class Socket : public pollfd
    {
    private:

        /** Mutex for thread synchronization. **/
        mutable std::mutex SOCKET_MUTEX;

    protected:

        mutable std::mutex DATA_MUTEX;

        /** Keep track of last time data was sent. **/
        std::atomic<uint64_t> nLastSend;


        /** Keep track of last time data was received. **/
        std::atomic<uint64_t> nLastRecv;


        /** The error codes for socket. **/
        std::atomic<int32_t> nError;


        /** Oversize buffer for large packets. **/
        std::vector<uint8_t> vBuffer;


        /** Flag to catch if buffer write failed. **/
        std::atomic<bool> fBufferFull;


    public:


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
        Socket(int32_t nSocketIn, const BaseAddress &addrIn);


        /** Constructor for socket
         *
         *  @param[in] addrDest The address to connect socket to
         *
         **/
        Socket(const BaseAddress &addrDest);


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
        bool Attempt(const BaseAddress &addrDest, uint32_t nTimeout = 5000);


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


        /** Error
         *
         *  Give the message (c-string) of the error in the socket.
         *
         **/
        char* Error() const;


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
