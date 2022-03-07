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


    /* Insert a new session into authentication map. */
    void Authentication::Insert(const uint256_t& hashSession, Session& rSession)
    {
        RECURSIVE(MUTEX);

        /* Add the new session to sessions map. */
        mapSessions.insert(std::make_pair(hashSession, std::move(rSession)));
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
                /* Set our returned session hash. */
                hashSession = rSession.first;

                /* Check if active. */
                if(rSession.second.Active() > config::GetArg("-inactivetimeout", 3600))
                    return false;

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
                return increment_failures(hashSession);
        }

        /* Adjust our activity time if authenticated. */
        rSession.nLastActive   = runtime::unifiedtimestamp();

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


    /* Get an instance of current session indexed by session-id. */
    Authentication::Session& Authentication::Instance(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-139, "Invalid credentials");

        /* Get a copy of our current active session. */
        return mapSessions[hashSession];
    }


    /* Unlock and get the active pin from current session. */
    bool Authentication::Unlock(const encoding::json& jParams, SecureString &strPIN, const uint8_t nRequestedActions)
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

        /* Get the active pin if not currently stored. */
        if(!rSession.Unlock(strPIN, nRequestedActions))
            strPIN = ExtractPIN(jParams);

        return true;
    }


    /* Unlock this session by inputing a valid pin and requested actions. */
    void Authentication::Unlock(const encoding::json& jParams, const uint8_t nActions)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return;

        /* Get a copy of our current active session. */
        //const Session& rSession =
        //    mapSessions[hashSession];

        return;
    }


    /* Terminate an active session by parameters. */
    void Authentication::Terminate(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session");

        /* Terminate the session now. */
        terminate_session(hashSession);
    }


    /* Shuts down the current authentication systems. */
    void Authentication::Shutdown()
    {

    }


    /* Terminate an active session by parameters. */
    void Authentication::terminate_session(const uint256_t& hashSession)
    {
        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return;

        /* Erase the session from map. */
        mapSessions.erase(hashSession);
    }

    /* Increment the failure counter to deauthorize user after failed auth. */
    bool Authentication::increment_failures(const uint256_t& hashSession)
    {
        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return false;

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Increment the auth failures. */
        if(++rSession.nAuthFailures >= config::GetArg("-authattempts", 3))
        {
            /* Terminate our internal session. */
            terminate_session(hashSession);

            /* Return failure to API endpoint. */
            throw Exception(-290, "Too many invalid password/pin attempts (",
                rSession.nAuthFailures.load(), "): Session ", hashSession.SubString(), " Terminated");
        }

        return false;
    }
}
