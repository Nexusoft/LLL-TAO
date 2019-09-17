/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/users.h>
#include <TAO/Ledger/types/tritium_minter.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Lock an account for mining (TODO: make this much more secure) */
        json::json Users::Lock(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Restrict Unlock / Lock to sessionless API */
            if(config::fMultiuser.load())
                throw APIException(-131, "Lock not supported in multiuser mode");

            /* Check if already unlocked. */
            if(pActivePIN.IsNull() || (!pActivePIN.IsNull() && pActivePIN->PIN() == ""))
                throw APIException(-132, "Account already locked");

            /* The current unlock actions */
            uint8_t nUnlockedActions = pActivePIN->UnlockedActions();

            /* Check for mining flag. */
            if(params.find("mining") != params.end())
            {
                std::string strMint = params["mining"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already locked. */
                    if(!pActivePIN.IsNull() && !pActivePIN->CanMine())
                        throw APIException(-196, "Account already locked for mining");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::MINING;
                }
            }

            /* Check for staking flag. */
            if(params.find("staking") != params.end())
            {
                std::string strMint = params["staking"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already locked. */
                    if(!pActivePIN.IsNull() && !pActivePIN->CanStake())
                        throw APIException(-197, "Account already locked for staking");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::STAKING;
                }
            }

            /* Check transactions flag. */
            if(params.find("transactions") != params.end())
            {
                std::string strTransactions = params["transactions"].get<std::string>();

                if(strTransactions == "1" || strTransactions == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && !pActivePIN->CanTransact())
                        throw APIException(-198, "Account already locked for transactions");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
                }
            }

            /* Check for notifications. */
            if(params.find("notifications") != params.end())
            {
                std::string strNotifications = params["notifications"].get<std::string>();

                if(strNotifications == "1" || strNotifications == "true")
                {
                     /* Check if already unlocked. */
                    if(!pActivePIN.IsNull() && !pActivePIN->ProcessNotifications())
                        throw APIException(-199, "Account already locked for notifications");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
                }
            }

            /* Clear the pin */
            LOCK(MUTEX);

            /* If we have changed specific unlocked actions them set them on the pin */
            if(nUnlockedActions != pActivePIN->UnlockedActions())
            {
                /* Extract the PIN. */
                SecureString strPin = pActivePIN->PIN();

                /* Clean up current pin */
                if(!pActivePIN.IsNull())
                    pActivePIN.free();

                /* Set new unlock options */
                pActivePIN = new TAO::Ledger::PinUnlock(strPin, nUnlockedActions);
            }
            else
            {
                /* If no unlock actions left then free the pin from cache */
                pActivePIN.free();
            }

            /* Stop the stake minter if it is no longer unlocked for staking */
            if(pActivePIN.IsNull() || (!(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING)))
            {
                /* If stake minter is running, stop it */
                TAO::Ledger::TritiumMinter& stakeMinter = TAO::Ledger::TritiumMinter::GetInstance();
                if(stakeMinter.IsStarted())
                    stakeMinter.Stop();
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
