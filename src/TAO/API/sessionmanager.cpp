/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/sessionmanager.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>
#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Single session manager pointer instead of a static reference, so that we can control destruction */
        SessionManager* SESSION_MANAGER = nullptr;

        /* Helper method to simplify session manager access */
        SessionManager& GetSessionManager()
        {
            return SessionManager::Instance();
        }

        /* Singleton access */
        SessionManager& SessionManager::Instance()
        {
            /* Lazy instantiate the session manager instance */
            if(SESSION_MANAGER == nullptr)
                SESSION_MANAGER = new SessionManager();

            return *SESSION_MANAGER;
        }


        /* Default Constructor. */
        SessionManager::SessionManager()
        : mapSessions()
        , MUTEX()
        , PURGE_THREAD()
        {
            /* Check to see if session timeout has been configured.  This value is in minutes */
            uint32_t nTimeout = config::GetArg("-sessiontimeout", 0);

            /* If a timeout has been set then start the Purge thread */
            if(nTimeout > 0)
                PURGE_THREAD = std::thread(std::bind(&SessionManager::PurgeInactive, this, nTimeout));
        }

        /* Default Destructor. */
        SessionManager::~SessionManager()
        {
            /* Wait for PURGE thread to complete */
            if(PURGE_THREAD.joinable())
                PURGE_THREAD.join();

            /* Lock session mutex one last time before clearing the sessions */
            LOCK(MUTEX);

            /* Clear all sessions */
            mapSessions.clear();
        }


        /* Creates and returns a new session */
        Session& SessionManager::Add(const TAO::Ledger::SignatureChain& pUser, const SecureString& strPin)
        {
            LOCK(MUTEX);

            /* If not in multiuser mode check that there is not already a session with ID 0 */
            if(!config::fMultiuser.load() && mapSessions.count(0) != 0)
                throw APIException(-140, "User already logged in");

            /* Generate a new session ID, or use ID 0 if in single user mode */
            uint256_t nSession = config::fMultiuser.load() ? LLC::GetRand256() : 0;

            /* Initialize the session instance */
            mapSessions[nSession].Initialize(pUser, strPin, nSession);

            /* Return the session instance */
            return mapSessions[nSession];
        }

        /* Decrypts and loads an existing session from disk */
        Session& SessionManager::Load(const uint256_t& hashGenesis, const SecureString& strPin)
        {
            LOCK(MUTEX);

            /* If not in multiuser mode check that there is not already a session with ID 0 */
            if(!config::fMultiuser.load() && mapSessions.count(0) != 0)
                throw APIException(-140, "User already logged in");

            /* Generate a new session ID, or use ID 0 if in single user mode */
            uint256_t nSession = config::fMultiuser.load() ? LLC::GetRand256() : 0;

            /* Initialize the session instance */
            try
            {
                mapSessions[nSession].Load(nSession, hashGenesis, strPin);

                /* Return the session instance */
                return mapSessions[nSession];
            }
            catch(const std::exception& ex)
            {
                /* Need to remove the session from the map if it failed to load */
                mapSessions.erase(nSession);
                return null_session;
            }
        }


        /* Remove a session from the manager */
        void SessionManager::Remove(const uint256_t& hashSession)
        {
            LOCK(MUTEX);

            if(mapSessions.count(hashSession) == 0)
                throw APIException(-11, "User not logged in");

            mapSessions.erase(hashSession);
        }


        /* Returns a session instance by session id */
        Session& SessionManager::Get(const uint256_t& hashSession, bool fLogActivity)
        {
            /* Lock the mutex before checking that it exists and then updating the last active time  */
            {
                LOCK(MUTEX);

                if(mapSessions.count(hashSession) == 0)
                    throw APIException(-11, "User not logged in");

                /* Update the activity if requested */
                if(fLogActivity)
                    mapSessions[hashSession].SetLastActive();
            }

            /* Return the session.  NOTE: we do this outside of the braces where the mutex is locked as we need to guarantee that
               the lock is released before returning.  Failure to do this can lead to deadlocks if subsequent methods are called on
               the returned session instance all in one line, as the mutex would remain locked until the stack unwinds from the
               additional method calls. */
            return mapSessions[hashSession];
        }


        /* Checks to see if the session ID exists in session map */
        bool SessionManager::Has(const uint256_t& hashSession)
        {
            LOCK(MUTEX);
            return mapSessions.count(hashSession) > 0;
        }


        /* Returns the number of active sessions in the session map */
        uint32_t SessionManager::Size()
        {
            LOCK(MUTEX);
            return mapSessions.size();
        }


        /* Destroys all sessions and removes them */
        void SessionManager::Clear()
        {
            LOCK(MUTEX);
            mapSessions.clear();
        }


        /* Destroys all sessions and removes them */
        void SessionManager::PurgeInactive(const uint32_t nTimeout)
        {
            /* Run in background to purge inactive sessions. */
            while(!config::fShutdown.load())
            {
                runtime::sleep(10000); //check sessions every 10 seconds

                /* Timestamp to determine if the session should be purged.  This is nTimeout minutes in the past from current time */
                uint64_t nPurgeTime = runtime::unifiedtimestamp() - nTimeout * 60;

                /* Vector of session ID's to purge. */
                std::vector<uint256_t> vPurge;

                {
                    /* lock the session mutex so that we can check the timeout state of each */
                    LOCK(MUTEX);

                    /* Delete any sessions where the last activity time is earlier than nTimeout minutes ago */
                    auto it = mapSessions.begin();
                    while(it != mapSessions.end())
                    {
                        /* Check to see if the session last active timestamp is earlier than the purge time */
                        if(it->second.GetLastActive() < nPurgeTime)
                            vPurge.push_back(it->first);

                        ++it;
                    }
                }

                /* Now iterate the purge lust and gracefully log out each session */
                for(const auto& nSession : vPurge)
                {
                    debug::log(3, FUNCTION, "Removing inactive API session: ", nSession.ToString() );

                    users->TerminateSession(nSession);
                }
            }
        }

    }
}
