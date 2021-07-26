/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>

#include <TAO/API/include/extract.h>

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
        encoding::json Users::Unlock(const encoding::json& jParams, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Pin parameter. */
            const SecureString strPin = ExtractPIN(jParams);

            /* Get the session */
            Session& session = GetSession(jParams);

            /* Check for unlock actions */
            uint8_t nUnlockedActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to ALL actions

            /* If it has already been unlocked then set the Unlocked actions to the current unlocked actions */
            if(!session.Locked())
                nUnlockedActions = session.GetActivePIN()->UnlockedActions();

            /* Check for mining flag. */
            if(jParams.find("mining") != jParams.end())
            {
                std::string strMint = jParams["mining"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                    /* Can't unlock for mining in multiuser mode */
                    if(config::fMultiuser.load())
                        throw Exception(-288, "Cannot unlock for mining in multiuser mode");

                     /* Check if already unlocked. */
                    if(!session.GetActivePIN().IsNull() && session.GetActivePIN()->CanMine())
                        throw Exception(-146, "Account already unlocked for mining");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINING;
                }
            }

            /* Check for staking flag. */
            if(jParams.find("staking") != jParams.end())
            {
                std::string strMint = jParams["staking"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                    /* Can't unlock for staking in multiuser mode */
                    if(config::fMultiuser.load())
                        throw Exception(-289, "Cannot unlock for staking in multiuser mode");

                     /* Check if already unlocked. */
                    if(!session.GetActivePIN().IsNull() && session.GetActivePIN()->CanStake())
                        throw Exception(-195, "Account already unlocked for staking");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::STAKING;
                }
            }

            /* Check transactions flag. */
            if(jParams.find("transactions") != jParams.end())
            {
                std::string strTransactions = jParams["transactions"].get<std::string>();

                if(strTransactions == "1" || strTransactions == "true")
                {
                     /* Check if already unlocked. */
                    if(!session.GetActivePIN().IsNull() && session.GetActivePIN()->CanTransact())
                        throw Exception(-147, "Account already unlocked for transactions");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
                }
            }

            /* Check for notifications. */
            if(jParams.find("notifications") != jParams.end())
            {
                std::string strNotifications = jParams["notifications"].get<std::string>();

                if(strNotifications == "1" || strNotifications == "true")
                {
                     /* Check if already unlocked. */
                    if(!session.GetActivePIN().IsNull() && session.GetActivePIN()->ProcessNotifications())
                        throw Exception(-194, "Account already unlocked for notifications");
                    else
                        nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
                }
            }

            /* If no unlock actions have been specifically set then default it to all */
            if(nUnlockedActions == TAO::Ledger::PinUnlock::UnlockActions::NONE)
            {
                if(!session.Locked())
                    throw Exception(-148, "Account already unlocked");
                else
                    nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::ALL;
            }


            /* Get the genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(session.GetAccount()->Generate(txPrev.nSequence + 1, strPin), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw Exception(-149, "Invalid PIN");

            /* update the unlocked actions */
            session.UpdatePIN(strPin, nUnlockedActions);

            /* Update the saved session if there is one */
            if(LLD::Local->HasSession(hashGenesis))
                session.Save(strPin);

            /* After unlock complete, attempt to start stake minter if unlocked for staking */
            if(session.CanStake())
            {
                TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();

                if(!stakeMinter.IsStarted())
                    stakeMinter.Start();
            }

            /* populate unlocked status */
            encoding::json jsonUnlocked;

            jsonUnlocked["mining"] = !session.GetActivePIN().IsNull() && session.CanMine();
            jsonUnlocked["notifications"] = !session.GetActivePIN().IsNull() && session.CanProcessNotifications();
            jsonUnlocked["staking"] = !session.GetActivePIN().IsNull() && session.CanStake();
            jsonUnlocked["transactions"] = !session.GetActivePIN().IsNull() && session.CanTransact();


            ret["unlocked"] = jsonUnlocked;
            return ret;
        }
    }
}
