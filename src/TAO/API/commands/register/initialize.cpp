/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/register.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/types/commands.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Register::Initialize()
    {
        /* Populate our ACCOUNT standard. */
        mapStandards["account"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::ACCOUNT;
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Register::AccountToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Populate our CRYPTO standard. */
        mapStandards["crypto"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::CRYPTO;
            }
        );

        /* Populate our NAME standard. */
        mapStandards["name"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::NAME;
            }
        );

        /* Populate our NAMESPACE standard. */
        mapStandards["namespace"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::NAMESPACE;
            }
        );

        /* Populate our OBJECT standard. */
        mapStandards["asset"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::NONSTANDARD;
            }
        );

        /* Populate our TOKEN standard. */
        mapStandards["token"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::TOKEN;
            }
        );

        /* Populate our TRUST standard. */
        mapStandards["trust"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::TRUST;
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Register::AccountToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Populate our APPEND standard. */
        mapStandards["append"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.nType == TAO::Register::REGISTER::APPEND;
            }
        );

        /* Populate our RAW standard. */
        mapStandards["raw"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.nType == TAO::Register::REGISTER::RAW;
            }
        );

        /* Populate our READONLY standard. */
        mapStandards["readonly"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.nType == TAO::Register::REGISTER::READONLY;
            }
        );


        /* Handle for generic get operations. */
        mapFunctions["get"] = Function
        (
            std::bind
            (
                &Templates::Get,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Register::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["history"] = Function
        (
            std::bind
            (
                &Templates::History,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Templates::Transactions,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
