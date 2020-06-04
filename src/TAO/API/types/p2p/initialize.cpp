/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/p2p.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        
        

        /* Standard initialization function. */
        void P2P::Initialize()
        {
            mapFunctions["list/connections"]        = Function(std::bind(&P2P::List, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/requests"]           = Function(std::bind(&P2P::ListRequests, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/connection"]          = Function(std::bind(&P2P::Get, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["request/connection"]      = Function(std::bind(&P2P::Request, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["accept/connection"]       = Function(std::bind(&P2P::Accept, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["ignore/connection"]       = Function(std::bind(&P2P::Ignore,this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["terminate/connection"]    = Function(std::bind(&P2P::Terminate,this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["send/message"]            = Function(std::bind(&P2P::Send,this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["peek/message"]            = Function(std::bind(&P2P::Peek,this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["pop/message"]             = Function(std::bind(&P2P::Pop,this, std::placeholders::_1, std::placeholders::_2));
            
        }
    }
}
