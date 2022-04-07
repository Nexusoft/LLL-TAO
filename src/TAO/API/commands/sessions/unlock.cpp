/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>

#include <TAO/Ledger/types/stake_minter.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Unlock an account for any given action. */
    encoding::json Sessions::Unlock(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for unlock actions */
        uint8_t nUnlockedActions =
            TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to NO actions

        /* Get our current unlocked status. */
        uint8_t nCurrentActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to NO actions
        Authentication::Unlocked(jParams, nCurrentActions);

        /* Check for mining flag. */
        if(ExtractBoolean(jParams, "mining"))
        {
            /* Can't unlock for mining in multiuser mode */
            if(config::fMultiuser.load())
                throw Exception(-288, "Cannot unlock for mining in multiuser mode");

             /* Check if already unlocked. */
            if(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::MINING)
                throw Exception(-146, "Account already unlocked for mining");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINING;
        }

        /* Check for staking flag. */
        if(ExtractBoolean(jParams, "staking"))
        {
            /* Can't unlock for staking in multiuser mode */
            if(config::fMultiuser.load())
                throw Exception(-289, "Cannot unlock for staking in multiuser mode");

             /* Check if already unlocked. */
            if(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING)
                throw Exception(-195, "Account already unlocked for staking");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::STAKING;
        }

        /* Check transactions flag. */
        if(ExtractBoolean(jParams, "transactions"))
        {
             /* Check if already unlocked. */
            if(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS)
                throw Exception(-147, "Account already unlocked for transactions");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
        }

        /* Check for notifications. */
        if(ExtractBoolean(jParams, "notifications"))
        {
             /* Check if already unlocked. */
            if(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS)
                throw Exception(-194, "Account already unlocked for notifications");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
        }

        /* If no unlock actions have been specifically set then default it to all */
        if(ExtractBoolean(jParams, "all"))
        {
            /* Check if already unlocked. */
            if(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::ALL)
                throw Exception(-148, "Account already unlocked");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::ALL;
        }

        /* Check for no actions. */
        if(nUnlockedActions == nCurrentActions)
            throw Exception(-259, "You must specify at least one unlock action");

        /* Check for authenticated sigchain. */
        if(!Authentication::Authenticate(jParams))
            throw Exception(-333, "Account failed to authenticate");

        /* Update our session with new pin. */
        Authentication::Update(jParams, nUnlockedActions);

        /* Get the genesis ID. */
        //const uint256_t hashGenesis =
        //    Authentication::Caller(jParams);

        /* Update the saved session if there is one */
        //if(LLD::Local->HasSession(hashGenesis))
        //    rSession.Save(strPin);

        /* After unlock complete, attempt to start stake minter if unlocked for staking */
        if(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING)
        {
            /* Grab a reference of our stake minter. */
            TAO::Ledger::StakeMinter& rStakeMinter =
                TAO::Ledger::StakeMinter::GetInstance();

            /* Start it if not started. */
            if(!rStakeMinter.IsStarted())
                rStakeMinter.Start();
        }

        /* populate unlocked status */
        const encoding::json jRet =
        {
            {
                "unlocked",
                {
                    { "mining",        bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::MINING        )},
                    { "notifications", bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS )},
                    { "staking",       bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING       )},
                    { "transactions",  bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS  )}
                }
            }
        };

        return jRet;
    }
}
