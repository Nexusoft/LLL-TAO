/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_TYPES_H
#define NEXUS_LLP_TEMPLATES_TYPES_H

#include <string>
#include <vector>
#include <stdio.h>

#include "../include/network.h"

#include "socket.h"

#include "../../Util/include/mutex.h"
#include "../../Util/include/runtime.h"
#include "../../Util/include/debug.h"
#include "../../Util/include/hex.h"
#include "../../Util/include/args.h"

namespace LLP
{

    /** Event Enumeration. Used to determine each Event from LLP Server. **/
    enum
    {
        EVENT_HEADER         = 0,
        EVENT_PACKET         = 1,
        EVENT_CONNECT        = 2,
        EVENT_DISCONNECT     = 3,
        EVENT_GENERIC        = 4,
        EVENT_FAILED         = 5,

        EVENT_COMMAND        = 6, //For Message Pushing to Server Processors

        //disonnect reason flags
        DISCONNECT_TIMEOUT   = 7,
        DISCONNECT_ERRORS    = 8,
        DISCONNECT_DDOS      = 9,
        DISCONNECT_FORCE     = 10
    };


    /** Type Definitions for LLP Functions **/
    typedef Socket Socket_t;


    /* DoS Wrapper for Returning  */
    template<typename NodeType>
    inline bool DoS(NodeType* pfrom, int nDoS, bool fReturn)
    {
        if(pfrom)
            pfrom->DDOS->rSCORE += nDoS;

        return fReturn;
    }


    /** Class that tracks DDOS attempts on LLP Servers.
        Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore] as a unit of Score / Second.
        Pointer stored by Connection class and Server Listener DDOS_MAP. **/
    class DDOS_Score
    {
        std::vector< std::pair<bool, int> > SCORE;
        Timer TIMER;
        int nIterator;
        Mutex_t MUTEX;


        /** Reset the Timer and the Score Flags to be Overwritten. **/
        void Reset()
        {
            for(int i = 0; i < SCORE.size(); i++)
                SCORE[i].first = false;

            TIMER.Reset();
            nIterator = 0;
        }

    public:

        /** Construct a DDOS Score of Moving Average Timespan. **/
        DDOS_Score(int nTimespan)
        {
            LOCK(MUTEX);

            for(int i = 0; i < nTimespan; i++)
                SCORE.push_back(std::make_pair(true, 0));

            TIMER.Start();
            nIterator = 0;
        }


        /** Flush the DDOS Score to 0. **/
        void Flush()
        {
            LOCK(MUTEX);

            Reset();
            for(int i = 0; i < SCORE.size(); i++)
                SCORE[i].second = 0;
        }


        /** Access the DDOS Score from the Moving Average. **/
        int Score()
        {
            LOCK(MUTEX);

            int nMovingAverage = 0;
            for(int i = 0; i < SCORE.size(); i++)
                nMovingAverage += SCORE[i].second;

            return nMovingAverage / SCORE.size();
        }


        /** Increase the Score by nScore. Operates on the Moving Average to Increment Score per Second. **/
        DDOS_Score & operator+=(const int& nScore)
        {
            LOCK(MUTEX);

            int nTime = TIMER.Elapsed();


            /** If the Time has been greater than Moving Average Timespan, Set to Add Score on Time Overlap. **/
            if(nTime >= SCORE.size())
            {
                Reset();
                nTime = nTime % SCORE.size();
            }


            /** Iterate as many seconds as needed, flagging that each has been iterated. **/
            for(int i = nIterator; i <= nTime; i++)
            {
                if(!SCORE[i].first)
                {
                    SCORE[i].first  = true;
                    SCORE[i].second = 0;
                }
            }


            /** Update the Moving Average Iterator and Score for that Second Instance. **/
            SCORE[nTime].second += nScore;
            nIterator = nTime;


            return *this;
        }
    };


    /** Filter to Contain DDOS Scores and Handle DDOS Bans. **/
    class DDOS_Filter
    {
        Timer TIMER;
        uint32_t BANTIME, TOTALBANS;

    public:
        DDOS_Score rSCORE, cSCORE;
        DDOS_Filter(uint32_t nTimespan) : BANTIME(0), TOTALBANS(0), rSCORE(nTimespan), cSCORE(nTimespan) { }
        Mutex_t MUTEX;

        /** Ban a Connection, and Flush its Scores. **/
        void Ban(std::string strViolation = "SCORE THRESHOLD")
        {
            LOCK(MUTEX);

            if((TIMER.Elapsed() < BANTIME))
                return;

            TIMER.Start();
            TOTALBANS++;

            BANTIME = std::max(TOTALBANS * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), TOTALBANS * 1200u);

            printf("XXXXX DDOS Filter cScore = %i rScore = %i Banned for %u Seconds. [VIOLATION: %s]\n", cSCORE.Score(), rSCORE.Score(), BANTIME, strViolation.c_str());

            cSCORE.Flush();
            rSCORE.Flush();
        }

        /** Check if Connection is Still Banned. **/
        bool Banned()
        {
            LOCK(MUTEX);

            uint32_t ELAPSED = TIMER.Elapsed();

            return (ELAPSED < BANTIME);

        }
    };

        /* Class to handle sending and receiving of LLP Packets. */
    class Packet
    {
    public:
        Packet() { SetNull(); }

        /* Components of an LLP Packet.
            BYTE 0       : Header
            BYTE 1 - 5   : Length
            BYTE 6 - End : Data      */
        uint8_t                 HEADER;
        uint32_t                LENGTH;
        std::vector<uint8_t>    DATA;


        /* Set the Packet Null Flags. */
        inline void SetNull()
        {
            HEADER   = 255;
            LENGTH   = 0;

            DATA.clear();
        }


        /* Packet Null Flag. Header = 255. */
        bool IsNull() { return (HEADER == 255); }


        /* Determine if a packet is fully read. */
        bool Complete() { return (Header() && DATA.size() == LENGTH); }


        /* Determine if header is fully read */
        bool Header() { return IsNull() ? false : (HEADER < 128 && LENGTH > 0) || (HEADER >= 128 && HEADER < 255 && LENGTH == 0); }


        /* Sets the size of the packet from Byte Vector. */
        void SetLength(std::vector<uint8_t> BYTES) { LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] ); }


        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes()
        {
            std::vector<uint8_t> BYTES(1, HEADER);

            /* Handle for Data Packets. */
            if(HEADER < 128)
            {
                BYTES.push_back((LENGTH >> 24)); BYTES.push_back((LENGTH >> 16));
                BYTES.push_back((LENGTH >> 8));  BYTES.push_back(LENGTH);

                BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
            }

            return BYTES;
        }
    };


    /* Base Template class to handle outgoing / incoming LLP data for both Client and Server. */
    template<typename PacketType = Packet> class BaseConnection
    {
    protected:

        /* Basic Connection Variables. */
        Timer         TIMER;
        Socket_t      SOCKET;
        Mutex_t       MUTEX;


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
            if(GetArg("-verbose", 0) >= 4)
                printf("***** Node Sent Message (%u, %u)\n", PACKET.LENGTH, PACKET.GetBytes().size());

            /* Debug dump of packet data. */
            if(GetArg("-verbose", 0) >= 5) {
                printf("***** Pakcet Dump: ");

                PrintHex(PACKET.GetBytes());
            }

            /* Write the packet to socket buffer. */
            Write(PACKET.GetBytes());

            /* Flush the rest of the buffer if any remains. */
            SOCKET.Flush();
        }


        /* Non-Blocking Packet reader to build a packet from TCP Connection.
            This keeps thread from spending too much time for each Connection. */
        virtual void ReadPacket() = 0;


        /* Connect Socket to a Remote Endpoint. */
        bool Connect(std::string strAddress, int nPort)
        {
            CService addrConnect(strprintf("%s:%i", strAddress.c_str(), nPort).c_str(), nPort);

            /// debug print
            printf("***** Node Connecting to %s\n",
                addrConnect.ToString().c_str());

            // Connect
            if (SOCKET.Connect(addrConnect))
            {
                /// debug print
                printf("***** Node Connected to %s\n", addrConnect.ToString().c_str());
                fCONNECTED = true;

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


    protected:


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
