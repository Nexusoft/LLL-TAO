/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>
#include <LLP/templates/trigger.h>

#include <TAO/API/types/users.h>

#include <TAO/Register/types/object.h>

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

            /* Pin parameter. */
            SecureString strPin;

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

            /* Check for pin parameter. Parse the pin parameter. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            /* Check for pin size. */
            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");

            /* Create the sigchain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUser, strPass);

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for -client mode. */
            if(config::fClient.load())
            {
                /* If not using multiuser then check to see whether another user is already logged in */
                if(mapSessions.count(0) && mapSessions[0]->Genesis() != hashGenesis)
                {
                    user.free();
                    throw APIException(-140, "CLIENT MODE: Already logged in with a different username.");
                }
                else if(mapSessions.count(0))
                {
                    json::json ret;
                    ret["genesis"] = hashGenesis.ToString();

                    return ret;
                }

                /* Check for genesis. */
                if(LLP::TRITIUM_SERVER)
                {
                    memory::atomic_ptr<LLP::TritiumNode>& pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: Sychronizing -client");

                        /* Get the last txid in sigchain. */
                        uint512_t hashLast;
                        LLD::Ledger->ReadLast(hashGenesis, hashLast); //NOTE: we don't care if it fails here, because zero means begin

                        /* Request the sig chain. */
                        debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::SIGCHAIN for ", hashGenesis.SubString());

                        LLP::TritiumNode::BlockingMessage(pNode, LLP::ACTION::LIST, uint8_t(LLP::TYPES::SIGCHAIN), hashGenesis, hashLast);

                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN received for ", hashGenesis.SubString());

                        /* Grab list of notifications. */
                        pNode->PushMessage(LLP::ACTION::LIST, uint8_t(LLP::TYPES::NOTIFICATION), hashGenesis);
                    }
                    else
                        debug::error(FUNCTION, "no connections available...");
                }
            }

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* If user genesis not in ledger, this will throw an exception. Just a matter of which one. */

                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    /* Account doesn't exist returns invalid credentials */
                    user.free();
                    throw APIException(-139, "Invalid credentials");
                }

                if(!config::fTestNet.load())
                {
                    /* After credentials verified, disallow login while in mempool and unconfirmed */
                    user.free();
                    throw APIException(-222, "User create pending confirmation");
                }

                /* Testnet allows mempool login. Get the memory pool tranasction. */
                else if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                {
                    user.free();
                    throw APIException(-137, "Couldn't get transaction");
                }
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    user.free();
                    throw APIException(-138, "No previous transaction found");
                }

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    user.free();
                    throw APIException(-138, "No previous transaction found");
                }
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, strPin, false), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
            {
                user.free();
                throw APIException(-139, "Invalid credentials");
            }

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
                            ret["session"] = session.first.ToString();

                        return ret;
                    }
                }
            }

            /* If not using multiuser then check to see whether another user is already logged in */
            if(!config::fMultiuser.load() && mapSessions.count(0) && mapSessions[0]->Genesis() != hashGenesis)
            {
                user.free();
                throw APIException(-140, "Already logged in with a different username.");
            }

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint256_t nSession = config::fMultiuser.load() ? LLC::GetRand256() : 0;
            ret["genesis"] = hashGenesis.ToString();

            if(config::fMultiuser.load())
                ret["session"] = nSession.ToString();

            /* Setup the account. */
            {
                LOCK(MUTEX);
                mapSessions.emplace(nSession, std::move(user));
            }

            /* If not using Multiuser then generate and cache the private key for the "network" key so that we can generate AUTH
               LLP messages to authenticate to peers */
            if(!config::fMultiuser.load())
            {
                /* Get the private key. */
                pAuthKey = new memory::encrypted_type<uint512_t>(user->Generate("network", 0, strPin));

                /* Generate an AUTH message to send to all peers */
                DataStream ssMessage = LLP::TritiumNode::GetAuth(true);

                /* Check whether it is valid before relaying it to all peers */
                if(ssMessage.size() > 0)
                    LLP::TRITIUM_SERVER->_Relay(uint8_t(LLP::ACTION::AUTH), ssMessage);
            }

            return ret;
        }
    }
}
