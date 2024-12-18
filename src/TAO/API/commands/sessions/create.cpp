/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands/sessions.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Login to a user account. */
    encoding::json Sessions::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Parse out username. */
        const SecureString strUsername =
            SecureString(ExtractString(jParams, "username").c_str());

        /* Parse out password. */
        const SecureString strPassword =
            SecureString(ExtractString(jParams, "password").c_str());

        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Build new session object. */
        Authentication::Session tSession =
            Authentication::Session(strUsername, strPassword, Authentication::Session::LOCAL);

        /* Check our session's credentials. */
        if(!validate_session(tSession, strPIN))
            throw Exception(-139, "Invalid credentials for ", tSession.Genesis().ToString());

        /* Check if already logged in. */
        uint256_t hashSession = Authentication::SESSION::DEFAULT; //we fallback to this in single user mode.
        if(Authentication::Active(tSession.Genesis(), hashSession))
        {
            /* Build return json data. */
            encoding::json jRet =
            {
                { "genesis", tSession.Genesis().ToString() },
                { "session", hashSession.ToString() }
            };

            /* Check for single user mode. */
            if(!config::fMultiuser.load())
                jRet.erase("session");

            return jRet;
        }

        /* Build a new session key. */
        if(config::fMultiuser.load())
            hashSession = LLC::GetRand256();

        /* Push the new session to auth. */
        Authentication::Insert(hashSession, tSession);

        /* Build return json data. */
        encoding::json jRet =
        {
            { "genesis", tSession.Genesis().ToString() },
            { "session", hashSession.ToString() }
        };

        /* Initialize our indexing session. */
        Indexing::Initialize(hashSession);

        /* Check for single user mode. */
        if(!config::fMultiuser.load())
            jRet.erase("session");

        return jRet;
    }
}
