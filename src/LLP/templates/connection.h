/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_CONNECTION_H
#define NEXUS_LLP_TEMPLATES_CONNECTION_H

#include <vector>
#include <stdio.h>

#include <LLP/include/network.h>
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

    /* Base Template class to handle outgoing / incoming LLP data for both Client and Server. */
    template<typename PacketType = Packet> class BaseConnection
    {
    protected:

        /* Basic Connection Variables. */
        Timer                TIMER;
        Socket_t            SOCKET;
        std::recursive_mutex MUTEX;


        /*  Pure Virtual Event Function to be Overridden allowing Custom Read Events.
            Each event fired on Header Complete, and each time data is read to fill packet.
            Useful to check Header length to maximum size of packet type for DDOS protection,
            sending a keep-alive ping while downloading large files, etc.

            LENGTH == 0: General Events
            LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
        */
        virtual void Event(uint8_t EVENT, uint32_t LENGTH = 0) = 0;


        /* Pure Virtual Process Function. To be overridden with your own custom packet processing. */
        virtual bool ProcessPacket() = 0;
    public:


        /* Incoming Packet Being Built. */
        PacketType        INCOMING;


        /* DDOS Score if a Incoming Server Connection. */
        DDOS_Filter*   DDOS;


        /* Flag to Determine if DDOS is Enabled. */
        bool fDDOS;


        /* Flag to Determine if the connection was made by this Node. */
        bool fOUTGOING;


        /* Flag to determine if the connection is active. */
        bool fCONNECTED;


        /* Build Base Connection with no parameters */
        BaseConnection() : SOCKET(), INCOMING(), DDOS(NULL), fDDOS(false), fOUTGOING(false), fCONNECTED(false) { INCOMING.SetNull(); }


        /* Build Base Connection with all Parameters. */
        BaseConnection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false, bool fOutgoing = false) : SOCKET(SOCKET_IN), INCOMING(), DDOS(DDOS_IN), fDDOS(isDDOS),  fOUTGOING(fOutgoing), fCONNECTED(false) { TIMER.Start(); }

        virtual ~BaseConnection() { Disconnect(); }


        /* Checks for any flags in the Error Handle. */
        bool Errors(){ return SOCKET.Error() != 0; }


        /* Give the message (c-string) of the error in the socket. */
        char* Error(){ return strerror(SOCKET.Error()); }


        /* Connection flag to determine if socket should be handled if not connected. */
        bool Connected() { return fCONNECTED; }


        /* Determines if nTime seconds have elapsed since last Read / Write. */
        bool Timeout(uint32_t nTime){ return (TIMER.Elapsed() >= nTime); }


        /* Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. */
        bool PacketComplete(){ return INCOMING.Complete(); }


        /* Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. */
        void ResetPacket(){ INCOMING.SetNull(); }


        /* Write a single packet to the TCP stream. */
        void WritePacket(PacketType PACKET)
        {
            LOCK(MUTEX);

            /* Debug dump of message type. */
            debug::log(4, NODE "Sent Message (%u bytes)\n", PACKET.GetBytes().size());

            /* Debug dump of packet data. */
            if(config::GetArg("-verbose", 0) >= 5)
                PrintHex(PACKET.GetBytes());

            /* Write the packet to socket buffer. */
            Write(PACKET.GetBytes());
        }


        /* Non-Blocking Packet reader to build a packet from TCP Connection.
            This keeps thread from spending too much time for each Connection. */
        virtual void ReadPacket() = 0;


        /* Connect Socket to a Remote Endpoint. */
        bool Connect(std::string strAddress, int nPort)
        {
            CService addrConnect(debug::strprintf("%s:%i", strAddress.c_str(), nPort).c_str(), nPort);

            /// debug print
            debug::log(1, NODE "Connecting to %s\n", addrConnect.ToString().c_str());

            // Connect
            if (SOCKET.Connect(addrConnect))
            {
                /// debug print
                debug::log(1, NODE "Connected to %s\n", addrConnect.ToString().c_str());

                fCONNECTED = true;
                fOUTGOING  = true;

                return true;
            }

            return false;
        }


        /* Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management. */
        void Disconnect()
        {
            SOCKET.Close();

            fCONNECTED = false;
        }


//    protected:


        /* Lower level network communications: Read. Interacts with OS sockets. */
        int Read(std::vector<uint8_t> &DATA, size_t nBytes)
        {
            TIMER.Reset();

            return SOCKET.Read(DATA, nBytes);
        }


        /* Lower level network communications: Write. Interacts with OS sockets. */
        void Write(std::vector<uint8_t> DATA)
        {
            TIMER.Reset();

            SOCKET.Write(DATA, DATA.size());
        }

    };


    class Connection : public BaseConnection<Packet>
    {
    public:

        /* Connection Constructors */
        Connection() : BaseConnection() { }
        Connection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false, bool fOutgoing = false) : BaseConnection(SOCKET_IN, DDOS_IN, isDDOS, fOutgoing) { }


        /* Regular Connection Read Packet Method. */
        void ReadPacket()
        {

            /* Handle Reading Packet Type Header. */
            if(SOCKET.Available() >= 1 && INCOMING.IsNull())
            {
                std::vector<uint8_t> HEADER(1, 255);
                if(Read(HEADER, 1) == 1)
                    INCOMING.HEADER = HEADER[0];
            }

            /* Read the packet length. */
            if(SOCKET.Available() >= 4 && INCOMING.LENGTH == 0)
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
            uint32_t nAvailable = SOCKET.Available();
            if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
            {

                /* Read the data in the packet with a maximum of 512 bytes at a time. */
                std::vector<uint8_t> DATA( std::min( std::min(nAvailable, 512u), (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

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
