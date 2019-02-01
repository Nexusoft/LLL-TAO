/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/types/time.h>

namespace LLP
{

    /* Virtual Functions to Determine Behavior of Message LLP. */
    void TimeNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /** Handle any DDOS Packet Filters. **/
        if(EVENT == EVENT_HEADER)
        {
            if(fDDOS)
            {
                Packet PACKET   = this->INCOMING;

                if(PACKET.HEADER == TIME_DATA)
                    DDOS->Ban();

                if(PACKET.HEADER == ADDRESS_DATA)
                    DDOS->Ban();

                if(PACKET.HEADER == TIME_OFFSET)
                    DDOS->Ban();

                if(PACKET.HEADER == GET_OFFSET && PACKET.LENGTH > 4)
                    DDOS->Ban();

                if(DDOS->Banned())
                    return;

            }
        }


        /** Handle for a Packet Data Read. **/
        if(EVENT == EVENT_PACKET)
            return;


        /** On Generic Event, Broadcast new block if flagged. **/
        if(EVENT == EVENT_GENERIC)
            return;

        /** On Connect Event, Assign the Proper Daemon Handle. **/
        if(EVENT == EVENT_CONNECT)
            return;

        /** On Disconnect Event, Reduce the Connection Count for Daemon **/
        if(EVENT == EVENT_DISCONNECT)
            return;

    }


    /* Main message handler once a packet is recieved. */
    bool TimeNode::ProcessPacket()
    {

        return true;
    }

}
