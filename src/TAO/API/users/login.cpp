/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/create.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        //TODO: have the authorization system build a SHA256 hash and salt on the client side as the AUTH hash.

        /* Login to a user account. */
        json::json Users::Login(const json::json& params, bool fHelp)
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
                throw APIException(-24, "Missing PIN");

            /* Check for unlock actions */
            uint8_t nUnlockedActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to no actions
            if(params.find("minting") != params.end()
            && (params["minting"].get<std::string>() == "1"
            || params["minting"].get<std::string>() == "true"))
                nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINTING;

            if(params.find("transactions") != params.end()
            && (params["transactions"].get<std::string>() == "1"
            || params["transactions"].get<std::string>() == "true"))
                nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;


            /* Create the sigchain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(params["username"].get<std::string>().c_str(), params["password"].get<std::string>().c_str());

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::legDB->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    user.free();

                    throw APIException(-26, "Account doesn't exists");
                }

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
                throw APIException(-28, "Invalid credentials");

            /* Check the sessions. */
            {
                LOCK(MUTEX);

                for(const auto& session : mapSessions)
                {
                    if(hashGenesis == session.second->Genesis())
                    {
                        user.free();

                        ret["genesis"] = hashGenesis.ToString();
                        if(config::fAPISessions)
                            ret["session"] = debug::safe_printstr(std::dec, session.first);

                        return ret;
                    }
                }
            }

            /* Extract the PIN, if supplied, and if the user has specified to remain unlocked for minting or transacting */
            if(!config::fAPISessions && nUnlockedActions == TAO::Ledger::PinUnlock::UnlockActions::NONE)
            {
                if(!pActivePIN.IsNull())
                    pActivePIN.free();

                pActivePIN = new TAO::Ledger::PinUnlock(params["pin"].get<std::string>().c_str(), nUnlockedActions);
            }


            /* Set the return value. */
            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSession = config::fAPISessions ? LLC::GetRand() : 0;
            ret["genesis"] = hashGenesis.ToString();
            if(config::fAPISessions)
                ret["session"] = debug::safe_printstr(std::dec, nSession);

            /* Setup the account. */
            { LOCK(MUTEX);
                mapSessions.emplace(nSession, std::move(user));
            }

            return ret;
        }
    }
}
