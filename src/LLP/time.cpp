/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/types/time.h>
#include <LLP/include/trust_address.h>
#include <LLP/include/global.h>
#include <LLP/include/manager.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/convert.h>

/* The location of the unified time seed. To enable a Unified Time System push data to this variable. */
std::atomic<int32_t> UNIFIED_AVERAGE_OFFSET;

namespace LLP
{

    /** Mutex to lock time data adjustments. **/
    std::mutex TIME_MUTEX;


    /** Map to keep track of current node's time data samples from other nodes. **/
    std::map<std::string, int32_t> MAP_TIME_DATA;


    /** Constructor **/
    TimeNode::TimeNode()
    : Connection ( )
    , nSamples   ( )
    , nRequests  (0)
    {
    }


    /** Constructor **/
    TimeNode::TimeNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (SOCKET_IN, DDOS_IN, fDDOSIn)
    , nSamples   ( )
    , nRequests  (0)
    {
    }


    /** Constructor **/
    TimeNode::TimeNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (DDOS_IN, fDDOSIn)
    , nSamples   ( )
    , nRequests  (0)
    {
    }


    /* Virtual destructor. */
    TimeNode::~TimeNode()
    {
        nSamples.clear();
    }


    /* Virtual Functions to Determine Behavior of Message LLP. */
    void TimeNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        if(EVENT == EVENTS::HEADER)
        {
            /* Checks for incoming connections only. */
            if(fDDOS.load() && Incoming())
            {
                /* Get the incoming packet. */
                Packet PACKET   = this->INCOMING;

                /* Incoming connection should never send time data. */
                if(PACKET.HEADER == TIME_DATA)
                    DDOS->Ban("INVALID HEADER: TIME_DATA");

                /* Incoming connection should never receive address data. */
                if(PACKET.HEADER == ADDRESS_DATA)
                    DDOS->Ban("INVALID HEADER: ADDRESS_DATA");

                /* Check for incoming data fields that are unsolicited. */
                if(PACKET.HEADER == TIME_OFFSET)
                    DDOS->Ban("INVALID HEADER: TIME_OFFSET");

                /* Check for expected get time sizes. */
                if(PACKET.HEADER == GET_ADDRESS)
                    DDOS->Ban("INVALID HEADER: GET_ADDRESS");

                /* Check for expected get offset sizes. */
                if(PACKET.HEADER == GET_OFFSET && PACKET.LENGTH > 4)
                    DDOS->Ban("INVALID SIZE: GET_OFFSET");

                /* Check for expected get time sizes. */
                if(PACKET.HEADER == GET_TIME)
                    DDOS->Ban("INVALID PACKET: GET_TIME");

                /* Return if banned. */
                if(DDOS->Banned())
                    return;

            }
        }

        /* Handle for a Packet Data Read. */
        if(EVENT == EVENTS::PACKET)
            return;

        /* On Generic Event, Broadcast new block if flagged. */
        if(EVENT == EVENTS::GENERIC)
            return;

        /* On Connect Event, Assign the Proper Daemon Handle. */
        if(EVENT == EVENTS::CONNECT)
        {
            /* Only send time seed when outgoing connection. */
            if(!Incoming())
            {
                /* Request for a new sample. */
                GetSample();
            }
            else
            {
                LOCK(TIME_MUTEX);

                /* Check for time server that is still initializing. */
                if(MAP_TIME_DATA.size() <= (config::fTestNet.load() ? 0 : 1))
                {
                    debug::log(0, FUNCTION, "REJECT: no time samples available");

                    Disconnect();
                    return;
                }
            }
        }

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENTS::DISCONNECT)
        {
            /* Add address to time server as dropped. */
            if(!Incoming() && TIME_SERVER->GetAddressManager())
                TIME_SERVER->GetAddressManager()->AddAddress(GetAddress(), ConnectState::DROPPED);

            return;
        }
    }


    /* Main message handler once a packet is received. */
    bool TimeNode::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET   = INCOMING;

        /* Respond with an offset. */
        if(PACKET.HEADER == GET_OFFSET)
        {
            uint32_t nTimestamp  = convert::bytes2uint(PACKET.DATA);
            int32_t   nOffset    = (int32_t)(runtime::unifiedtimestamp() - nTimestamp);

            Packet RESPONSE;
            RESPONSE.HEADER = TIME_OFFSET;
            RESPONSE.LENGTH = 4;
            RESPONSE.DATA   = convert::int2bytes(nOffset);

            debug::log(4, NODE, "Sent offset ", nOffset);

            WritePacket(RESPONSE);
            return true;
        }

        /* Disable GET_ADDRESS for now. */
        if(PACKET.HEADER == GET_ADDRESS)
            return true;

        /* Add a New Sample each Time Packet Arrives. */
        if(PACKET.HEADER == TIME_OFFSET)
        {
            /* Make sure request counter hasn't exceeded requested offsets. */
            if(--nRequests < 0)
            {
                DDOS->Ban("UNSOLICITED TIME OFFSET");
                return false;
            }

            /* Calculate this particular sample. */
            int32_t nOffset = convert::bytes2int(PACKET.DATA);
            nSamples.push(nOffset);

            /* Verbose debug output. */
            debug::log(2, NODE, "Added Sample ", nOffset, " | Seed ", GetAddress().ToStringIP());

            /* Close the Connection Gracefully if Received all Packets. */
            if(nSamples.samples() >= 5)
            {
                {
                    LOCK(TIME_MUTEX);

                    /* Add new time data by IP to the time data map. */
                    MAP_TIME_DATA[GetAddress().ToStringIP()] = nSamples.top();
                }

                /* Set the Unified Average to the Majority Seed. */
                UNIFIED_AVERAGE_OFFSET.store(TimeNode::GetOffset());

                /* Log the debug output. */
                debug::log(0, NODE, MAP_TIME_DATA.size(),
                    " Total Samples | ", nSamples.top(),
                    " Offset | ", UNIFIED_AVERAGE_OFFSET.load(),
                    " Majority | ", runtime::unifiedtimestamp());

                /* Reset node's samples. */
                nSamples.clear();

                return false;
            }

            /* Get a new sample. */
            GetSample();

            return true;
        }

        return true;
    }


    /* Get a time sample from the time server. */
    void TimeNode::GetSample()
    {
        /* Reset the Timer and request another sample. */
        Packet REQUEST;
        REQUEST.HEADER = GET_OFFSET;
        REQUEST.LENGTH = 4;
        REQUEST.DATA = convert::uint2bytes(static_cast<uint32_t>(runtime::timestamp()));

        /* Write the packet to socket streams. */
        WritePacket(REQUEST);

        /* Increment packet counter. */
        ++nRequests;
    }


    /* Get the current time offset from the unified majority. */
    int32_t TimeNode::GetOffset()
    {
        /* Majority Object to check for consensus on time samples. */
        majority<int32_t> UNIFIED_MAJORITY;

        {
            LOCK(TIME_MUTEX);

            /* Iterate the Time Data map to find the majority time seed. */
            for(auto it = MAP_TIME_DATA.begin(); it != MAP_TIME_DATA.end(); ++it)
            {
                /* Update the Unified Majority. */
                UNIFIED_MAJORITY.push(it->second);
            }
        }

        return UNIFIED_MAJORITY.top();
    }
}
