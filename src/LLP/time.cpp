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
        return;
    }

    /* Main message handler once a packet is recieved. */
    bool TimeNode::ProcessPacket()
    {

        return true;
    }

}
