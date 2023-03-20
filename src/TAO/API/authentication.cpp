/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/hash/xxh3.h>

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


    /* Static initialize of map locks. */
    std::vector<std::recursive_mutex> Authentication::vLocks;


    /* Initializes the current authentication systems. */
    void Authentication::Initialize()
    {
        /* Create our session locks. */
        vLocks = std::vector<std::recursive_mutex>(config::GetArg("-sessionlocks", 1024));
    }


    /* Insert a new session into authentication map. */
    void Authentication::Insert(const uint256_t& hashSession, Session& rSession)
    {
        RECURSIVE(MUTEX);

        /* Add the new session to sessions map. */
        mapSessions.insert(std::make_pair(hashSession, std::move(rSession)));
    }


    /* Lets everything know that session is ready to be used.*/
    void Authentication::SetReady(const uint256_t& hashSession)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-11, "Session not found");

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Set our initializing flag as ready now. */
        rSession.fInitializing.store(false);
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


    /* Check if user is already authenticated by genesis-id. */
    bool Authentication::Active(const uint256_t& hashGenesis)
    {
        RECURSIVE(MUTEX);

        /* Loop through sessions map. */
        for(const auto& rSession : mapSessions)
        {
            /* Check genesis to session. */
            if(rSession.second.Genesis() == hashGenesis)
                return true;
        }

        return false;
    }


    /* Get the last time that session was accessed */
    uint64_t Authentication::Accessed(const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        return Accessed(hashSession);
    }


    /* Get the last time that session was accessed */
    uint64_t Authentication::Accessed(const uint256_t& hashSession)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return runtime::unifiedtimestamp();

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        return rSession.Accessed();
    }


    /* Checks if a session is ready to be used. */
    bool Authentication::Indexing(const encoding::json& jParams)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            return false;

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        return rSession.fInitializing.load();
    }


    /* Authenticate a user's credentials against their sigchain. */
    bool Authentication::Authenticate(const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        /* Extract the PIN from parameters. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        {
            RECURSIVE(MUTEX);

            /* Check for active session. */
            if(!mapSessions.count(hashSession))
                throw Exception(-11, "Session not found");

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
                    /* Increment failure and return. */
                    increment_failures(hashSession);

                    return false;
                }
            }

            /* Check internal authenticate function. */
            if(!authenticate(strPIN, rSession))
            {
                /* Increment failure and return. */
                increment_failures(hashSession);

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
            ExtractHash(jParams, "session", default_session());

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


    /* Get the genesis-id of the given caller using session from params. */
    uint256_t Authentication::Caller(const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        return Caller(hashSession);
    }


    /* Get the genesis-id of the given caller using session. */
    uint256_t Authentication::Caller(const uint256_t& hashSession, const bool fThrow)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
        {
            /* Handle if we want to throw. */
            if(fThrow)
                throw Exception(-11, "Session not found");

            return uint256_t(0);
        }

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Set the caller from our session data. */
        return rSession.Genesis();
    }


    /* Determine if a sigchain is unlocked for given actions. */
    bool Authentication::UnlockStatus(const encoding::json& jParams, uint8_t &nRequestedActions)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        {
            RECURSIVE(MUTEX);

            /* Check for active session. */
            if(!mapSessions.count(hashSession))
                return TAO::Ledger::PinUnlock::UnlockActions::NONE;

            /* Get a copy of our current active session. */
            const Session& rSession =
                mapSessions[hashSession];

            /* Check for initializing sigchain. */
            if(rSession.fInitializing.load())
                return false;

            /* Check if actions are unlocked and allowed. */
            return rSession.Authorized(nRequestedActions);
        }

        return false;
    }


    /* Determine if a sigchain is unlocked for given actions. */
    bool Authentication::Unlocked(const uint8_t nRequestedActions, const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        return Unlocked(nRequestedActions, hashSession);
    }


    /* Determine if a sigchain is unlocked for given actions. */
    bool Authentication::Unlocked(const uint8_t nRequestedActions, const uint256_t& hashSession)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-11, "Session not found");

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Check for initializing sigchain. */
        if(rSession.fInitializing.load())
            return false;

        /* Check if actions are unlocked and allowed. */
        uint8_t nAllowedActions =
            nRequestedActions;

        /* Check that we have a valid pin number. */
        if(!rSession.Authorized(nAllowedActions))
            return false;

        return (nRequestedActions & nAllowedActions);
    }


    /* Get an instance of current session credentials indexed by session-id. */
    const memory::encrypted_ptr<TAO::Ledger::Credentials>& Authentication::Credentials(const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        return Credentials(hashSession);
    }


    /* Get an instance of current session credentials indexed by session-id. */
    const memory::encrypted_ptr<TAO::Ledger::Credentials>& Authentication::Credentials(const uint256_t& hashSession)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-9, "Session doesn't exist");

        /* Get a copy of our current active session. */
        return mapSessions[hashSession].Credentials();
    }


    /* List the currently active sessions in manager. */
    const std::vector<std::pair<uint256_t, uint256_t>> Authentication::Sessions(const int64_t nThread, const uint64_t nThreads)
    {
        /* Build our return vector. */
        std::vector<std::pair<uint256_t, uint256_t>> vRet;

        {
            RECURSIVE(MUTEX);

            /* Loop through sessions map. */
            for(const auto& rSession : mapSessions)
            {
                /* Grab a copy of our genesis. */
                const uint256_t hashGenesis =
                    rSession.second.Genesis();

                /* Check for no thread parameter. */
                if(nThread == -1)
                {
                    vRet.push_back(std::make_pair(rSession.first, hashGenesis));
                    continue;
                }

                /* Check if this is valid thread. */
                if(hashGenesis % nThreads == nThread)
                    vRet.push_back(std::make_pair(rSession.first, hashGenesis));
            }
        }

        return vRet;
    }


    /* Unlock and get the active pin from current session. */
    std::recursive_mutex& Authentication::Unlock(const encoding::json& jParams, SecureString &strPIN, const uint8_t nRequestedActions)
    {
        RECURSIVE(MUTEX);

        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-11, "Session not found");

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Check for initializing sigchain. */
        if(rSession.fInitializing.load())
            throw Exception(-139, "Cannot unlock while initializing dynamic indexing services: Check sessions/status/local");

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
                /* Increment failure and throw. */
                increment_failures(hashSession);

                throw Exception(-139, "Failed to unlock (Invalid Password)");
            }
        }

        /* Get the active pin if not currently stored. */
        if(CheckParameter(jParams, "pin", "string, number") || !rSession.Unlock(strPIN, nRequestedActions))
            strPIN = ExtractPIN(jParams);

        /* Check internal authenticate function. */
        if(!authenticate(strPIN, rSession))
        {
            /* Increment failure and throw. */
            increment_failures(hashSession);

            throw Exception(-139, "Failed to unlock (Invalid PIN)");
        }

        /* Get bytes of our session. */
        const std::vector<uint8_t> vSession =
            hashSession.GetBytes();

        /* Get an xxHash. */
        const uint64_t nHash =
            XXH64(&vSession[0], vSession.size(), 0);

        return vLocks[nHash % vLocks.size()];
    }


    /* Unlock and get the active pin from current session. */
    std::recursive_mutex& Authentication::Unlock(SecureString &strPIN, const uint8_t nRequestedActions, const uint256_t& hashSession)
    {
        RECURSIVE(MUTEX);

        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-11, "Session not found");

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Check for initializing sigchain. */
        if(rSession.fInitializing.load())
            throw Exception(-139, "Cannot unlock while initializing dynamic indexing services: Check sessions/status/local");

        /* Get the active pin if not currently stored. */
        if(!rSession.Unlock(strPIN, nRequestedActions))
            throw Exception(-139, "Failed to unlock (No PIN)");

        /* Check internal authenticate function. */
        if(!authenticate(strPIN, rSession))
        {
            /* Increment failure and throw. */
            increment_failures(hashSession);

            throw Exception(-139, "Failed to unlock (Invalid PIN)");
        }

        /* Get bytes of our session. */
        const std::vector<uint8_t> vSession =
            hashSession.GetBytes();

        /* Get an xxHash. */
        const uint64_t nHash =
            XXH64(&vSession[0], vSession.size(), 0);

        return vLocks[nHash % vLocks.size()];
    }


    /* Update the allowed actions for given pin. */
    void Authentication::Update(const encoding::json& jParams, const uint8_t nUpdatedActions, const SecureString& strPIN)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        /* Extract the PIN from parameters. */
        SecureString strNewPIN = strPIN;
        if(strNewPIN.empty())
            strNewPIN = ExtractPIN(jParams);

        {
            RECURSIVE(MUTEX);

            /* Check for active session. */
            if(!mapSessions.count(hashSession))
                throw Exception(-9, "Session doesn't exist");

            /* Get a copy of our current active session. */
            Session& rSession =
                mapSessions[hashSession];

            /* Check for initializing sigchain. */
            if(rSession.fInitializing.load())
                throw Exception(-139, "Cannot unlock while initializing dynamic indexing services: Check sessions/status/local");

            /* Update our internal session. */
            rSession.Update(strNewPIN, nUpdatedActions);
        }
    }

    /* Update the password internal credentials */
    void Authentication::Update(const encoding::json& jParams, const SecureString& strPassword)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        {
            RECURSIVE(MUTEX);

            /* Check for active session. */
            if(!mapSessions.count(hashSession))
                throw Exception(-9, "Session doesn't exist");

            /* Get a copy of our current active session. */
            Session& rSession =
                mapSessions[hashSession];

            /* Check for initializing sigchain. */
            if(rSession.fInitializing.load())
                throw Exception(-139, "Cannot unlock while initializing dynamic indexing services: Check sessions/status/local");

            /* Update our internal session. */
            rSession.Update(strPassword);
        }
    }


    /* Terminate an active session by parameters. */
    void Authentication::Terminate(const encoding::json& jParams)
    {
        /* Get the current session-id. */
        const uint256_t hashSession =
            ExtractHash(jParams, "session", default_session());

        /* We want to lock the mutex for this sigchain before allowing termination. */
        {
            /* Get bytes of our session. */
            const std::vector<uint8_t> vSession =
                hashSession.GetBytes();

            /* Get an xxHash. */
            const uint64_t nHash =
                XXH64(&vSession[0], vSession.size(), 0);

            RECURSIVE(vLocks[nHash % vLocks.size()]); //this will make sure transactions have finished processing
        }

        /* Terminate the session now. */
        RECURSIVE(MUTEX);
        terminate_session(hashSession);
    }


    /* Shuts down the current authentication systems. */
    void Authentication::Shutdown()
    {
        /* Make sure all authentication locks have been released. */
        for(auto& rLock : vLocks)
            RECURSIVE(rLock);

        {
            RECURSIVE(MUTEX);

            /* Copy our session keys before deleting. */
            std::set<uint256_t> setDelete;

            /* Copy our session keys to our delete set. */
            for(const auto& rSession : mapSessions)
                setDelete.insert(rSession.first);

            /* Iterate through session keys to terminate. */
            for(const auto& rDelete : setDelete)
                terminate_session(rDelete);
        }
    }


    /* Authenticate and get the active pin from current session. */
    bool Authentication::authenticate(const SecureString& strPIN, const Session& rSession)
    {
        /* Check that this is a local session. */
        if(rSession.Type() != Session::LOCAL)
            throw Exception(-9, "Only local sessions can be unlocked");

        /* Check for crypto object register. */
        const TAO::Register::Address hashCrypto =
            TAO::Register::Address(std::string("crypto"), rSession.Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the crypto object register. */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(hashCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            return false;

        /* Read the key type from crypto object register. */
        const uint256_t hashAuth =
            oCrypto.get<uint256_t>("auth");

        /* Check if the auth has is deactivated. */
        if(hashAuth == 0)
            throw Exception(-9, "auth key is disabled, please run profiles/create/auth to enable");

        /* Generate a key to check credentials against. */
        const uint256_t hashCheck =
            rSession.Credentials()->KeyHash("auth", 0, strPIN, hashAuth.GetType());

        /* Check for invalid authorization hash. */
        if(hashAuth != hashCheck)
            return false;

        /* Reset our activity and auth attempts if pin was successfully unlocked. */
        rSession.nAuthFailures = 0;
        rSession.nLastAccess   = runtime::unifiedtimestamp();

        return true;
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
    void Authentication::increment_failures(const uint256_t& hashSession)
    {
        /* Check for active session. */
        if(!mapSessions.count(hashSession))
            throw Exception(-9, "Session doesn't exist");

        /* Get a copy of our current active session. */
        const Session& rSession =
            mapSessions[hashSession];

        /* Increment the auth failures. */
        if(++rSession.nAuthFailures >= config::GetArg("-authattempts", 3))
        {
            /* Get a copy of our auth failures. */
            const uint64_t nAuthFailures =
                rSession.nAuthFailures.load();

            /* Terminate our internal session. */
            terminate_session(hashSession);

            /* Return failure to API endpoint. */
            throw Exception(-290, "Too many invalid password/pin attempts (",
                nAuthFailures, "): Session ", hashSession.SubString(), " Terminated");
        }
    }


    /* Checks for the correct session-id for single user mode. */
    uint256_t Authentication::default_session()
    {
        /* Handle for the default session. */
        if(!config::fMultiuser.load())
            return SESSION::DEFAULT;

        return ~uint256_t(0);
    }
}
