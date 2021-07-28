/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/market.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Market::Initialize()
    {
        /* Standard contract to create new order. */
        mapFunctions["place/order"] = Function
        (
            std::bind
            (
                &Market::Place,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Standard contract to execute an order. */
        mapFunctions["execute/order"] = Function
        (
            std::bind
            (
                &Market::Execute,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
