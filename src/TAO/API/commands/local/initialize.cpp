/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/local.h>
#include <TAO/API/types/operators/initialize.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Local::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Handle for all ERASE operations. */
        mapFunctions["erase"] = Function
        (
            std::bind
            (
                &Local::Erase,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "record"
        );


        /* Handle for all GET operations. */
        mapFunctions["has"] = Function
        (
            std::bind
            (
                &Local::Has,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "record"
        );

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Local::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "record"
        );

        /* Handle for all PUSH operations. */
        mapFunctions["push"] = Function
        (
            std::bind
            (
                &Local::Push,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "record"
        );
    }
}
