/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/constants.h>

#include <TAO/API/include/check.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Finance::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Populate our ACCOUNT standard. */
        mapStandards["account"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::ACCOUNT;
            }
        );

        /* Populate our TOKEN standard. */
        mapStandards["token"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::TOKEN;
            }
        );

        /* Populate our TRUST standard. */
        mapStandards["trust"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::TRUST;
            }
        );

        /* Populate our ANY standard. */
        mapStandards["any"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check that base object is account. */
                if(rObject.Base() != TAO::Register::OBJECTS::ACCOUNT)
                    return false;

                /* Make sure not a token. */
                if(rObject.Standard() == TAO::Register::OBJECTS::TOKEN)
                    return false;

                return true;
            }
        );

        /* Populate our ALL standard. */
        mapStandards["all"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                /* Check that base object is account. */
                if(rObject.Base() != TAO::Register::OBJECTS::ACCOUNT)
                    return false;

                /* Make sure not a token. */
                if(rObject.Standard() == TAO::Register::OBJECTS::TOKEN)
                    return false;

                return true;
            }
        );

        /* Handle for all BURN operations. */
        mapFunctions["burn"] = Function
        (
            std::bind
            (
                &Finance::Burn,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "token"
        );

        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Templates::Create,
                std::placeholders::_1,
                std::placeholders::_2,

                "standard",
                USER_TYPES::STANDARD,
                "standard"
            )
        );

        /* Handle for all CREDIT operations. */
        mapFunctions["credit"] = Function
        (
            std::bind
            (
                &Finance::Credit,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all DEBIT operations. */
        mapFunctions["debit"] = Function
        (
            std::bind
            (
                &Finance::Debit,
                this,
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

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , "account, token, trust"
        );


        /* Handle for all LIST operations. */
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

        /* Handle for BALANCES. */
        mapFunctions["get/balances"] = Function
        (
            std::bind
            (
                &Finance::GetBalances,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* Handle for Stake Info. */
        mapFunctions["get/stakeinfo"] = Function
        (
            std::bind
            (
                &Finance::GetStakeInfo,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for migrate accounts. */
        mapFunctions["migrate/accounts"] = Function //XXX: we still need to clean this method up
        (
            std::bind
            (
                &Finance::MigrateAccounts,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for set/stake. */
        mapFunctions["set/stake"] = Function //XXX: we still need to clean this method up
        (
            std::bind
            (
                &Finance::SetStake,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for BALANCES. */
        mapFunctions["void/transaction"] = Function
        (
            std::bind
            (
                &Finance::Void,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );


        /* DEPRECATED */
        mapFunctions["list/balances"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/get/balances instead"
        );

        /* DEPRECATED */
        mapFunctions["list/trustaccounts"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use register/list/trust instead"
        );

        /* DEPRECATED */
        mapFunctions["list/account/transactions"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/transactions/account instead"
        );
    }
}
