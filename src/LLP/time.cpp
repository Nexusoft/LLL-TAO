/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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


    /* Declare the static thread. */
    std::thread TimeNode::TIME_ADJUSTMENT;


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
                if(PACKET.HEADER == GET_TIME)
                    DDOS->Ban("INVALID PACKET: GET_TIME");

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
                /* Request for a new sample. */
                GetSample();
            }
        }

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENT_DISCONNECT)
        {
            if(TIME_SERVER->pAddressManager)
                TIME_SERVER->pAddressManager->AddAddress(GetAddress(), ConnectState::DROPPED);

            return;
        }
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

        /* Disable GET_ADDRESS for now. */
        if(PACKET.HEADER == GET_ADDRESS)
            return true;

        /* Add a New Sample each Time Packet Arrives. */
        if(PACKET.HEADER == TIME_OFFSET)
        {
            /* Make sure request counter hasn't exceeded requested offsets. */
            if(--nRequests < 0)
            {
                DDOS->Ban("UNSOLICTED TIME OFFSET");
                return false;
            }

            /* Calculate this particular sample. */
            int32_t nOffset = convert::bytes2int(PACKET.DATA);
            nSamples.Add(nOffset);

            /* Verbose debug output. */
            debug::log(2, NODE, "Added Sample ", nOffset, " | Seed ", GetAddress().ToStringIP());

            /* Close the Connection Gracefully if Received all Packets. */
            if(nSamples.Samples() >= 5)
            {
                {
                    LOCK(TIME_MUTEX);

                    /* Add new time data by IP to the time data map. */
                    MAP_TIME_DATA[GetAddress().ToStringIP()] = nSamples.Majority();
                }

                /* Set the Unified Average to the Majority Seed. */
                UNIFIED_AVERAGE_OFFSET.store(TimeNode::GetOffset());

                /* Log the debug output. */
                debug::log(0, NODE, MAP_TIME_DATA.size(),
                    " Total Samples | ", nSamples.Majority(),
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
        CMajority<int32_t> UNIFIED_MAJORITY;

        {
            LOCK(TIME_MUTEX);

            /* Iterate the Time Data map to find the majority time seed. */
            for(auto it = MAP_TIME_DATA.begin(); it != MAP_TIME_DATA.end(); ++it)
            {
                /* Update the Unified Majority. */
                UNIFIED_MAJORITY.Add(it->second);
            }
        }

        return UNIFIED_MAJORITY.Majority();
    }


    /* This thread is responsible for unified time offset adjustments. This will be deleted on post v8 updates. */
    const uint32_t TIME_ADJUSTMENT_VALUE = 299;
    void TimeNode::AdjustmentThread()
    {
        /* Switch for different time adjustment intervals. */
        static const uint32_t TIME_ADJUSTMENT_SPAN = (config::fTestNet ? 60 : 600);

        /* Check for the current adjustment period. */
        uint32_t nCurrentAdjustment = (TAO::Ledger::BlockVersionActive(runtime::unifiedtimestamp(), 8) ?
            ((runtime::unifiedtimestamp() - TAO::Ledger::CurrentBlockTimelock()) / TIME_ADJUSTMENT_SPAN) : 0);

        /* Check for valid adjustments. */
        if(nCurrentAdjustment > TIME_ADJUSTMENT_VALUE)
        {
            debug::log(0, "Time Adjustment Complete... Exiting...");
            return;
        }

        /* Loop to check for time changes. */
        while(!config::fShutdown)
        {
            /* Sleep for 1 second at a time. */
            runtime::sleep(1000);

            /* Check current unified timestamp. */
            if(runtime::unifiedtimestamp() < TAO::Ledger::StartBlockTimelock(8))
                continue;

            /* Grab the current time-lock. */
            uint32_t nElapsed   = (runtime::unifiedtimestamp() - TAO::Ledger::CurrentBlockTimelock()) / TIME_ADJUSTMENT_SPAN;
            if(nElapsed > nCurrentAdjustment)
            {
                /* This should adjust by one second at a time, but we must be prepared to handle things if this assumption fails. */
                uint32_t nAdjustment = (std::min(TIME_ADJUSTMENT_VALUE, nElapsed) - nCurrentAdjustment);

                {
                    LOCK(TIME_MUTEX);

                    /* Iterate the Time Data map to find the majority time seed. */
                    for(auto it = MAP_TIME_DATA.begin(); it != MAP_TIME_DATA.end(); ++it)
                        it->second += nAdjustment;
                }

                /* Recalculate the unified offset. */
                UNIFIED_AVERAGE_OFFSET.store(TimeNode::GetOffset());

                /* Increment adjustment to current period. */
                nCurrentAdjustment += nAdjustment;

                /* Log the debug output. */
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, MAP_TIME_DATA.size(),
                    " Offsets | ", UNIFIED_AVERAGE_OFFSET.load(),
                    " Majority | ", runtime::unifiedtimestamp(),
                    " ADJUSTMENT OF +", nAdjustment, " Seconds (", nCurrentAdjustment, "/", TIME_ADJUSTMENT_VALUE, ")", ANSI_COLOR_RESET);
            }

            /* Check that the adjustment is complete. */
            if(nCurrentAdjustment > TIME_ADJUSTMENT_VALUE)
            {
                debug::log(0, "Time Adjustment Complete... Exiting...");
                return;
            }
        }
    }
}
