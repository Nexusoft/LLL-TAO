/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/accounts.h>

namespace TAO::API
{
    /** List of accounts in API. **/
    Accounts accounts;


    /* Standard initialization function. */
    void Accounts::Initialize()
    {
        mapFunctions["create"]              = Function(std::bind(&Accounts::CreateAccount,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getid"]               = Function(std::bind(&Accounts::GetAccount,      this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Create's a user account. */
    json::json Accounts::CreateAccount(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = { { "genesis", "hash" } };

        return ret;
    }


    /* Get a user's account. */
    json::json Accounts::GetAccount(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = {"test two"};

        return ret;
    }
}
