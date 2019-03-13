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
#include <TAO/Ledger/types/sigchain.h>

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
            TAO::Ledger::SignatureChain user(params["username"].get<std::string>().c_str(), params["password"].get<std::string>().c_str());

            /* Get the Genesis ID. */
            uint256_t hashGenesis = user.Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction tx;
            if(LLD::legDB->HasGenesis(hashGenesis))
                throw APIException(-26, "Account already exists");

            /* Create the transaction. */
            if(!TAO::Ledger::CreateTransaction(&user, params["pin"].get<std::string>().c_str(), tx))
                throw APIException(-25, "Failed to create transaction");

            /* Sign the transaction. */
            tx.Sign(user.Generate(tx.nSequence, params["pin"].get<std::string>().c_str()));

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


        /* Get a user's account. */
        json::json Accounts::GetTransactions(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Check for genesis or username parameter, or active sig chain if neither are provided */
            if(params.find("genesis") == params.end() 
                && params.find("username") == params.end()
                && (config::fAPISessions || mapSessions.count(0) == 0) )
            {
                throw APIException(-25, "Missing Genesis ID or Username");
            }

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = atoi(params["page"].get<std::string>().c_str());

            /* Check for username parameter. */
            uint32_t nLimit = 50;
            if(params.find("limit") != params.end())
                nLimit = atoi(params["limit"].get<std::string>().c_str());

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;
            if(params.find("genesis") != params.end() )
                hashGenesis = uint256_t(params["genesis"].get<std::string>());
            else if(params.find("username") != params.end() )
                hashGenesis = TAO::Ledger::SignatureChain::GetGenesis( SecureString(params["username"].get<std::string>().c_str()));
            else if(!config::fAPISessions && mapSessions.count(0))
            {
                /* If no specific genesis or username have been provided then fall back to the active sig chain */
                hashGenesis = mapSessions[0].Genesis();
            }
            


            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                json::json obj;
                obj["version"]   = tx.nVersion;
                obj["sequence"]  = tx.nSequence;
                obj["timestamp"] = tx.nTimestamp;
                obj["genesis"]   = tx.hashGenesis.ToString();
                obj["nexthash"]  = tx.hashNext.ToString();
                obj["prevhash"]  = tx.hashPrevTx.ToString();
                obj["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                obj["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                obj["hash"]      = tx.GetHash().ToString();

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
