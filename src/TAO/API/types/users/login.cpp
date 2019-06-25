/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/allocators.h>

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
                throw APIException(-127, "Missing username");

            /* Parse out username. */
            SecureString strUser = SecureString(params["username"].get<std::string>().c_str());

            /* Check for username size. */
            if(strUser.size() == 0)
                throw APIException(-133, "Zero-length username");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-128, "Missing password");

            /* Parse out password. */
            SecureString strPass = SecureString(params["password"].get<std::string>().c_str());

            /* Check for password size. */
            if(strPass.size() == 0)
                throw APIException(-134, "Zero-length password");

            /* Check for pin parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-129, "Missing PIN");

            /* Parse out pin. */
            SecureString strPin  = SecureString(params["pin"].get<std::string>().c_str());

            /* Check for pin size. */
            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");

            /* Create the sigchain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUser, strPass);

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    user.free();
                    throw APIException(-136, "Account doesn't exist");
                }

                /* Get the memory pool tranasction. */
                if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                    throw APIException(-137, "Couldn't get transaction");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                    throw APIException(-138, "No previous transaction found");

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev))
                    throw APIException(-138, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, strPin, false), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-139, "Invalid credentials");

            /* Check the sessions. */
            {
                LOCK(MUTEX);

                for(const auto& session : mapSessions)
                {
                    if(hashGenesis == session.second->Genesis())
                    {
                        user.free();

                        ret["genesis"] = hashGenesis.ToString();
                        if(config::fMultiuser.load())
                            ret["session"] = debug::safe_printstr(std::dec, session.first);

                        return ret;
                    }
                }
            }

            /* If not using multiuser then check to see whether another user is already logged in */
            if(!config::fMultiuser.load() && mapSessions.count(0) && mapSessions[0]->Genesis() != hashGenesis)
                throw APIException(-140, "Already logged in with a different username.");

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSession = config::fMultiuser.load() ? LLC::GetRand() : 0;
            ret["genesis"] = hashGenesis.ToString();

            if(config::fMultiuser.load())
                ret["session"] = debug::safe_printstr(std::dec, nSession);

            /* Setup the account. */
            {
                LOCK(MUTEX);
                mapSessions.emplace(nSession, std::move(user));
            }

            return ret;
        }
    }
}
