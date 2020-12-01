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
        
        /* Helper method to simplify session manager access */
        SessionManager& GetSessionManager()
        {
            return SessionManager::Instance();
        }

        /* Singleton access */
        SessionManager& SessionManager::Instance()
        {
            static SessionManager sessionManager;
            return sessionManager;
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
        Session& SessionManager::Add(const SecureString& strUsername, 
                        const SecureString& strPassword, 
                        const SecureString& strPin)
        {
            LOCK(MUTEX);

            /* If not in multiuser mode check that there is not already a session with ID 0 */
            if(!config::fMultiuser.load() && mapSessions.count(0) != 0)
                throw APIException(-140, "User already logged in");

            /* Generate a new session ID, or use ID 0 if in single user mode */
            uint256_t nSession = config::fMultiuser.load() ? LLC::GetRand256() : 0;

            /* Initialize the session instance */
            mapSessions[nSession].Initialize(strUsername, strPassword, strPin, nSession);

            /* Return the session instance */
            return mapSessions[nSession];
        }


        /* Remove a session from the manager */
        void SessionManager::Remove(const uint256_t& sessionID)
        {
            LOCK(MUTEX);

            if(mapSessions.count(sessionID) == 0)
                throw APIException(-11, "User not logged in");

            mapSessions.erase(sessionID);
        }

        /* Returns a session instance by session id */
        Session& SessionManager::Get(const uint256_t& sessionID, bool fLogActivity)
        {
            /* Lock the mutex before checking that it exists and then updating the last active time  */
            {
                LOCK(MUTEX);

                if(mapSessions.count(sessionID) == 0)
                    throw APIException(-11, "User not logged in");

                /* Update the activity if requested */
                if(fLogActivity)
                    mapSessions[sessionID].SetLastActive();
            }

            /* Return the session.  NOTE: we do this outside of the braces where the mutex is locked as we need to guarantee that
               the lock is released before returning.  Failure to do this can lead to deadlocks if subsequent methods are called on
               the returned session instance all in one line, as the mutex would remain locked until the stack unwinds from the 
               additional method calls. */
            return mapSessions[sessionID];
        }

        /* Checks to see if the session ID exists in session map */
        bool SessionManager::Has(const uint256_t& sessionID)
        {
            LOCK(MUTEX);
            return mapSessions.count(sessionID) > 0;
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
        void SessionManager::PurgeInactive(uint32_t nTimeout)
        {
            /* mutex and condition variable for the thread loop */
            std::mutex PURGE_MUTEX;
            std::condition_variable PURGE_CONDITION;

            while(!config::fShutdown.load())
            {
                std::unique_lock<std::mutex> lock(PURGE_MUTEX);
                
                /* Check the sessions every 10 seconds */
                PURGE_CONDITION.wait_for(lock, std::chrono::seconds(10), [this]{ return config::fShutdown.load();});

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return;

                /* Timestamp to determine if the session should be purged.  This is nTimeout minutes in the past from current time */
                uint64_t nPurgeTime = runtime::unifiedtimestamp() - nTimeout * 60;

                /* Vector of session ID's to purge.  First we identify those that are inactive and add them to this list, then we
                   gracefully log out each of the sessions */
                std::vector<uint256_t> vPurge;

                {
                    /* lock the session mutex so that we can check the timeout state of each */
                    LOCK(MUTEX);

                    /* Delete any sessions where the last activity time is earlier than nTimeout minutes ago */
                    auto session = mapSessions.begin();
                    while(session != mapSessions.end())
                    {
                        /* Check to see if the session last active timestamp is earlier than the purge time */
                        if(session->second.GetLastActive() < nPurgeTime)
                            /* Add the session ID to the purge list */
                            vPurge.push_back(session->first);
                        
                        /* increment iterator */
                        ++session;
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
