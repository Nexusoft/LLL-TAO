/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>

#include <TAO/Ledger/types/tritium_minter.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Login to a user account. */
    encoding::json Sessions::Terminate(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the genesis-id. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check for an active session. */
        if(!Authentication::Active(hashGenesis))
            throw Exception(-234, "Session does not exist");

        /* Check for authenticated sigchain. */
        if(config::fMultiuser.load() && config::GetBoolArg("-terminateauth", true) && !Authentication::Authenticate(jParams))
            throw Exception(-333, "Account failed to authenticate");

        /* Stop stake minter if running. Minter ignores request if not running, so safe to just call both */
        TAO::Ledger::StakeMinter::GetInstance().Stop();

        /* Check if we have set to clear session too. */
        if(ExtractBoolean(jParams, "clear"))
            LLD::Local->EraseSession(hashGenesis);

        /* Terminate our session now. */
        Authentication::Terminate(jParams);

        return BuildResponse();
    }
}
