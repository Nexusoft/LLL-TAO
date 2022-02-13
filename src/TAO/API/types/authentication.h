/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/sigchain.h>

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

        /** @class
         *
         *  Class to hold session related data for quick access.
         *
         **/
        class Session
        {
            /** Our active sigchain object. **/
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> pCredentials;


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
                NETWORK = 3,
            };


            /** Track failed authentication attempts. **/
            mutable std::atomic<uint64_t> nAuthFailures;


            /** Default Constructor. **/
            Session()
            : pCredentials  (nullptr)
            , hashGenesis   (0)
            , nType         (LOCAL)
            , nAuthFailures (0)
            {
            }


            /** Constructor based on geneis. **/
            Session(const SecureString& strUsername, const SecureString& strPassword, const uint8_t nTypeIn = LOCAL)
            : pCredentials  (new TAO::Ledger::SignatureChain(strUsername, strPassword))
            , hashGenesis   (pCredentials->Genesis())
            , nType         (nTypeIn)
            , nAuthFailures (0)
            {
            }


            /** Default Destructor. **/
            ~Session()
            {
                pCredentials.free();
            }


            /** Credentials
             *
             *  Retrieve the active credentials from the current session.
             *
             *  @return The signature chain credentials.
             *
             **/
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Credentials() const
            {
                return pCredentials;
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
        };


        /** Initialize
         *
         *  Initializes the current authentication systems.
         *
         **/
        static void Initialize();


        /** Authenticate
         *
         *  Authenticate a new session from incoming parameters.
         *
         *  @param[in] jParams The parameters to authenticate from.
         *
         *  @return the return json data if authentication succeeded.
         *
         **/
        static encoding::json Authenticate(const encoding::json& jParams);


        /** Active
         *
         *  Check if user is already authenticated by genesis-id.
         *
         *  @param[in] hashGenesis The current genesis-id to lookup for.
         *  @param[in] hashSession The current session-id if logged in.
         *
         *  @return true if session is authenticated.
         *
         **/
        static bool Active(const uint256_t& hashGenesis, uint256_t &hashSession);


        /** Authenticated
         *
         *  Check if user is already authenticated by parameters.
         *
         *  @param[in] jParams The incoming parameters for lookup.
         *
         *  @return true if session is authenticated.
         *
         **/
        static bool Authenticated(const encoding::json& jParams);


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


    };
}
