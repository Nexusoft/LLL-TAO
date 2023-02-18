/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <Util/include/json.h>
#include <Util/include/mutex.h>
#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class
     *
     *  This class is responsible for handling all authentication requests to verify credentials against sessions.
     *
     **/
    class Authentication
    {
    public:

        /* Enum to track default session indexes. */
        enum SESSION : uint8_t
        {
            DEFAULT  = 1,
            MINER    = 2,
            INVALID  = 255,
        };


        /** @class
         *
         *  Class to hold session related data for quick access.
         *
         **/
        class Session
        {
            /** Our active sigchain object. **/
            memory::encrypted_ptr<TAO::Ledger::Credentials> pCredentials;


            /** Our active pin unlock object. **/
            memory::encrypted_ptr<TAO::Ledger::PinUnlock> pUnlock;


            /** Cache of the genesis-id for this session. **/
            uint256_t hashGenesis;


            /** Cache of current session type. **/
            uint8_t nType;


        public:


            /* Enum to track session type. */
            enum : uint8_t
            {
                EMPTY   = 0,
                LOCAL   = 1,
                REMOTE  = 2,
            };


            /** Track failed authentication attempts. **/
            mutable std::atomic<uint64_t> nAuthFailures;


            /** Track the last time session was active. **/
            mutable std::atomic<uint64_t> nLastAccess;


            /** Track if session is currently initializing. **/
            mutable std::atomic<bool> fInitializing;


            /** Default Constructor. **/
            Session()
            : pCredentials  (nullptr)
            , pUnlock       (nullptr)
            , hashGenesis   (0)
            , nType         (LOCAL)
            , nAuthFailures (0)
            , nLastAccess   (runtime::unifiedtimestamp())
            , fInitializing (true)
            {
            }


            /** Copy Constructor. **/
            Session(const Session& rSession) = delete;


            /** Move Constructor. **/
            Session(Session&& rSession)
            : pCredentials  (std::move(rSession.pCredentials))
            , pUnlock       (std::move(rSession.pUnlock))
            , hashGenesis   (std::move(rSession.hashGenesis))
            , nType         (std::move(rSession.nType))
            , nAuthFailures (rSession.nAuthFailures.load())
            , nLastAccess   (rSession.nLastAccess.load())
            , fInitializing (rSession.fInitializing.load())
            {
                /* We wnat to reset our pointers here so they don't get sniped. */
                rSession.pCredentials.SetNull();
                rSession.pUnlock.SetNull();
            }


            /** Copy Assignment. **/
            Session& operator=(const Session& rSession) = delete;


            /** Move Assignment. **/
            Session& operator=(Session&& rSession)
            {
                pCredentials  = std::move(rSession.pCredentials);
                pUnlock       = std::move(rSession.pUnlock);
                hashGenesis   = std::move(rSession.hashGenesis);
                nType         = std::move(rSession.nType);
                nAuthFailures = rSession.nAuthFailures.load();
                nLastAccess   = rSession.nLastAccess.load();
                fInitializing = rSession.fInitializing.load();

                /* We wnat to reset our pointers here so they don't get sniped. */
                rSession.pCredentials.SetNull();
                rSession.pUnlock.SetNull();

                return *this;
            }


            /** Constructor based on geneis. **/
            Session(const SecureString& strUsername, const SecureString& strPassword, const uint8_t nTypeIn = LOCAL)
            : pCredentials  (new TAO::Ledger::Credentials(strUsername, strPassword))
            , pUnlock       (new TAO::Ledger::PinUnlock())
            , hashGenesis   (pCredentials->Genesis())
            , nType         (nTypeIn)
            , nAuthFailures (0)
            , nLastAccess   (runtime::unifiedtimestamp())
            , fInitializing (true)
            {
            }


            /** Default Destructor. **/
            ~Session()
            {
                /* Cleanup the credentials object. */
                if(!pCredentials.IsNull())
                    pCredentials.free();

                /* Cleanup the pin unlock object. */
                if(!pUnlock.IsNull())
                    pUnlock.free();
            }


            /** Credentials
             *
             *  Retrieve the active credentials from the current session.
             *
             *  @return The signature chain credentials.
             *
             **/
            const memory::encrypted_ptr<TAO::Ledger::Credentials>& Credentials() const
            {
                return pCredentials;
            }


            /** Authorized
             *
             *  Check if given sigchain was authorized for given actions.
             *
             *  @param[out] nRequestedActions The actions to check.
             *
             *  @return true if the actions were authroized.
             *
             **/
            bool Authorized(uint8_t &nRequestedActions) const
            {
                /* Check that we have initialized our PIN. */
                if(pUnlock.IsNull())
                    return false;

                /* Set our return by reference. */
                nRequestedActions =
                    pUnlock->UnlockedActions();

                return true;
            }


            /** Unlock
             *
             *  Unlock and get the active pin from current session.
             *
             *  @param[out] strPIN The pin number to return by reference
             *  @param[in] nRequestedActions The actions requested for PIN unlock.
             *
             *  @return The secure string containing active pin.
             *
             **/
            bool Unlock(SecureString &strPIN, const uint8_t nRequestedActions) const
            {
                /* Check that we have initialized our PIN. */
                if(pUnlock.IsNull())
                    return false;

                /* Check against our unlock actions. */
                if(pUnlock->UnlockedActions() & nRequestedActions)
                {
                    /* Get a copy of our secure string. */
                    strPIN = pUnlock->PIN();
                    if(strPIN.empty())
                        return false;

                    return true;
                }

                return false;
            }


            /** Update
             *
             *  Update our PIN object with new unlocked actions.
             *
             *  @param[in] strPIN The pin number to update.
             *  @param[in] nUpdatedActions The actions to update.
             *
             **/
            void Update(const SecureString& strPIN, const uint8_t nUpdatedActions)
            {
                /* Update the internal PIN object. */
                pUnlock->Update(strPIN, nUpdatedActions);
            }


            /** Update
             *
             *  Update our password internal to our credentials.
             *
             *  @param[in] strPassword The passowrd we are updating to
             *
             **/
            void Update(const SecureString& strPassword)
            {
                /* Update the internal credentials object. */
                pCredentials->Update(strPassword);
            }


            /** Genesis
             *
             *  Get the genesis-id of the current session.
             *
             *  @return the genesis-id of logged in session.
             *
             **/
            uint256_t Genesis() const
            {
                return hashGenesis;
            }


            /** Type
             *
             *  Get the current session type
             *
             *  @return the genesis-id of logged in session.
             *
             **/
            uint8_t Type() const
            {
                return nType;
            }


            /** Accessed
             *
             *  Get the last timestamp session was accessed.
             *
             *  @return The unix timestamp of last session access.
             *
             **/
            uint64_t Accessed() const
            {
                return nLastAccess.load();
            }


            /** Active
             *
             *  Get the number of seconds a session has been active.
             *
             *  @return The number of seconds session has been active.
             *
             **/
            uint64_t Active() const
            {
                return (runtime::unifiedtimestamp() - nLastAccess.load());
            }
        };


        /** Initialize
         *
         *  Initializes the current authentication systems.
         *
         **/
        static void Initialize();


        /** Insert
         *
         *  Insert a new session into authentication map.
         *
         *  @param[in] hashSession The session identifier to add by index.
         *  @param[in] rSession The session to add to the map.
         *
         **/
        static void Insert(const uint256_t& hashSession, Session& rSession);


        /** Ready
         *
         *  Lets everything know that session is ready to be used.
         *
         *  @param[in] hashSession The session identifier to add by index.
         *
         **/
        static void SetReady(const uint256_t& hashSession);


        /** Active
         *
         *  Check if user is already authenticated by genesis-id and return the session.
         *
         *  @param[in] hashGenesis The current genesis-id to lookup for.
         *  @param[out] hashSession The current session-id if logged in.
         *
         *  @return true if session is authenticated.
         *
         **/
        static bool Active(const uint256_t& hashGenesis, uint256_t &hashSession);


        /** Active
         *
         *  Check if user is already authenticated by genesis-id.
         *
         *  @param[in] hashGenesis The current genesis-id to lookup for.
         *
         *  @return true if session is authenticated.
         *
         **/
        static bool Active(const uint256_t& hashGenesis);


        /** Accessed
         *
         *  Get the last time that session was accessed
         *
         *  @param[in] hashSession The session identifier to check accessed with.
         *
         *  @return the timestamp of last session access
         *
         **/
        static uint64_t Accessed(const encoding::json& jParams);


        /** Accessed
         *
         *  Get the last time that session was accessed
         *
         *  @param[in] jParams The parameters to check against.
         *
         *  @return the timestamp of last session access
         *
         **/
        static uint64_t Accessed(const uint256_t& hashSession);


        /** Indexing
         *
         *  Checks if a session is ready to be used and not indexing
         *
         *  @param[in] hashSession The session identifier to add by index.
         *
         **/
        static bool Indexing(const encoding::json& jParams);


        /** Authenticate
         *
         *  Authenticate a user's credentials against their sigchain.
         *  This function requires the PIN to be inputed.
         *
         *  @param[in] jParams The parameters to check against.
         *
         *  @return true if sigchain was authenticated.
         *
         **/
        static bool Authenticate(const encoding::json& jParams);


        /** Caller
         *
         *  Get the genesis-id of the given caller using session from params.
         *
         *  @param[in] jParams The incoming parameters to parse session from.
         *  @param[out] hashCaller The genesis-id of the caller
         *
         *  @return True if the caller was found
         *
         **/
        static bool Caller(const encoding::json& jParams, uint256_t &hashCaller);


        /** Unloacked
         *
         *  Determine if a sigchain is unlocked for given actions.
         *
         *  @param[in] jParams The incoming paramters to parse
         *  @param[in] nRequestedActions The actions requested for PIN unlock.
         *
         *  @return true if the PIN is unlocked for given actions.
         *
         **/
        static bool UnlockStatus(const encoding::json& jParams, uint8_t &nRequestedActions);


        /** Unloacked
         *
         *  Determine if a sigchain is unlocked for given actions.
         *
         *  @param[in] jParams The incoming paramters to parse
         *  @param[in] nRequestedActions The actions requested for PIN unlock.
         *
         *  @return true if the PIN is unlocked for given actions.
         *
         **/
        static bool Unlocked(const uint8_t nRequestedActions, const encoding::json& jParams);


        /** Unloacked
         *
         *  Determine if a sigchain is unlocked for given actions.
         *
         *  @param[in] hashSession The given session-id to check our status for.
         *  @param[in] nRequestedActions The actions requested for PIN unlock.
         *
         *  @return true if the PIN is unlocked for given actions.
         *
         **/
        static bool Unlocked(const uint8_t nRequestedActions, const uint256_t& hashSession = default_session());


        /** Caller
         *
         *  Get the genesis-id of the given caller using session from params. Throws exception if not found.
         *
         *  @param[in] jParams The incoming parameters to parse session from.
         *
         *  @return the caller if found
         *
         **/
        static uint256_t Caller(const encoding::json& jParams);


        /** Caller
         *
         *  Get the genesis-id of the given caller using session. Throws exception if not found.
         *
         *  @param[in] hashSession The session-id to extract genesis from.
         *  @param[in] fThrow Flag to tell if we want to throw on error.
         *
         *  @return the caller if found
         *
         **/
        static uint256_t Caller(const uint256_t& hashSession = default_session(), const bool fThrow = true);


        /** Credentials
         *
         *  Get an instance of current session credentials indexed by session-id.
         *  This will throw if session not found, do not use without checking first.
         *
         *  @param[in] jParams The incoming parameters.
         *
         *  @return The active session.
         *
         **/
        static const memory::encrypted_ptr<TAO::Ledger::Credentials>& Credentials(const encoding::json& jParams);


        /** Credentials
         *
         *  Get an instance of current session credentials indexed by session-id.
         *  This will throw if session not found, do not use without checking first.
         *
         *  @param[in] hashSession The session parameter required.
         *
         *  @return The active session.
         *
         **/
        static const memory::encrypted_ptr<TAO::Ledger::Credentials>& Credentials(const uint256_t& hashSession = default_session());


        /** Sessions
         *
         *  List the currently active sessions in manager.
         *
         *  @param[in] nThread The thread-id to filter by if applicable.
         *
         *  @return the list of sessions if any found.
         *
         **/
        static const std::vector<std::pair<uint256_t, uint256_t>> Sessions(const int64_t nThread = -1, const uint64_t nSize = 0);


        /** Unlock
         *
         *  Unlock and get the active pin from current session.
         *
         *  @param[in] jParams The incoming paramters to parse
         *  @param[out] strPIN The pin number to return by reference
         *  @param[in] nRequestedActions The actions requested for PIN unlock.
         *
         *  @return True if this unlock action was successful.
         *
         **/
        static std::recursive_mutex& Unlock(const encoding::json& jParams, SecureString &strPIN,
                           const uint8_t nRequestedActions = TAO::Ledger::PinUnlock::TRANSACTIONS);


       /** Unlock
        *
        *  Unlock and get the active pin from current session.
        *
        *  @param[in] hashSession The incoming session identifier
        *  @param[out] strPIN The pin number to return by reference
        *  @param[in] nRequestedActions The actions requested for PIN unlock.
        *
        *  @return True if this unlock action was successful.
        *
        **/
       static std::recursive_mutex& Unlock(SecureString &strPIN, const uint8_t nRequestedActions,
                                                                 const uint256_t& hashSession = default_session());


        /** Update
         *
         *  Update the allowed actions for given pin
         *
         *  @param[in] jParams the incoming parameters to parse
         *  @param[in] nUpdatedActions The actions allowed for PIN unlock.
         *
         **/
        static void Update(const encoding::json& jParams, const uint8_t nUpdatedActions, const SecureString& strPIN = "");


        /** Update
         *
         *  Update the password internal credentials
         *
         *  @param[in] jParams the incoming parameters to parse
         *  @param[in] strPassword The new password to update.
         *
         **/
        static void Update(const encoding::json& jParams, const SecureString& strPassword);


        /** Terminate
         *
         *  Terminate an active session by parameters.
         *
         *  @param[in] jParams The incoming parameters for lookup.
         *
         **/
        static void Terminate(const encoding::json& jParams);


        /** Shutdown
         *
         *  Shuts down the current authentication systems.
         *
         **/
        static void Shutdown();


    private:


        /** Mutex to lock around critical data. **/
        static std::recursive_mutex MUTEX;


        /** Map to hold session related data. **/
        static std::map<uint256_t, Session> mapSessions;


        /** Vector of our lock objects for session level locks. **/
        static std::vector<std::recursive_mutex> vLocks;


        /** authenticate
         *
         *  Authenticate and get the active pin from current session.
         *
         *  @param[in] strPIN The pin number to return by reference.
         *  @param[in rSession The session object ot authenticate]
         *
         *  @return True if given crecentials were correct.
         *
         **/
        static bool authenticate(const SecureString& strPIN, const Session& rSession);


        /** terminate_session
         *
         *  Terminate an active session by parameters.
         *
         *  @param[in] hashSession The incoming session to terminate.
         *
         **/
        static void terminate_session(const uint256_t& hashSession);


        /** increment_failures
         *
         *  Increment the failure counter to deauthorize user after failed auth.
         *
         *  @param[in] hashSession The incoming session to terminate.
         *
         **/
        static void increment_failures(const uint256_t& hashSession);


        /** default_session
         *
         *  Checks for the correct session-id for single user mode.
         *
         **/
        __attribute__((const)) static uint256_t default_session();
    };
}
