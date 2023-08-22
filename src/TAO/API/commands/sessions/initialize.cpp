/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/sessions.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Sessions::Initialize()
    {
        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Sessions::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all LIST operations. */
        if(config::GetBoolArg("-multiusername", false))
        {
            /* Only list sessions if -multiusername is supplied. */
            mapFunctions["list"] = Function
            (
                std::bind
                (
                    &Sessions::List,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
                , "local"
            );
        }

        /* Handle for all LOAD operations. */
        mapFunctions["load"] = Function
        (
            std::bind
            (
                &Sessions::Load,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all LOCK operations. */
        mapFunctions["lock"] = Function
        (
            std::bind
            (
                &Sessions::Lock,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all SAVE operations. */
        mapFunctions["save"] = Function
        (
            std::bind
            (
                &Sessions::Save,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all STATUS operations. */
        mapFunctions["status"] = Function
        (
            std::bind
            (
                &Sessions::Status,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all TERMINATE operations. */
        mapFunctions["terminate"] = Function
        (
            std::bind
            (
                &Sessions::Terminate,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );

        /* Handle for all UNLOCK operations. */
        mapFunctions["unlock"] = Function
        (
            std::bind
            (
                &Sessions::Unlock,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "local"
        );
    }
}
