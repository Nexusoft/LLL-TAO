/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_SOCKET_H
#define NEXUS_LLP_TEMPLATES_SOCKET_H

#include <vector>

#include <LLP/include/network.h>

#include <LLP/include/baseaddress.h>

#include <Util/include/runtime.h>

namespace LLP
{

    /** Socket
     *
     *  Base Template class to handle outgoing / incoming LLP data for both
     *  Client and Server.
     *
     **/
    class Socket : public pollfd
    {
    protected:

        /** The error codes for socket. **/
        int32_t nError;


        /** Keep track of last time data was sent. **/
        uint32_t nLastSend;


        /** Keep track of last time data was received. **/
        uint32_t nLastRecv;


        /** Oversize buffer for large packets. **/
        std::vector<uint8_t> vBuffer;

    public:

        /** The address of this connection. */
        BaseAddress addr;


        /** The default constructor. **/
        Socket()
        : nError(0)
        , nLastSend(runtime::timestamp())
        , nLastRecv(runtime::timestamp())
        , vBuffer()
        , addr()
        {
            fd = -1;

            events = POLLIN;// | POLLOUT;
        }


        /** The socket constructor. **/
        Socket(int32_t nSocketIn, const BaseAddress &addrIn)
        : nError(0)
        , nLastSend(runtime::timestamp())
        , nLastRecv(runtime::timestamp())
        , vBuffer()
        , addr(addrIn)
        {
            fd = nSocketIn;

            events = POLLIN;// | POLLOUT;
        }

        /* Destroy the socket object. */
        ~Socket()
        {
            vBuffer.clear();
        }


        /** Constructor for Socket
         *
         *  @param[in] addrDest The address to connect socket to
         *
         **/
        Socket(BaseAddress addrDest);



        /** Error
         *
         *  Returns the error of socket if any
         *
         *  @return error code of the socket
         *
         **/
        int32_t ErrorCode() const;


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

    };

    /** Type Definitions for LLP Functions **/
    typedef Socket Socket_t;

}

#endif
