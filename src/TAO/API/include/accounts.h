/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_ACCOUNTS_H
#define NEXUS_TAO_API_INCLUDE_ACCOUNTS_H

#include <TAO/API/types/base.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Accounts API Class
         *
         *  Manages the function pointers for all Accounts commands.
         *
         **/
        class Accounts : public Base
        {
            /** The signature chain for login and logout. */
            mutable std::map<uint64_t, memory::encrypted_ptr<TAO::Ledger::SignatureChain>> mapSessions;


            /** The active pin for sessionless API use **/
            mutable memory::encrypted_ptr<TAO::Ledger::PinUnlock> pActivePIN;


            /** The mutex for locking. **/
            mutable std::mutex MUTEX;

        public:

            /** Default Constructor. **/
            Accounts()
            : mapSessions()
            , pActivePIN()
            , MUTEX()
            {
                Initialize();
            }


            /** Destructor. **/
            ~Accounts()
            {
                /* Iterate through the sessions map and delete any sig chains that are still active */
                for( auto& session : mapSessions)
                {
                    /* Check that is hasn't already been destroyed before freeing it*/
                    if( !session.second.IsNull())
                        session.second.free();
                }
                /* Clear the sessions map of all entries */
                mapSessions.clear();

                if( !pActivePIN.IsNull())
                    pActivePIN.free();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** LoggedIn Function
             *
             *  Determine if a sessionless user is logged in.
             *
             **/
            bool LoggedIn() const;


            /** Locked Function
             *
             *  Determine if the currently active sig chain is locked.
             *
             **/
            bool Locked() const;


            /** CanTransact
             *
             *  In sessionless API mode this method checks that the active sig chain has
             *  been unlocked to allow transactions.  If the account has not been specifically
             *  unlocked then we assume that they ARE allowed to transact, since the PIN would
             *  need to be provided in each API call.
             *
             **/
            bool CanTransact() const;


            /** CanMint
             *
             *  In sessionless API mode this method checks that the active sig chain has
             *  been unlocked to allow minting.
             *
             **/
            bool CanMint() const;


            /** GetKey
             *
             *  Returns a key from the account logged in.
             *
             *  @param[in] nKey The key nonce used.
             *  @param[in] strSecret The secret phrase to use.
             *  @param[in] nSession The session identifier.
             *
             **/
            uint512_t GetKey(uint32_t nKey, SecureString strSecret, uint64_t nSession) const;


            /** GetGenesis
             *
             *  Returns the genesis ID from the account logged in.
             *
             *  @param[in] nSession The session identifier.
             *
             *  @return The genesis ID if logged in.
             *
             **/
            uint256_t GetGenesis(uint64_t nSession) const;


            /** GetAccount
             *
             *  Returns the sigchain the account logged in.
             *
             *  @param[in] nSession The session identifier.
             *
             *  @return the signature chain.
             *
             **/
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& GetAccount(uint64_t nSession) const;

            /** GetActivePin
             *
             *  Returns the pin number for the currently logged in account.
             *
             *  @return the pin.
             *
             **/
            SecureString GetActivePin() const;

            /** GetPin
             *
             *  If the API is running in sessionless mode this method will return the currently
             *  active PIN (if logged in) or the pin from the params.  If not in sessionless mode
             *  then the method will return the pin from the params.  If no pin is available then
             *  an APIException is thrown
             *
             *  @return the pin.
             *
             **/
            SecureString GetPin(const json::json params) const;

            /** GetSession
             *
             *  If the API is running in sessionless mode this method will return the default
             *  session ID that is used to store the one and only session (ID 0). If the user is not
             *  logged in than an APIException is thrown.
             *  If not in sessionless mode then the method will return the session from the params.
             *  If the session is not is available in the params then an APIException is thrown.
             *
             *  @return the pin.
             *
             **/
            uint64_t GetSession(const json::json params) const;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Accounts";
            }


            /** Login
             *
             *  Login to a user account.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Login(const json::json& params, bool fHelp);


            /** Unlock
             *
             *  Unlock an account for mining (TODO: make this much more secure)
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Unlock(const json::json& params, bool fHelp);


            /** Lock
             *
             *  Lock an account for mining (TODO: make this much more secure)
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Lock(const json::json& params, bool fHelp);


            /** Logout
             *
             *  Logout of a user account
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Logout(const json::json& params, bool fHelp);


            /** CreateAccount
             *
             *  Create's a user account.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Create(const json::json& params, bool fHelp);


            /** GetTransactions
             *
             *  Get transactions for an account
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Transactions(const json::json& params, bool fHelp);


            /** Notifications
             *
             *  Get notifications for an account
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Notifications(const json::json& params, bool fHelp);
        };

        extern Accounts accounts;
    }
}

#endif
