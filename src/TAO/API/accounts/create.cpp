/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

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
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(params["username"].get<std::string>().c_str(), params["password"].get<std::string>().c_str());

            /* Get the Genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction tx;
            if(LLD::legDB->HasGenesis(hashGenesis))
            {
                user.free();
                throw APIException(-26, "Account already exists");
            }

            /* Create the transaction. */
            if(!TAO::Ledger::CreateTransaction(user, params["pin"].get<std::string>().c_str(), tx))
            {
                user.free();
                throw APIException(-25, "Failed to create transaction");
            }

            /* Sign the transaction. */
            tx.Sign(user->Generate(tx.nSequence, params["pin"].get<std::string>().c_str()));

            /* Free the sigchain. */
            user.free();

            /* Check that the transaction is valid. */
            if(!tx.IsValid())
                throw APIException(-26, "Invalid Transaction");

            /* Accept to memory pool. */
            TAO::Ledger::mempool.Accept(tx);

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
    }
}
