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
        /* Populate our BID standard. */
        mapStandards["bid"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );

        /* Populate our ASK standard. */
        mapStandards["ask"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );

        /* Populate our ORDER standard. */
        mapStandards["order"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );

        /* Populate our EXECUTED standard. */
        mapStandards["executed"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return false;
            }
        );


        /* Standard contract to create new order. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Market::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Standard contract to create new order. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Market::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Standard contract to execute an order. */
        mapFunctions["execute"] = Function
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
