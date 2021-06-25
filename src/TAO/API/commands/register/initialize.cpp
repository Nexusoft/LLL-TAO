/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/register.h>
#include <TAO/API/objects/types/objects.h>

#include <TAO/API/types/commands.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Register::Initialize()
    {
        /* Populate our standard object registers. */
        mapStandards["account"]   = TAO::Register::OBJECTS::ACCOUNT;
        mapStandards["crypto"]    = TAO::Register::OBJECTS::CRYPTO;
        mapStandards["name"]      = TAO::Register::OBJECTS::NAME;
        mapStandards["namespace"] = TAO::Register::OBJECTS::NAMESPACE;
        mapStandards["object"]    = TAO::Register::OBJECTS::NONSTANDARD;
        mapStandards["token"]     = TAO::Register::OBJECTS::TOKEN;
        mapStandards["trust"]     = TAO::Register::OBJECTS::TRUST;

        /* Populate our standard state registers. */
        mapStandards["append"]    = TAO::Register::REGISTER::APPEND;
        mapStandards["raw"]       = TAO::Register::REGISTER::RAW;
        mapStandards["readonly"]  = TAO::Register::REGISTER::READONLY;


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
        mapFunctions["list/transactions"] = Function
        (
            std::bind
            (
                &Register::Transactions,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
