/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/types/transaction.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/profiles.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get status information for a given profile. */
    encoding::json Profiles::Status(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our calling genesis. */
        const uint256_t hashGenesis =
            ExtractGenesis(jParams);

        /* Define for our return data */
        encoding::json jRet;

        /* We need this for our database queries. */
        uint512_t hashLast = 0;

        /* Add the genesis */
        jRet["genesis"]      = hashGenesis.ToString();
        jRet["confirmed"]    = bool(LLD::Logical->ReadFirst(hashGenesis, hashLast));
        jRet["recovery"]     = false;
        jRet["crypto"]       = false;
        jRet["transactions"] = 0;

        /* By default don'r require authentication on single user or multiusername mode. */
        bool fAuthenticated =
            (!config::fMultiuser.load() || config::GetBoolArg("-multiusername", false));

        /* If PIN is supplied and logged in, give a more detailed status of profile. */
        if(!fAuthenticated && CheckParameter(jParams, "pin", "string, number"))
        {
            /* Check our authentication values if supplied. */
            if(!Authentication::Authenticate(jParams))
                throw Exception(-333, "Account failed to authenticate");

            /* Set our authentication status now. */
            fAuthenticated = true;
        }

        /* Add our session information if authenticated. */
        if(fAuthenticated && Authentication::Active(hashGenesis))
        {
            /* Add in profile username for active session with profile. */
            const encoding::json jSession =
            {
                { "username", Authentication::Credentials(jParams)->UserName() },
                { "accessed", Authentication::Accessed(jParams)                }
            };

            /* Add as sessions object. */
            jRet["session"] = jSession;
        }

        /* Read the last transaction for the sig chain */
        if(LLD::Ledger->ReadLast(hashGenesis, hashLast))
        {
            /* Get the transaction from disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashLast, tx))
                throw Exception(-108, "Failed to read transaction");

            /* Populate transactional level data. */
            jRet["transactions"] = tx.nSequence + 1;
            jRet["recovery"]     = bool(tx.hashRecovery != 0);
        }

        /* Check for crypto object register. */
        const TAO::Register::Address hashCrypto =
            TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

        /* Add our crypto auth data if we found the register. */
        TAO::Register::Object oCrypto;
        if(LLD::Register->ReadObject(hashCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            jRet["crypto"] = (oCrypto.get<uint256_t>("auth") != 0);

        return jRet;
    }
}
