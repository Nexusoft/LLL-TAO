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

#include <Util/include/mutex.h>

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
            /** Map of sessions to signature chain for session-based API use. */
            mutable std::map<uint64_t, TAO::Ledger::SignatureChain> mapSessions;


            /** The active pin for sessionless API use **/
            SecureString strActivePIN;


            /** The mutex for locking. **/
            mutable std::mutex MUTEX;

        public:

            /** Default Constructor. **/
            Accounts()
            : mapSessions()
            , strActivePIN("")
            , MUTEX()
            {
                Initialize();
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
            bool Locked(SecureString& strSecret) const;


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
              *  @param[out] user The user's account.
              *
              *  @return The genesis ID if logged in.
              *
              **/
             bool GetAccount(uint64_t nSession, TAO::Ledger::SignatureChain &user) const;


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
            json::json CreateAccount(const json::json& params, bool fHelp);


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
            json::json GetTransactions(const json::json& params, bool fHelp);
        };

        extern Accounts accounts;
    }
}

#endif
