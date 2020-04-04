/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>
#include <Util/include/args.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Unlock an account for mining (TODO: make this much more secure) */
        json::json Users::Unlock(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Restrict Unlock to sessionless API */
            if(config::fMultiuser.load())
                throw APIException(-145, "Unlock not supported in multiuser mode");

            /* Check default session (unlock only supported in single user mode). */
            if(!mapSessions.count(0))
                throw APIException(-11, "User not logged in.");

            /* Check for pin parameter. Parse the pin parameter. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");

            /* Check for unlock actions */
            uint8_t nUnlockedActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to ALL actions

            /* If it has already been unlocked then set the Unlocked actions to the current unlocked actions */
            if(!Locked())
                nUnlockedActions = pActivePIN->UnlockedActions();

            /* Check for mining flag. */
            if(params.find("mining") != params.end())
            {
                std::string strMint = params["mining"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && pActivePIN->CanMine())
                        throw APIException(-146, "Account already unlocked for mining");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINING;
                }
            }

            /* Check for staking flag. */
            if(params.find("staking") != params.end())
            {
                std::string strMint = params["staking"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && pActivePIN->CanStake())
                        throw APIException(-195, "Account already unlocked for staking");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::STAKING;
                }
            }

            /* Check transactions flag. */
            if(params.find("transactions") != params.end())
            {
                std::string strTransactions = params["transactions"].get<std::string>();

                if(strTransactions == "1" || strTransactions == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && pActivePIN->CanTransact())
                        throw APIException(-147, "Account already unlocked for transactions");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
                }
            }

            /* Check for notifications. */
            if(params.find("notifications") != params.end())
            {
                std::string strNotifications = params["notifications"].get<std::string>();

                if(strNotifications == "1" || strNotifications == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && pActivePIN->ProcessNotifications())
                        throw APIException(-194, "Account already unlocked for notifications");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
                }
            }

            /* If no unlock actions have been specifically set then default it to all */
            if(nUnlockedActions == TAO::Ledger::PinUnlock::UnlockActions::NONE)
            {
                if(!Locked())
                    throw APIException(-148, "Account already unlocked");
                else
                    nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::ALL;
            }

            /* Get the sigchain from map of users. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = mapSessions[0];

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                    throw APIException(-136, "Account doesn't exist");

                /* Get the memory pool tranasction. */
                if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                    throw APIException(-137, "Couldn't get transaction");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user->Generate(txPrev.nSequence + 1, strPin, false), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-149, "Invalid PIN");

            /* Extract the PIN. */
            if(!pActivePIN.IsNull())
                pActivePIN.free();

            pActivePIN = new TAO::Ledger::PinUnlock(strPin, nUnlockedActions);

            /* After unlock complete, attempt to start stake minter if unlocked for staking */
            if(pActivePIN->CanStake())
            {
                TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();

                if(!stakeMinter.IsStarted())
                    stakeMinter.Start();
            }

            /* populate unlocked status */
            json::json jsonUnlocked;

            jsonUnlocked["mining"] = !pActivePIN.IsNull() && pActivePIN->CanMine();
            jsonUnlocked["notifications"] = !pActivePIN.IsNull() && pActivePIN->ProcessNotifications();
            jsonUnlocked["staking"] = !pActivePIN.IsNull() && pActivePIN->CanStake();
            jsonUnlocked["transactions"] = !pActivePIN.IsNull() && pActivePIN->CanTransact();


            ret["unlocked"] = jsonUnlocked;
            return ret;
        }
    }
}
