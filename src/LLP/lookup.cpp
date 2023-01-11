/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/types/lookup.h>
#include <LLP/templates/events.h>

namespace LLP
{

    /** Constructor **/
    LookupNode::LookupNode()
    : Connection ( )
    {
    }


    /** Constructor **/
    LookupNode::LookupNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (SOCKET_IN, DDOS_IN, fDDOSIn)
    {
    }


    /** Constructor **/
    LookupNode::LookupNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection (DDOS_IN, fDDOSIn)
    {
    }


    /* Virtual destructor. */
    LookupNode::~LookupNode()
    {
    }


    /* Virtual Functions to Determine Behavior of Message LLP. */
    void LookupNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        if(EVENT == EVENTS::HEADER)
        {
            /* Checks for incoming connections only. */
            if(fDDOS.load() && Incoming())
            {
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
            return;

        /* On Disconnect Event, Reduce the Connection Count for Daemon */
        if(EVENT == EVENTS::DISCONNECT)
            return;
    }


    /* Main message handler once a packet is recieved. */
    bool LookupNode::ProcessPacket()
    {
        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.HEADER)
        {

        }

        return true;
    }
}
