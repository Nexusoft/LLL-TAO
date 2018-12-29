/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

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

        /* Create the sigchain. */
        TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain(params["username"].get<std::string>().c_str(), params["password"].get<std::string>().c_str());

        /* Get the genesis ID. */
        uint256_t hashGenesis = user->Genesis();

        /* Check for duplicates in ledger db. */
        TAO::Ledger::Transaction tx;
        if(!LLD::legDB->HasGenesis(hashGenesis))
        {
            delete user;
            user = nullptr;

            throw APIException(-26, "Account doesn't exists");
        }

        /* Check the sessions. */
        for(auto session = mapSessions.begin(); session != mapSessions.end(); ++ session)
        {
            if(hashGenesis == session->second->Genesis())
            {
                delete user;
                user = nullptr;

                throw APIException(-22, "Already logged in.");
            }
        }

        /* Set the return value. */
        uint64_t nSession = LLC::GetRand();
        ret["genesis"] = hashGenesis.ToString();
        ret["session"] = nSession;

        /* Setup the account. */
        mapSessions[nSession] = user;

        return ret;
    }


    /* Login to a user account. */
    json::json Accounts::Logout(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
        json::json ret;

        /* Check for username parameter. */
        if(params.find("session") == params.end())
            throw APIException(-23, "Missing Session ID");

        /* Generate the signature chain. */
        uint64_t nSession = std::stoull(params["session"].get<std::string>());
        if(!mapSessions.count(nSession))
            throw APIException(-1, "Already logged out");

        /* Set the return value. */
        ret["genesis"] = GetGenesis(nSession).ToString();

        /* Delete the sigchan. */
        TAO::Ledger::SignatureChain* user = mapSessions[nSession];

        delete user;
        user = nullptr;

        /* Erase the session. */
        mapSessions.erase(nSession);

        return ret;
    }
}
