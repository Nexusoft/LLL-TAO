/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/core.h>

namespace TAO
{
    namespace API
    {
        /* Custom Events for Core API */
        void Core::Event(uint8_t EVENT, uint32_t LENGTH)
        {
            //no events for now
            //TODO: see if at all possible to call from down in inheritance heirarchy
        }


        /** Main message handler once a packet is recieved. **/
        bool Core::ProcessPacket()
        {
            printf("Received HTTP Request: %s::%s\n", INCOMING.strRequest.c_str(), INCOMING.strContent.c_str());

            PushResponse(200, "CONTENT:::" + INCOMING.strContent + "\n\nThis would be test content!");

            return true;
        }

    }

}
