/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>

#include <TAO/API/include/check.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Finance::Initialize()
    {
        /* Populate our ACCOUNT standard. */
        mapStandards["account"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                return objCheck.Standard() == TAO::Register::OBJECTS::ACCOUNT;
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
        );


        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Finance::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
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
                &Finance::Get,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Finance::List,
                this,
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


        /* DEPRECATED */
        mapFunctions["list/balances"] = Function
        (
            std::bind
            (
                &Finance::Deprecated,
                this,
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
                &Finance::Deprecated,
                this,
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
                &Finance::Deprecated,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use register/list/transactions instead"
        );
    }
}
