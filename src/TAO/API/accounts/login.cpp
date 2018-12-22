/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

namespace TAO::API
{

    //TODO: have the authorization system build a SHA256 hash and salt on the client side as the AUTH hash.

    /* Login to a user account. */
    json::json Accounts::Login(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
        json::json ret;

        /* Check for username parameter. */
        if(params.find("username") == params.end())
            throw APIException(-23, "Missing Username");

        /* Check for password parameter. */
        if(params.find("password") == params.end())
            throw APIException(-24, "Missing Password");

        /* Generate the signature chain. */
        if(user)
            throw APIException(-1, "Already logged in");

        /* Create the sigchain. */
        user = new TAO::Ledger::SignatureChain(params["username"], params["password"]);

        /* Check for duplicates in ledger db. */
        TAO::Ledger::Transaction tx;
        if(!LLD::legDB->HasGenesis(GetGenesis()))
        {
            delete user;
            user = nullptr;

            throw APIException(-26, "Account doesn't exists");
        }

        /* Set the return value. */
        ret["genesis"] = GetGenesis().ToString();

        return ret;
    }


    /* Login to a user account. */
    json::json Accounts::Logout(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
        json::json ret;

        /* Generate the signature chain. */
        if(!user)
            throw APIException(-1, "Already logged out");

        /* Set the return value. */
        ret["genesis"] = GetGenesis().ToString();

        /* Delete the sigchan. */
        delete user;
        user = nullptr;

        return ret;
    }
}
