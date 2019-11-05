/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/types/time.h>

#include <Util/include/convert.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

/* The location of the unified time seed. To enable a Unified Time System push data to this variable. */
std::atomic<int32_t> UNIFIED_AVERAGE_OFFSET;

namespace LLP
{
    std::map<std::string, int32_t> MAP_TIME_DATA;


    /** Constructor **/
    TimeNode::TimeNode()
    : Connection()
    , nSamples()
    {
    }


    /** Constructor **/
    TimeNode::TimeNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(SOCKET_IN, DDOS_IN, isDDOS)
    , nSamples()
    {
    }


    /** Constructor **/
    TimeNode::TimeNode(DDOS_Filter* DDOS_IN, bool isDDOS)
    : Connection(DDOS_IN, isDDOS)
    , nSamples()
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
        if(EVENT == EVENT_HEADER)
        {
            /* Checks for incoming connections only. */
            if(fDDOS && Incoming())
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
                if(PACKET.HEADER == GET_TIME && PACKET.LENGTH > 4)
                    DDOS->Ban("INVALID SIZE: GET_TIME");

                /* Return if banned. */
                if(DDOS->Banned())
                    return;

            }
        }

        /* Handle for a Packet Data Read. */
        if(EVENT == EVENT_PACKET)
            return;

        /* On Generic Event, Broadcast new block if flagged. */
        if(EVENT == EVENT_GENERIC)
            return;

        /* On Connect Event, Assign the Proper Daemon Handle. */
        if(EVENT == EVENT_CONNECT)
        {
            /* Only send time seed when outgoing connection. */
            if(fOUTGOING)
            {
                /* Reset the Timer and request another sample. */
                Packet REQUEST;
                REQUEST.HEADER = GET_OFFSET;
                REQUEST.LENGTH = 4;
                REQUEST.DATA = convert::uint2bytes(static_cast<uint32_t>(runtime::timestamp()));

                WritePacket(REQUEST);
            }
        }

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENT_DISCONNECT)
            return;

    }


    /* Main message handler once a packet is recieved. */
    bool TimeNode::ProcessPacket()
    {
        /* Get the incoming packet. */
        Packet PACKET   = INCOMING;

        /* Rspond with an offset. */
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

        if(PACKET.HEADER == GET_TIME)
        {
            uint32_t nTimestamp = static_cast<uint32_t>(runtime::unifiedtimestamp());

            Packet RESPONSE;
            RESPONSE.HEADER = TIME_DATA;
            RESPONSE.LENGTH = 4;
            RESPONSE.DATA = convert::uint2bytes(nTimestamp);

            debug::log(4, NODE, "Sent time sample ", nTimestamp);

            WritePacket(RESPONSE);
            return true;
        }

        if(PACKET.HEADER == GET_ADDRESS)
            return true;

        /* Add a New Sample each Time Packet Arrives. */
        if(PACKET.HEADER == TIME_OFFSET)
        {
            /* Calculate this particular sample. */
            int32_t nOffset = convert::bytes2int(PACKET.DATA);
            nSamples.Add(nOffset);

            /* Reset the Timer and request another sample. */
            Packet REQUEST;
            REQUEST.HEADER = GET_OFFSET;
            REQUEST.LENGTH = 4;
            REQUEST.DATA = convert::uint2bytes(static_cast<uint32_t>(runtime::timestamp()));

            /* Verbose debug output. */
            debug::log(2, NODE, "Added Sample ", nOffset, " | Seed ", GetAddress().ToStringIP());

            /* Close the Connection Gracefully if Received all Packets. */
            if(nSamples.Samples() >= 5)
            {
                MAP_TIME_DATA[GetAddress().ToStringIP()] = nSamples.Majority();

                /* Update Iterators and Flags. */
                if((MAP_TIME_DATA.size() > 0))
                {
                    /* Majority Object to check for consensus on time samples. */
                    CMajority<int32_t> UNIFIED_MAJORITY;

                    /* Info tracker to see the total samples. */
                    std::map<int32_t, uint32_t> TOTAL_SAMPLES;

                    /* Iterate the Time Data map to find the majority time seed. */
                    for(auto it=MAP_TIME_DATA.begin(); it != MAP_TIME_DATA.end(); ++it)
                    {
                        /* Update the Unified Majority. */
                        UNIFIED_MAJORITY.Add(it->second);

                        /* Increase the count per samples (for debugging only). */
                        if(!TOTAL_SAMPLES.count(it->second))
                            TOTAL_SAMPLES[it->second] = 1;
                        else
                            TOTAL_SAMPLES[it->second] ++;
                    }

                    /* Set the Unified Average to the Majority Seed. */
                    UNIFIED_AVERAGE_OFFSET.store(UNIFIED_MAJORITY.Majority());

                    /* Log the debug output. */
                    debug::log(0, NODE, MAP_TIME_DATA.size(), " Total Samples | ", nSamples.Majority(), " Offset (", TOTAL_SAMPLES[nSamples.Majority()], ") | ", UNIFIED_AVERAGE_OFFSET.load(), " Majority (", TOTAL_SAMPLES[UNIFIED_AVERAGE_OFFSET.load()], ") | ", runtime::unifiedtimestamp());
                }

                nSamples.clear();

                return false;
            }

            WritePacket(REQUEST);

            return true;
        }

        return true;
    }

}
