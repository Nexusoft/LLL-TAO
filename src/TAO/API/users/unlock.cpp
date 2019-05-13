/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <Util/include/args.h>

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
        json::json Users::Unlock(const json::json& params, bool fHelp)
        {
            /* Restrict Unlock to sessionless API */
            if(config::fAPISessions)
                throw APIException(-23, "Unlock not supported for session-based API");

            if(!mapSessions.count(0))
                throw APIException(-1, "User not logged in.");

            /* Check for pin parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-24, "Missing Pin");

            /* Check if already unlocked. */
            if(!Locked())
                throw APIException(-26, "Account already unlocked");

            /* Check for unlock actions */
            uint8_t nUnlockedActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to ALL actions
            if (params.find("minting") != params.end()
            && (params["minting"].get<std::string>() == "1"
            ||  params["minting"].get<std::string>() == "true"))
                nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINTING;

            if (params.find("transactions") != params.end()
            && (params["transactions"].get<std::string>() == "1"
            ||  params["transactions"].get<std::string>() == "true"))
                nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;

            /* If no unlock actions have been specifically set then default it to all */
            if (nUnlockedActions == TAO::Ledger::PinUnlock::UnlockActions::NONE)
                nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::ALL;

            /* Get the sigchain from map of users. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = mapSessions[0];

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::legDB->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                    throw APIException(-26, "Account doesn't exist");

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
                if(!LLD::legDB->ReadTx(hashLast, txPrev))
                    throw APIException(-27, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, params["pin"].get<std::string>().c_str(), false));

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-28, "Invalid PIN");

            /* Extract the PIN. */
            if(!pActivePIN.IsNull())
                pActivePIN.free();

            pActivePIN = new TAO::Ledger::PinUnlock( params["pin"].get<std::string>().c_str(), nUnlockedActions);

            return true;
        }
    }
}
