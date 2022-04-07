/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>

#include <TAO/API/include/extract.h>

#include <TAO/Ledger/types/stake_minter.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Lock an account for mining (TODO: make this much more secure) */
        encoding::json Users::Lock(const encoding::json& jParams, const bool fHelp)
        {
            /* Pin parameter. */
            const SecureString strPin =
                ExtractPIN(jParams);

            /* Get the session */
            Session& rSession = GetSession(jParams);

            /* If it has already been locked then set the Unlocked actions to the current locked actions */
            if(rSession.Locked())
                throw Exception(-148, "Account already locked");

            /* Get our previous actions. */
            const uint8_t nPreviousActions =
                rSession.GetActivePIN()->UnlockedActions();;

            /* Check for lock actions */
            uint8_t nUnlockedActions = nPreviousActions;

            /* Check for mining flag. */
            if(ExtractBoolean(jParams, "mining"))
            {
                /* Can't unlock for mining in multiuser mode */
                if(config::fMultiuser.load())
                    throw Exception(-288, "Cannot lock for mining in multiuser mode");

                 /* Check if already locked. */
                if(!rSession.CanMine())
                    throw Exception(-146, "Account already locked for mining");

                /* Adjust the locked flags. */
                nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::MINING;
            }

            /* Check for staking flag. */
            if(ExtractBoolean(jParams, "staking"))
            {
                /* Can't unlock for staking in multiuser mode */
                if(config::fMultiuser.load())
                    throw Exception(-289, "Cannot lock for staking in multiuser mode");

                 /* Check if already locked. */
                if(!rSession.CanStake())
                    throw Exception(-195, "Account already locked for staking");

                /* Adjust the locked flags. */
                nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::STAKING;
            }

            /* Check transactions flag. */
            if(ExtractBoolean(jParams, "transactions"))
            {
                 /* Check if already locked. */
                if(!rSession.CanTransact())
                    throw Exception(-147, "Account already locked for transactions");

                /* Adjust the locked flags. */
                nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
            }

            /* Check for notifications. */
            if(ExtractBoolean(jParams, "notifications"))
            {
                 /* Check if already locked. */
                if(!rSession.CanProcessNotifications())
                    throw Exception(-194, "Account already locked for notifications");

                /* Adjust the locked flags. */
                nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
            }

            /* If no unlock actions have been specifically set then default it to all */
            if(ExtractBoolean(jParams, "all"))
            {
                /* Check if already locked. */
                if(!rSession.CanMine() && !rSession.CanStake() && !rSession.CanTransact() && !rSession.CanProcessNotifications())
                    throw Exception(-148, "Account already locked");

                /* Adjust the locked flags. */
                nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::ALL;
            }

            /* Check for no actions. */
            if(nUnlockedActions == nPreviousActions)
                throw Exception(-259, "You must specify at least one unlock action");

            /* Authenticate the request. */
            if(!Authenticate(jParams))
                throw Exception(-139, "Invalid credentials");

            /* update the locked actions */
            rSession.UpdatePIN(strPin, nUnlockedActions);

            /* Get the genesis ID. */
            const uint256_t hashGenesis =
                rSession.GetAccount()->Genesis();

            /* Update the saved session if there is one */
            if(LLD::Local->HasSession(hashGenesis))
                rSession.Save(strPin);

            /* After lock complete, attempt to stop stake minter if locked for staking */
            if(!rSession.CanStake())
            {
                /* Grab a reference of our stake minter. */
                TAO::Ledger::StakeMinter& rStakeMinter =
                    TAO::Ledger::StakeMinter::GetInstance();

                /* Start it if not started. */
                if(!rStakeMinter.IsStarted())
                    rStakeMinter.Stop();
            }

            /* populate locked status */
            const encoding::json jRet =
            {
                {
                    "locked",
                    {
                        { "mining",        rSession.CanMine()                 },
                        { "notifications", rSession.CanProcessNotifications() },
                        { "staking",       rSession.CanStake()                },
                        { "transactions",  rSession.CanTransact()             }
                    }
                }
            };

            return jRet;
        }
    }
}
