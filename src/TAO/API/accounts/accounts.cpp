/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

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
    json::json Accounts::CreateAccount(const json::json& params, bool fHelp)
    {
        json::json ret;

        /* Check for username parameter. */
        if(params.find("username") == params.end())
            throw APIException(-23, "Missing Username");

        /* Check for password parameter. */
        if(params.find("password") == params.end())
            throw APIException(-24, "Missing Password");

        /* Check for pin parameter. */
        if(params.find("pin") == params.end())
            throw APIException(-25, "Missing PIN");

        /* Generate the signature chain. */
        TAO::Ledger::SignatureChain user(params["username"], params["password"]);

        /* Create the transaction object. */
        TAO::Ledger::Transaction tx;
        tx.NextHash(user.Generate(1, params["pin"]));
        tx.hashGenesis = tx.Genesis();

        /* Sign the transaction. */
        tx.Sign(user.Generate(0, params["pin"]));

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Invalid Transaction");

        /* Build a JSON response object. */
        ret["version"]   = tx.nVersion;
        ret["sequence"]  = tx.nSequence;
        ret["timestamp"] = tx.nTimestamp;
        ret["genesis"]   = tx.hashGenesis.ToString();
        ret["nexthash"]  = tx.hashNext.ToString();
        ret["prevhash"]  = tx.hashPrevTx.ToString();
        ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
        ret["hash"]      = tx.Genesis().ToString();

        return ret;
    }


    /* Get a user's account. */
    json::json Accounts::GetAccount(const json::json& params, bool fHelp)
    {
        json::json ret = {"test two"};

        return ret;
    }
}
