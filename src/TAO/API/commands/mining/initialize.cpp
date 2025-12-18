/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/mining.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Mining::Initialize()
    {
        /* Handle for list/pools */
        mapFunctions["list/pools"] = Function
        (
            std::bind
            (
                &Mining::ListPools,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for get/poolstats */
        mapFunctions["get/poolstats"] = Function
        (
            std::bind
            (
                &Mining::GetPoolStats,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for start/pool */
        mapFunctions["start/pool"] = Function
        (
            std::bind
            (
                &Mining::StartPool,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for stop/pool */
        mapFunctions["stop/pool"] = Function
        (
            std::bind
            (
                &Mining::StopPool,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for get/connectedminers */
        mapFunctions["get/connectedminers"] = Function
        (
            std::bind
            (
                &Mining::GetConnectedMiners,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
