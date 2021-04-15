/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/session.h>

#include <thread>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** SessionManager Class
         *
         *  Manages active API sessions.
         *
         **/
        class SessionManager
        {
            public:

                /* Singleton access */
                static SessionManager& Instance();


                /** Destructor. **/
                ~SessionManager();

                /* Delete copy constructor and assignment to guarantee only one session */
                SessionManager(const SessionManager&)  = delete;
                void operator=(const SessionManager&)  = delete;


                /** Add
                 *
                 *  Adds a session to the manager
                 *
                 *  @param[in] pUser Signature chain of the user starting their session
                 *  @param[in] strPin Pin of the user starting their session
                 *
                 *  @return The newly created session instance
                 *
                 **/
                Session& Add(const TAO::Ledger::SignatureChain& pUser, const SecureString& strPin);


                /** Load
                 *
                 *  Loads an existing session from disk and adds it to the session manager
                 *
                 *  @param[in] hashGenesis The genesis hash of the user to load the session for.
                 *  @param[in] strPin The pin to use to load the session.
                 *
                 *  @return The newly loaded session instance
                 **/
                Session& Load(const uint256_t& hashGenesis, const SecureString& strPin);


                /** Remove
                 *
                 *  Remove a session from the manager
                 *
                 *  @param[in] hashSession The session id to remove
                 *
                 *  @return The newly created session instance
                 *
                 **/
                void Remove(const uint256_t& hashSession);


                /** Get
                 *
                 *  Returns a session instance by session id
                 *
                 *  @param[in] hashSession The session id to search for
                 *  @param[in] fLogActivity Flag indicating that this call should update the session activity timestamp
                 *
                 *  @return The session instance
                 *
                 **/
                Session& Get(const uint256_t& hashSession, bool fLogActivity = true);


                /** Has
                 *
                 *  Checks to see if the session ID exists in session map
                 *
                 *  @param[in] hashSession The session id to search for
                 *
                 *  @return True if the session ID exists
                 *
                 **/
                bool Has(const uint256_t& hashSession);


                /** Size
                 *
                 *  Returns the number of active sessions in the session map
                 *
                 *  @return True if the session ID exists
                 *
                 **/
                uint32_t Size();


                /** Clear
                 *
                 *  Destroys all sessions and removes them
                 *
                 **/
                void Clear();


                /* Map of session objects to session ID */
                std::map<uint256_t, Session> mapSessions;


                /* Mutex to control access to the session map */
                std::mutex MUTEX;


            private:

                /** Default Constructor made private as access should be via singleton. **/
                SessionManager();


                /** PurgeInactive
                 *
                 *  Removes any sessions that have been inactive for longer than the session timeout.
                 *
                 *  @param[in] nTimeout The timeout in minutes to determine if sessions are inactive
                 *
                 **/
                void PurgeInactive(const uint32_t nTimeout);


                /* Thread to purge inactive sessions */
                std::thread PURGE_THREAD;


        };

        /* Helper method to simplify session manager access */
        extern SessionManager& GetSessionManager();

        /* Single session manager pointer instead of a static reference, so that we can control destruction */
        extern SessionManager* SESSION_MANAGER;


    }// end API namespace

}// end TAO namespace
