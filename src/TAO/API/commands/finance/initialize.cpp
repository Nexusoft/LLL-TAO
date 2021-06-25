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
        /* Populate our standard objects. */
        mapStandards["account"] = TAO::Register::OBJECTS::ACCOUNT;
        mapStandards["token"]   = TAO::Register::OBJECTS::TOKEN;


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

        mapFunctions["list/account/transactions"]  = Function(std::bind(&Finance::ListTransactions, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["migrate/accounts"]    = Function(std::bind(&Finance::MigrateAccounts, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["set/stake"]       = Function(std::bind(&Finance::SetStake, this, std::placeholders::_1, std::placeholders::_2));

        //mapFunctions["list/trustaccounts"] = Function(std::bind(&Finance::TrustAccounts, this, std::placeholders::_1, std::placeholders::_2));

        /* DEPRECATED */
        mapFunctions["list/balances"] = Function
        (
            std::bind
            (
                &Finance::List,
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
                &Finance::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use register/list/trust instead"
        );
    }
}
