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
    /** List of accounts in API. **/
    Accounts accounts;


    /* Standard initialization function. */
    void Accounts::Initialize()
    {
        mapFunctions["create"]              = Function(std::bind(&Accounts::CreateAccount,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transactions"]        = Function(std::bind(&Accounts::GetTransactions, this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Create's a user account. */
    json::json Accounts::CreateAccount(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
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

        /* Get the Genesis ID. */
        uint256_t hashGenesis = LLC::SK256(user.Generate(0, params["pin"]).GetBytes());

        /* Check for duplicates in local db. */
        TAO::Ledger::Transaction tx;
        if(LLD::legDB->HasGenesis(hashGenesis))
            throw APIException(-26, "Account already exists");

        /* Create the transaction object. */
        tx.NextHash(user.Generate(tx.nSequence + 1, params["pin"]));
        tx.hashGenesis = hashGenesis;

        /* Sign the transaction. */
        tx.Sign(user.Generate(tx.nSequence, params["pin"]));

        /* Check that the transaction is valid. */
        if(!tx.IsValid())
            throw APIException(-26, "Invalid Transaction");

        /* Write transaction to ledger database. */
        LLD::legDB->WriteGenesis(hashGenesis, tx);
        LLD::legDB->WriteTx(tx.GetHash(), tx);

        /* Write last to local database. */
        LLD::locDB->WriteLast(hashGenesis, tx.GetHash());

        /* Build a JSON response object. */
        ret["version"]   = tx.nVersion;
        ret["sequence"]  = tx.nSequence;
        ret["timestamp"] = tx.nTimestamp;
        ret["genesis"]   = tx.hashGenesis.ToString();
        ret["nexthash"]  = tx.hashNext.ToString();
        ret["prevhash"]  = tx.hashPrevTx.ToString();
        ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
        ret["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
        ret["hash"]      = tx.GetHash().ToString();

        return ret;
    }


    /* Get a user's account. */
    json::json Accounts::GetTransactions(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
        json::json txList;

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

        /* Get the Genesis ID. */
        uint256_t hashGenesis = LLC::SK256(user.Generate(0, params["pin"]).GetBytes());

        /* Get the last transaction. */
        uint512_t hashLast;
        if(!LLD::locDB->ReadLast(hashGenesis, hashLast))
            throw APIException(-28, "No transactions found");

        /* Loop until genesis. */
        while(hashLast != 0)
        {
            TAO::Ledger::Transaction tx;
            if(!LLD::legDB->ReadTx(hashLast, tx))
                throw APIException(-28, "Failed to read transaction");

            json::json ret;
            ret["version"]   = tx.nVersion;
            ret["sequence"]  = tx.nSequence;
            ret["timestamp"] = tx.nTimestamp;
            ret["genesis"]   = tx.hashGenesis.ToString();
            ret["nexthash"]  = tx.hashNext.ToString();
            ret["prevhash"]  = tx.hashPrevTx.ToString();
            ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
            ret["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
            ret["hash"]      = tx.GetHash().ToString();

            txList.push_back(ret);

            hashLast = tx.hashPrevTx;
        }

        return txList;
    }
}
