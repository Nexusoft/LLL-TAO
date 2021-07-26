/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/supply.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Supply::Initialize()
    {
        /* Populate our item standard. */
        mapStandards["item"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(rObject.nType != TAO::Register::REGISTER::OBJECT)
                    return false;

                /* Make sure this isn't a command-set standard. */
                if(!rObject.Check("_usertype", TAO::Register::TYPES::UINT16_T, false))
                    return false;

                /* Check that this is for this command-set. */
                if(rObject.get<uint16_t>("_usertype") != USER_TYPES::SUPPLY)
                    return false;

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
                return GetStandardType(rObject) == USER_TYPES::SUPPLY;
            }
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
                return GetStandardType(rObject) == USER_TYPES::SUPPLY;
            }
        );

        /* Populate our any standard. */
        mapStandards["any"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& rObject)
            {
                /* Check for correct state type. */
                if(CheckObject("item", rObject))
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
                USER_TYPES::SUPPLY //the enumerated value for states
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
                USER_TYPES::SUPPLY //the enumerated value for states
            )
        );
    }
}
