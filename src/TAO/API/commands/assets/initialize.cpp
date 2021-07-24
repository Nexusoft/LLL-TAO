/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/assets.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/check.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Assets::Initialize()
    {
        /* Populate our asset standard. */
        mapStandards["asset"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(rObject.nType != TAO::Register::REGISTER::OBJECT)
                    return false;

                /* Make sure this isn't a command-set standard. */
                if(rObject.Check("_commands", TAO::Operation::OP::TYPES::STRING, false))
                    return false;

                return rObject.Standard() == TAO::Register::OBJECTS::NONSTANDARD;
            }
        );

        /* Populate our schema standard. */
        mapStandards["schema"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                return CheckObject("asset", rObject);
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Assets::SchemaToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* Handle for all CLAIM operations. */
        mapFunctions["claim"] = Function
        (
            std::bind
            (
                &Templates::Claim,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all GET operations. */
        mapFunctions["get"] = Function
        (
            std::bind
            (
                &Templates::Get,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all HISTORY operations. */
        mapFunctions["history"] = Function
        (
            std::bind
            (
                &Templates::History,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all TRANSACTIONS operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Templates::Transactions,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all TRANSFER operations. */
        mapFunctions["transfer"] = Function
        (
            std::bind
            (
                &Templates::Transfer,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );




        mapFunctions["create/asset"]             = Function(std::bind(&Assets::Create,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["update/asset"]             = Function(std::bind(&Assets::Update,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["tokenize/asset"]           = Function(std::bind(&Assets::Tokenize,  this, std::placeholders::_1, std::placeholders::_2));


        /* DEPRECATED */
        mapFunctions["list/asset/history"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/transactions/account command instead"
        );
    }
}
