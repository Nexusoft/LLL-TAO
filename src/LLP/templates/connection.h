/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_CONNECTION_H
#define NEXUS_LLP_TEMPLATES_CONNECTION_H

#include <vector>
#include <stdio.h>

#include <LLP/include/legacyaddress.h>
#include <LLP/packets/packet.h>
#include <LLP/templates/socket.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/args.h>

namespace LLP
{

    /** BaseConnection
     *
     *  Base Template class to handle outgoing / incoming LLP data for both Client and Server.
     *
     **/
    template<typename PacketType = Packet>
    class BaseConnection : public Socket
    {
    protected:

        /** Mutex for thread synchronization. **/
        std::mutex MUTEX;


        /** Event
         *
         *  Pure Virtual Event Function to be Overridden allowing Custom Read Events.
         *  Each event fired on Header Complete, and each time data is read to fill packet.
         *  Useful to check Header length to maximum size of packet type for DDOS protection,
         *  sending a keep-alive ping while downloading large files, etc.
         *
         *  LENGTH == 0: General Events
         *  LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
         *
         **/
        virtual void Event(uint8_t EVENT, uint32_t LENGTH = 0) = 0;


        /** ProcessPacket
         *
         *  Pure Virtual Process Function. To be overridden with your own custom
         *  packet processing.
         *
         **/
        virtual bool ProcessPacket() = 0;

    public:

        /** Incoming Packet Being Built. **/
        PacketType     INCOMING;


        /** DDOS Score for Connection. **/
        DDOS_Filter*   DDOS;


        /** Flag to Determine if DDOS is Enabled. **/
        bool fDDOS;


        /** Flag to Determine if the connection was made by this Node. **/
        bool fOUTGOING;


        /** Flag to determine if the connection is active. **/
        bool fCONNECTED;


        /** Build Base Connection with no parameters **/
        BaseConnection()
        : Socket()
        , INCOMING()
        , DDOS(nullptr)
        , fDDOS(false)
        , fOUTGOING(false)
        , fCONNECTED(false)
        {
            INCOMING.SetNull();
        }


        /** Build Base Connection with all Parameters. **/
        BaseConnection(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false, bool fOutgoing = false)
        : Socket(SOCKET_IN)
        , INCOMING()
        , DDOS(DDOS_IN)
        , fDDOS(isDDOS)
        , fOUTGOING(fOutgoing)
        , fCONNECTED(false)
        {
        }


        /* Default destructor */
        virtual ~BaseConnection()
        {
            Disconnect();
        }


        /** Reset
         *
         *  Resets the internal timers.
         *
         **/
        void Reset()
        {
            nLastRecv = runtime::timestamp();
            nLastSend = runtime::timestamp();
        }


        /** SetNull
         *
         *  Sets the object to an invalid state.
         *
         **/
        void SetNull()
        {
            fd = -1;
            nError = 0;

            INCOMING.SetNull();
            DDOS  = nullptr;
            fDDOS = false;
            fOUTGOING = false;
            fCONNECTED = false;
        }


        /** IsNull
         *
         *  Checks if is in null state.
         *
         **/
        bool IsNull() const
        {
            return fd == -1;
        }


        /** Errors
         *
         *  Checks for any flags in the Error Handle.
         *
         **/
        bool Errors() const
        {
            return ErrorCode() != 0;
        }


        /** Error
         *
         *  Give the message (c-string) of the error in the socket.
         *
         **/
        char* Error()
        {
            return strerror(ErrorCode());
        }


        /** Connected
         *
         *  Connection flag to determine if socket should be handled if not connected.
         *
         **/
        bool Connected()
        {
            return fCONNECTED;
        }


        /** Timeout
         *
         *  Determines if nTime seconds have elapsed since last Read / Write.
         *
         *  @param[in] nTime The time in seconds.
         *
         **/
        bool Timeout(uint32_t nTime)
        {
            return (runtime::timestamp() > nLastSend + nTime &&
                    runtime::timestamp() > nLastRecv + nTime);
        }


        /** PacketComplete
         *
         *  Handles two types of packets, requests which are of header >= 128,
         *  and data which are of header < 128.
         **/
        bool PacketComplete()
        {
            return INCOMING.Complete();
        }


        /** ResetPacket
         *
         *  Used to reset the packet to Null after it has been processed.
         *  This then flags the Connection to read another packet.
         *
         **/
        void ResetPacket()
        {
            INCOMING.SetNull();
        }


        /** WritePacket
         *
         *  Write a single packet to the TCP stream.
         *
         *  @param[in] PACKET The packet of type PacketType to write.
         *
         **/
        void WritePacket(PacketType PACKET)
        {
            LOCK(MUTEX);

            /* Debug dump of message type. */
            debug::log(3, NODE "Sent Message (", PACKET.GetBytes().size(), " bytes)");

            /* Debug dump of packet data. */
            if(config::GetArg("-verbose", 0) >= 5)
                PrintHex(PACKET.GetBytes());

            /* Write the packet to socket buffer. */
            std::vector<uint8_t> vBytes = PACKET.GetBytes();
            Write(vBytes, vBytes.size());
        }


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        virtual void ReadPacket() = 0;


        /** Connect
         *
         *  Connect Socket to a Remote Endpoint.
         *
         *  @param[in] strAddress The IP address string.
         *  @param[in] nPort The port number.
         *
         *  @return Returns true if successful connection, false otherwise.
         *
         */
        bool Connect(std::string strAddress, uint16_t nPort)
        {
            BaseAddress addrConnect(strAddress, nPort);

            /// debug print
            debug::log(1, NODE, "Connecting to ", addrConnect.ToString());

            // Connect
            if (Attempt(addrConnect))
            {
                /// debug print
                debug::log(1, NODE, "Connected to ", addrConnect.ToString());

                fCONNECTED = true;
                fOUTGOING  = true;

                return true;
            }

            return false;
        }

        /** GetAddress
         *
         *  Returns the address of socket.
         *
         **/
        BaseAddress GetAddress()
        {
            return addr;
        }


        /** Disconnect
         *
         *  Disconnect Socket. Cleans up memory usage to prevent "memory runs"
         *  from poor memory management.
         *
         **/
        void Disconnect()
        {
            Close();

            fCONNECTED = false;
        }

    };


    /** Connection
     *
     *
     *
     **/
    class Connection : public BaseConnection<Packet>
    {
    public:

        /** Default Constructor **/
        Connection()
        : BaseConnection() { }

        /** Constructor **/
        Connection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false, bool fOutgoing = false)
        : BaseConnection(SOCKET_IN, DDOS_IN, isDDOS, fOutgoing) { }


        /** ReadPacket
         *
         *  Regular Connection Read Packet Method.
         *
         **/
        void ReadPacket() final
        {

            /* Handle Reading Packet Type Header. */
            if(Available() >= 1 && INCOMING.IsNull())
            {
                std::vector<uint8_t> HEADER(1, 255);
                if(Read(HEADER, 1) == 1)
                    INCOMING.HEADER = HEADER[0];
            }

            /* Read the packet length. */
            if(Available() >= 4 && INCOMING.LENGTH == 0)
            {
                /* Handle Reading Packet Length Header. */
                std::vector<uint8_t> BYTES(4, 0);
                if(Read(BYTES, 4) == 4)
                {
                    INCOMING.SetLength(BYTES);
                    Event(EVENT_HEADER);
                }
            }

            /* Handle Reading Packet Data. */
            uint32_t nAvailable = Available();
            if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                /* Read the data in the packet */
                std::vector<uint8_t> DATA( std::min(nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                /* On successful read, fire event and add data to packet. */
                if(Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    Event(EVENT_PACKET, DATA.size());
                }
            }
        }
    };
}

#endif
