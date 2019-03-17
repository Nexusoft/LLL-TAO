/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Unlock an account for mining (TODO: make this much more secure) */
        json::json Accounts::Unlock(const json::json& params, bool fHelp)
        {
            /* Check for username parameter. */
            if(params.find("session") == params.end())
                throw APIException(-23, "Missing Session");

            /* Check for password parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-24, "Missing Pin");

            /* Check if already unlocked. */
            if(pairUnlocked.first != 0)
                throw APIException(-26, "Account already unlocked");

            /* Extract the session. */
            uint64_t nSession = std::stoull(params["session"].get<std::string>());
            if(!mapSessions.count(nSession))
                throw APIException(-26, "Session not found");

            /* Get the sigchain from map of users. */
            TAO::Ledger::SignatureChain* user = mapSessions[nSession];

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::legDB->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                    throw APIException(-26, "Account doesn't exists");

                /* Get the memory pool tranasction. */
                if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                    throw APIException(-26, "Couldn't get transaction");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                    throw APIException(-27, "No previous transaction found");

                /* Get previous transaction */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashLast, txPrev))
                    throw APIException(-27, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, params["pin"].get<std::string>().c_str()));

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-28, "Invalid PIN");

            /* Extract the PIN. */
            pairUnlocked = std::make_pair(nSession, params["pin"].get<std::string>().c_str());

            return true;
        }
    }
}
