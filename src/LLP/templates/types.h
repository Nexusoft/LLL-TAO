/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_TYPES_H
#define NEXUS_LLP_TEMPLATES_TYPES_H

#include <string>
#include <vector>
#include <stdio.h>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>

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
        
        EVENT_COMMAND        = 6 //For Message Pushing to Server Processors
    };


    /** Type Definitions for LLP Functions **/
    typedef boost::shared_ptr<boost::asio::ip::tcp::socket>      Socket_t;
    typedef boost::asio::ip::tcp::acceptor                       Listener_t;
    typedef boost::asio::io_service                              Service_t;
    typedef boost::system::error_code                            Error_t;
    
    
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
        unsigned int BANTIME, TOTALBANS;
        
    public:
        DDOS_Score rSCORE, cSCORE;
        DDOS_Filter(unsigned int nTimespan) : BANTIME(0), TOTALBANS(0), rSCORE(nTimespan), cSCORE(nTimespan) { }
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
            
            unsigned int ELAPSED = TIMER.Elapsed();
            
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
        unsigned char    HEADER;
        unsigned int     LENGTH;
        std::vector<unsigned char> DATA;
        
        
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
        void SetLength(std::vector<unsigned char> BYTES) { LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] ); }
        
        
        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<unsigned char> GetBytes()
        {
            std::vector<unsigned char> BYTES(1, HEADER);
            
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
        Error_t       ERROR_HANDLE;
        Socket_t      SOCKET;
        Mutex_t       MUTEX;

        
        /*  Pure Virtual Event Function to be Overridden allowing Custom Read Events. 
            Each event fired on Header Complete, and each time data is read to fill packet.
            Useful to check Header length to maximum size of packet type for DDOS protection, 
            sending a keep-alive ping while downloading large files, etc.
            
            LENGTH == 0: General Events
            LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
        */
        virtual void Event(unsigned char EVENT, unsigned int LENGTH = 0) = 0;
        
        
        /* Pure Virtual Process Function. To be overridden with your own custom packet processing. */
        virtual bool ProcessPacket() = 0;
    public:
        
    
        /* Incoming Packet Being Built. */
        PacketType        INCOMING;
        
        
        /* DDOS Score if a Incoming Server Connection. */
        DDOS_Filter*   DDOS;
        
        
        /* Connected Flag. **/
        bool fCONNECTED;
        
        
        /* Flag to Determine if DDOS is Enabled. */
        bool fDDOS;
        
        
        /* Flag to Determine if the connection was made by this Node. */
        bool fOUTGOING;
        
        
        /* Build Base Connection with no parameters */
        BaseConnection() : SOCKET(), INCOMING(), DDOS(NULL), fCONNECTED(false), fDDOS(false), fOUTGOING(false) { INCOMING.SetNull(); }
        
        
        /* Build Base Connection with all Parameters. */
        BaseConnection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false, bool fOutgoing = false) : SOCKET(SOCKET_IN), INCOMING(), DDOS(DDOS_IN), fCONNECTED(false), fDDOS(isDDOS),  fOUTGOING(fOutgoing) { TIMER.Start(); }
        
        virtual ~BaseConnection() { Disconnect(); }
        
        
        /* Checks for any flags in the Error Handle. */
        bool Errors(){ return (ERROR_HANDLE == boost::asio::error::eof || ERROR_HANDLE); }
                
                
        /* Determines if nTime seconds have elapsed since last Read / Write. */
        bool Timeout(unsigned int nTime){ return (TIMER.Elapsed() >= nTime); }
        
        
        /* Determines if Connected or Not. */
        bool Connected(){ return fCONNECTED; }
        
        
        /* Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. */
        bool PacketComplete(){ return INCOMING.Complete(); }
        
        
        /* Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. */
        void ResetPacket(){ INCOMING.SetNull(); }
        
        
        /* Write a single packet to the TCP stream. */
        void WritePacket(PacketType PACKET)
        { 
            LOCK(MUTEX);
            
            if(GetArg("-verbose", 0) >= 5)
                PrintHex(PACKET.GetBytes());
            
            else if(GetArg("-verbose", 0) >= 4)
                printf("***** Node Sent Message (%u, %u)\n", PACKET.LENGTH, PACKET.GetBytes().size());
                
            Write(PACKET.GetBytes());
        }
        
        
        /* Non-Blocking Packet reader to build a packet from TCP Connection.
            This keeps thread from spending too much time for each Connection. */
        virtual void ReadPacket() = 0;

        
        /* Connect Socket to a Remote Endpoint. */
        bool Connect(std::string strAddress, std::string strPort, Service_t& IO_SERVICE)
        {
            try
            {
                
                /* Establish the new Connection. */
                using boost::asio::ip::tcp;
                
                tcp::resolver					RESOLVER(IO_SERVICE);
                tcp::resolver::query			QUERY   (tcp::v4(), strAddress.c_str(), strPort.c_str());
                tcp::resolver::iterator			ADDRESS = RESOLVER.resolve(QUERY);
                
                SOCKET = Socket_t(new tcp::socket(IO_SERVICE));
                SOCKET -> connect(*ADDRESS, ERROR_HANDLE);
            
                /* Handle a Connection Error. */
                if(ERROR_HANDLE)
                    return error("Failed to Connect to %s:%s::%s", strAddress.c_str(), strPort.c_str(), ERROR_HANDLE.message().c_str());
                
                /* Set the object to be conneted if success. */
                fCONNECTED = true;
                
                return true;
            }
            catch(...)
            {
                return false;
            }
        }
        
        
        /* Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management. */
        void Disconnect()
        {
            if(!fCONNECTED)
                return;
                
            try
            {
                SOCKET -> shutdown(boost::asio::ip::tcp::socket::shutdown_both, ERROR_HANDLE);
                SOCKET -> close();
            }
            catch(...){}
            
            fCONNECTED = false;
        }

        std::string GetIPAddress() { return SOCKET->remote_endpoint().address().to_string(); }
        
        
        /* Helpful for debugging the code. */
        std::string ErrorMessage() 
        { 
            if(!Errors())
                return "N/A";
            
            return ERROR_HANDLE.message(); 
        }
        
        
    protected:
        
        
        /* Lower level network communications: Read. Interacts with OS sockets. */
        size_t Read(std::vector<unsigned char> &DATA, size_t nBytes) 
        {
            if(Errors())
                return 0;
            
            TIMER.Reset();
            
            return  boost::asio::read(*SOCKET, boost::asio::buffer(DATA, nBytes), ERROR_HANDLE); 
        }
        
        
        /* Lower level network communications: Write. Interacts with OS sockets. */
        void Write(std::vector<unsigned char> DATA) 
        { 
            if(Errors())
                return;
            
            TIMER.Reset();
            
            boost::asio::write(*SOCKET, boost::asio::buffer(DATA, DATA.size()), ERROR_HANDLE);
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
            if(SOCKET->available() > 0 && INCOMING.IsNull())
            {
                std::vector<unsigned char> HEADER(1, 255);
                if(Read(HEADER, 1) == 1)
                    INCOMING.HEADER = HEADER[0];
                    
            }
                
            if(!INCOMING.IsNull() && !INCOMING.Complete())
            {
                /* Handle Reading Packet Length Header. */
                if(SOCKET->available() >= 4 && INCOMING.LENGTH == 0)
                {
                    std::vector<unsigned char> BYTES(4, 0);
                    if(Read(BYTES, 4) == 4)
                    {
                        INCOMING.SetLength(BYTES);
                        Event(EVENT_HEADER);
                    }
                }
                    
                /* Handle Reading Packet Data. */
                unsigned int nAvailable = SOCKET->available();
                if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
                {
                    std::vector<unsigned char> DATA( std::min(nAvailable, (unsigned int)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);
                    unsigned int nRead = Read(DATA, DATA.size());
                    
                    if(nRead == DATA.size())
                    {
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                        Event(EVENT_PACKET, nRead);
                    }
                }
            }
        }	
    };
}

#endif
    
    
