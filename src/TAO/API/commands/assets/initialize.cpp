/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/assets.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Assets::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


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
                if(rObject.Check("_usertype", TAO::Register::TYPES::UINT16_T, false))
                {
                    /* Check for user-types if specified. */
                    if(rObject.get<uint16_t>("_usertype") != USER_TYPES::ASSET) //this is the new check
                        return false;
                } //we put this check inside fieldname check for backwards compatability

                return rObject.Standard() == TAO::Register::OBJECTS::NONSTANDARD;
            }
        );

        /* Populate our raw standard. */
        mapStandards["raw"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(rObject.nType != TAO::Register::REGISTER::RAW)
                    return false;

                /* Check that this matches our user type. */
                return GetStandardType(rObject) == USER_TYPES::ASSET;
            }
            , "raw"
        );

        /* Populate our readonly standard. */
        mapStandards["readonly"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(rObject.nType != TAO::Register::REGISTER::READONLY)
                    return false;

                /* Check that this matches our user type. */
                return GetStandardType(rObject) == USER_TYPES::ASSET;
            }
            , "readonly"
        );

        /* Populate our any standard. */
        mapStandards["any"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(CheckObject("asset", rObject))
                    return true;

                /* Check for correct state type. */
                if(CheckObject("raw", rObject))
                    return true;

                /* Check for correct state type. */
                if(CheckObject("readonly", rObject))
                    return true;

                return false;
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

        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Templates::Create,
                std::placeholders::_1,
                std::placeholders::_2,

                /* Our accepted formats for this command-set. */
                "readonly, raw, basic, json",
                USER_TYPES::ASSET, //the enumerated value for states
                "required"         //the default format
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

        /* Handle for all LIST operations with noun PARTIAL. */
        mapFunctions["list/partial"] = Function
        (
            std::bind
            (
                &Assets::Partial,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all VERIFY operations with noun PARTIAL. */
        mapFunctions["verify/partial"] = Function
        (
            std::bind
            (
                &Assets::Verify,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all TOKENIZE operations. */
        mapFunctions["tokenize"] = Function
        (
            std::bind
            (
                &Assets::Tokenize,
                this,
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

        /* Handle for all UPDATE operations. */
        mapFunctions["update"] = Function
        (
            std::bind
            (
                &Templates::Update,
                std::placeholders::_1,
                std::placeholders::_2,

                /* Our accepted formats for this command-set. */
                "raw, basic, json",
                USER_TYPES::ASSET, //the enumerated value for states
                "required"         //the default format
            )
        );


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
            , "please use assets/history/asset command instead"
        );
    }
}
