/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/argon2.h>
#include <LLC/include/encrypt.h>
#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/commands/sessions.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Saves the users session into the local DB so that it can be resumed later after a crash */
    encoding::json Sessions::Load(const encoding::json& jParams, const bool fHelp)
    {
        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Get our genesis hash. */
        const uint256_t hashGenesis =
            ExtractGenesis(jParams);

        /* Check if already logged in. */
        uint256_t hashSession = Authentication::SESSION::DEFAULT; //we fallback to this in single user mode.
        if(Authentication::Active(hashGenesis, hashSession))
        {
            /* Build return json data. */
            encoding::json jRet =
            {
                { "genesis", hashGenesis.ToString() },
                { "session", hashSession.ToString() }
            };

            /* Check for single user mode. */
            if(!config::fMultiuser.load())
                jRet.erase("session");

            return jRet;
        }

        /* Load the encrypted data from the local DB */
        std::vector<uint8_t> vchCypherText;
        if(!LLD::Local->ReadSession(hashGenesis, vchCypherText))
            throw Exception(-309, "Error loading session.");

        /* Generate a symmetric key to encrypt it based on the session ID and pin */
        std::vector<uint8_t> vchKey = hashGenesis.GetBytes();
        vchKey.insert(vchKey.end(), strPIN.begin(), strPIN.end());

        /* Generate symmetric key for decryption */
        const uint256_t hashCypher =
            LLC::Argon2Fast_256(vchKey);

        /* The decrypted session bytes */
        std::vector<uint8_t> vchSession;
        if(!LLC::DecryptAES256(hashCypher.GetBytes(), vchCypherText, vchSession))
            throw Exception(-309, "Error loading session."); // generic message callers can't brute force session id's

        /* Attempt to deserialize the session. */
        try
        {
            /* Use a data stream to help deserialize */
            DataStream ssData(vchSession, SER_LLD, 1);

            /* De-serialize our username. */
            SecureString strUsername;
            ssData >> strUsername;

            /* De-serialize our password. */
            SecureString strPassword;
            ssData >> strPassword;

            /* Build new session object. */
            Authentication::Session tSession =
                Authentication::Session(strUsername, strPassword, Authentication::Session::LOCAL);

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
        catch(const std::exception& e)
        {
            throw Exception(-309, "Error loading session."); // generic message callers can't brute force session id's
        }
    }
}
