/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/profiles.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/types/operators/initialize.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Profiles::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Populate our any standard. */
        mapStandards["register"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                return true;
            }
        );


        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Profiles::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "master, auth"
        );

        /* Handle for all transactions operations. */
        mapFunctions["notifications"] = Function
        (
            std::bind
            (
                &Profiles::Notifications,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "master"
        );


        /* Handle for all transactions operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "register"
        );

        /* Handle for all RECOVER operations. */
        mapFunctions["recover"] = Function
        (
            std::bind
            (
                &Profiles::Recover,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "master"
        );

        /* Handle for all transactions operations. */
        mapFunctions["status"] = Function
        (
            std::bind
            (
                &Profiles::Status,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "master"
        );

        /* Handle for all transactions operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Profiles::Transactions,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "master"
        );

        /* Handle for all update operations. */
        mapFunctions["update"] = Function
        (
            std::bind
            (
                &Profiles::Update,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "credentials, recovery"
        );
    }
}
