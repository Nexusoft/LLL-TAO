/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/argon2.h>
#include <LLC/include/encrypt.h>
#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the current logged in sessions for -multiusername mode. */
    encoding::json Sessions::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Extra paranoid check, this call should not be reachable without this parameter but double check anyhow. */
        if(!config::GetBoolArg("-multiusername", false))
            throw Exception(-1, FUNCTION, " not available unless -multiusername is enabled");

        /* Get a current list of all of our active sessions. */
        const auto vSessions =
            Authentication::Sessions();

        /* Build our response JSON.. */
        encoding::json jRet = encoding::json::array();

        /* Loop through our active sessions. */
        for(const auto& rSession : vSessions)
        {
            /* Cache some local variables. */
            const uint256_t& hashSession = rSession.first;
            const uint256_t& hashGenesis = rSession.second;

            /* Add the access time */
            const uint64_t nAccessed =
                Authentication::Accessed(hashSession);

            /* Add the duration time. */
            const uint64_t nDuration =
                (runtime::unifiedtimestamp() - nAccessed);

            /* Build our json object. */
            const encoding::json jSession =
            {
                { "username", Authentication::Credentials(hashSession)->UserName() },
                { "genesis",  hashGenesis.ToString()                               },
                { "session",  hashSession.ToString()                               },
                { "accessed", nAccessed                                            },
                { "duration", nDuration                                            }
            };

            /* Push our session to return value. */
            jRet.push_back(jSession);
        }

        return jRet;
    }
}
