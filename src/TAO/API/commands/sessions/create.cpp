/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Login to a user account. */
    encoding::json Sessions::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for username parameter. */
        if(!CheckParameter(jParams, "username", "string"))
            throw Exception(-127, "Missing username");

        /* Parse out username. */
        const SecureString strUsername =
            SecureString(jParams["username"].get<std::string>().c_str());

        /* Check for password parameter. */
        if(!CheckParameter(jParams, "password", "string"))
            throw Exception(-128, "Missing password");

        /* Parse out password. */
        const SecureString strPassword =
            SecureString(jParams["password"].get<std::string>().c_str());

        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Build new session object. */
        Authentication::Session tSession =
            Authentication::Session(strUsername, strPassword, Authentication::Session::LOCAL);

        /* Check for crypto object register. */
        const TAO::Register::Address hashCrypto =
            TAO::Register::Address(std::string("crypto"), tSession.Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the crypto object register. */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(hashCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-139, "Invalid credentials");

        /* Read the key type from crypto object register. */
        const uint256_t hashAuth =
            oCrypto.get<uint256_t>("auth");

        /* Check if the auth has is deactivated. */
        if(hashAuth == 0)
            throw Exception(-130, "Auth hash deactivated, please call crypto/create/auth");

        /* Generate a key to check credentials against. */
        const uint256_t hashCheck =
            tSession.Credentials()->KeyHash("auth", 0, strPIN, hashAuth.GetType());

        /* Check for invalid authorization hash. */
        if(hashAuth != hashCheck)
            throw Exception(-139, "Invalid credentials");

        /* Check if already logged in. */
        uint256_t hashSession;
        if(Authentication::Active(tSession.Genesis(), hashSession))
        {
            /* Build return json data. */
            const encoding::json jRet =
            {
                { "genesis", tSession.Genesis().ToString() },
                { "session", hashSession.ToString() }
            };

            return jRet;
        }

        /* Build a new session key. */
        hashSession =
            LLC::GetRand256();

        /* Push the new session to auth. */
        Authentication::Insert(hashSession, tSession);

        /* Build return json data. */
        const encoding::json jRet =
        {
            { "genesis", tSession.Genesis().ToString() },
            { "session", hashSession.ToString() }
        };

        return jRet;
    }
}
