/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/network.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Network::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Network::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "node"
        );
    }
}
