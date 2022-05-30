/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get status information for the currently logged in user. */
    encoding::json Sessions::Status(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our current object type. */
        const std::string strType = ExtractType(jParams);

        /* Populate unlocked status */
        uint8_t nCurrentActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to NO actions
        Authentication::Unlocked(jParams, nCurrentActions);

        /* Build all of our values now. */
        const encoding::json jUnlocked =
        {
            { "mining",        bool(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::MINING        )},
            { "notifications", bool(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS )},
            { "staking",       bool(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING       )},
            { "transactions",  bool(nCurrentActions & TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS  )}
        };

        /* Get our calling genesis. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Add the genesis */
        const encoding::json jRet =
        {
            { "genesis",  hashGenesis.ToString()            },
            { "accessed", Authentication::Accessed(jParams) },
            { "location", strType                           }, //replace this with Authentication::Location(jParams);
            { "unlocked", jUnlocked                         }
        };

        return jUnlocked;
    }
}
