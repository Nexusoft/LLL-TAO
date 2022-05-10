/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/transaction.h>
#include <TAO/API/types/commands/sessions.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get status information for the currently logged in user. */
    encoding::json Sessions::Status(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our calling genesis. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* We require PIN for status when in multiuser mode. */
        bool fUsername = false;
        if(config::fMultiuser.load())
            fUsername = (CheckParameter(jParams, "pin", "string, number") && Authentication::Authenticate(jParams));
        else
            fUsername = true;

        /* Populate Username */
        encoding::json jRet;
        if(fUsername)
            jRet["username"] = Authentication::Credentials(jParams)->UserName().c_str();

        /* We need this for our database queries. */
        uint512_t hashLast = 0;

        /* Add the genesis */
        jRet["genesis"]      = hashGenesis.ToString();
        jRet["confirmed"]    = bool(LLD::Logical->ReadFirst(hashGenesis, hashLast));
        jRet["lastactive"]   = Authentication::Accessed(jParams);
        jRet["recovery"]     = false;
        jRet["transactions"] = 0;

        /* Read the last transaction for the sig chain */
        if(LLD::Logical->ReadLast(hashGenesis, hashLast))
        {
            /* Get the transaction from disk. */
            Transaction tx;
            if(!LLD::Logical->ReadTx(hashLast, tx))
                throw Exception(-108, "Failed to read transaction");

            /* Populate transactional level data. */
            jRet["transactions"] = tx.nSequence + 1;
            jRet["recovery"]     = bool(tx.hashRecovery != 0);
        }

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

        /* Add unlocked to return. */
        jRet["unlocked"] = jUnlocked;

        return jRet;
    }
}
