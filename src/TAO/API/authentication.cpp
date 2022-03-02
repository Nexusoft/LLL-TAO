/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Static initialization of mutex. */
    std::recursive_mutex Authentication::MUTEX;


    /* Static initialize of map sessions. */
    std::map<uint256_t, Authentication::Session> Authentication::mapSessions;


    /* Initializes the current authentication systems. */
    void Authentication::Initialize()
    {

    }


    /* Authenticate a new session from incoming parameters. */
    encoding::json Authentication::Authenticate(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Pin parameter. */
        const SecureString strPIN = ExtractPIN(jParams);

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

        /* Build new session object. */
        Session tSession =
            Session(strUsername, strPassword, Session::LOCAL);

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
            throw Exception(-130, "Auth hash deactivated, please call sessions/initialize/credentials");

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
        hashSession = LLC::GetRand256();

        /* Add the new session to sessions map. */
        mapSessions.insert(std::make_pair(hashSession, std::move(tSession)));

        /* Build return json data. */
        const encoding::json jRet =
        {
            { "genesis", tSession.Genesis().ToString() },
            { "session", hashSession.ToString() }
        };

        return jRet;
    }


    /* Check if user is already authenticated by genesis-id. */
    bool Authentication::Active(const uint256_t& hashGenesis, uint256_t &hashSession)
    {
        RECURSIVE(MUTEX);

        /* Loop through sessions map. */
        for(const auto& rSession : mapSessions)
        {
            /* Check genesis to session. */
            if(rSession.second.Genesis() == hashGenesis)
            {
                hashSession = rSession.first;
                return true;
            }
        }

        return false;
    }


    /* Check if user is already authenticated by incoming parameters. */
    bool Authentication::Authenticated(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Check that the session exists. */
        if(!mapSessions.count(hashSession))
            return false;

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Check for password requirement field. */
        if(config::GetBoolArg("-requirepassword", false))
        {
            /* Grab our password from parameters. */
            if(!CheckParameter(jParams, "password", "string"))
                throw Exception(-128, "-requirepassword active, must pass in password=<password> for all commands when enabled");

            /* Parse out password. */
            const SecureString strPassword =
                SecureString(jParams["password"].get<std::string>().c_str());

            /* Check our password input compared to our internal sigchain password. */
            if(rSession.Credentials()->Password() != strPassword)
            {
                /* Increment our auth attempts if failed password. */
                ++rSession.nAuthFailures;

                return false;
            }
        }

        return true;
    }


    /* Get the genesis-id of the given caller using session from params. */
    bool Authentication::Caller(const encoding::json& jParams, uint256_t &hashCaller)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return false;

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Set the caller from our session data. */
        hashCaller = rSession.Genesis();

        return true;
    }


    /* Terminate an active session by parameters. */
    void Authentication::Terminate(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return;

        /* Erase the session from map. */
        mapSessions.erase(hashSession);
    }


    /* Shuts down the current authentication systems. */
    void Authentication::Shutdown()
    {

    }
}
