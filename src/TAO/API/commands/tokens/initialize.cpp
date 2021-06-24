/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/tokens.h>
#include <TAO/API/types/commands/finance.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

#include <TAO/Register/include/enum.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Tokens::Initialize()
    {
        /* Populate our standard objects. */
        mapStandards["account"] = TAO::Register::OBJECTS::ACCOUNT;
        mapStandards["token"]   = TAO::Register::OBJECTS::TOKEN;


        /* Handle for all BURN operations. */
        mapFunctions["burn"] = Function
        (
            std::bind
            (
                &Finance::Burn,
                Commands::Get<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Finance::Create,
                Commands::Get<Finance>(),
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
                Commands::Get<Finance>(),
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
                Commands::Get<Finance>(),
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
                Commands::Get<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(3, 0)
            , version::get_version(5, 2)
            , "please use finance/get command instead"
        );


        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Finance::List,
                Commands::Get<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
        );



        //XXX: we should format this better, we can't go over 132 characters in a line based on formatting guidelines.
        mapFunctions["list/token/transactions"]   = Function(std::bind(&Tokens::ListTransactions,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/token/accounts"]       = Function(std::bind(&Tokens::ListTokenAccounts, this, std::placeholders::_1, std::placeholders::_2));

        /* Temporary reroute of the account methods to the finance API equivalents XXX: this is really hacky */
        mapFunctions["list/account/transactions"] = Function(std::bind(&Finance::ListTransactions, Commands::Get<Finance>(), std::placeholders::_1, std::placeholders::_2));
    }
}
