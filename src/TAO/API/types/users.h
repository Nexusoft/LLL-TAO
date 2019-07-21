/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Users API Class
         *
         *  Manages the function pointers for all Users commands.
         *
         **/
        class Users : public Base
        {

        private:

            /** The signature chain for login and logout. */
            mutable std::map<uint64_t, memory::encrypted_ptr<TAO::Ledger::SignatureChain> > mapSessions;


            /** The active pin for sessionless API use **/
            mutable memory::encrypted_ptr<TAO::Ledger::PinUnlock> pActivePIN;


            /** The mutex for locking. **/
            mutable std::mutex MUTEX;


            /** The mutex for events processing. **/
            mutable std::mutex EVENTS_MUTEX;


            /** The sigchain events processing thread. **/
            std::thread EVENTS_THREAD;


            /** The condition variable to awaken sleeping events thread. **/
            std::condition_variable CONDITION;


            /** the events flag for active oustanding events. **/
            std::atomic<bool> fEvent;


            /** the shutdown flag for gracefully shutting down events thread. **/
            std::atomic<bool> fShutdown;


        public:


            /** Default Constructor. **/
            Users();


            /** Destructor. **/
            ~Users();


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** LoggedIn
             *
             *  Determine if a sessionless user is logged in.
             *
             **/
            bool LoggedIn() const;


            /** Locked
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
             *  @param[in] fThrow Flag indicating whether to throw if the session is not found / not logged in.
             *
             *  @return The genesis ID if logged in.
             *
             **/
            uint256_t GetGenesis(uint64_t nSession, bool fThrow = false) const;


            /** GetCallersGenesis
             *
             *  Returns the genesis ID from the calling session or the the account logged in.
             *
             *  @param[in] params the parameters passed to the API method call.
             *
             *  @return The genesis ID if logged in.
             *
             **/
            uint256_t GetCallersGenesis(const json::json& params) const;


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
             *  @param[in] jsonParams The json array of parameters being passed to this method.
             *  @param[in] fThrow Flag to indicate whether this method should throw an exception
             *             if a valid session ID cannot be found.
             *
             *  @return the pin.
             *
             **/
            uint64_t GetSession(const json::json params, bool fThrow = true) const;



            /** RewriteURL
             *
             *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
             *  map directly to a function in the target API.  Insted this method can be overridden to
             *  parse the incoming URL and route to a different/generic method handler, adding parameter
             *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
             *  added to the jsonParams
             *  The return json contains the modifed method URL to be called.
             *
             *  @param[in] strMethod The name of the method being invoked.
             *  @param[in] jsonParams The json array of parameters being passed to this method.
             *
             *  @return the API method URL
             *
             **/
            std::string RewriteURL(const std::string& strMethod, json::json& jsonParams) override;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Users";
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


            /** Create
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


            /** Update
             *
             *  Update a user account credentials.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Update(const json::json& params, bool fHelp);


            /** Recovery
             *
             *  Create a user recovery passphrase.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Recovery(const json::json& params, bool fHelp);


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


            /** Assets
             *
             *  Get a list of assets owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Assets(const json::json& params, bool fHelp);


            /** Tokens
             *
             *  Get a list of tokens owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Tokens(const json::json& params, bool fHelp);


            /** Accounts
             *
             *  Get a list of accounts owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Accounts(const json::json& params, bool fHelp);


            /** Names
             *
             *  Get a list of names owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Names(const json::json& params, bool fHelp);


            /** Namespaces
             *
             *  Get a list of namespaces owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Namespaces(const json::json& params, bool fHelp);


            /** Items
             *
             *  Get a list of items (raw registers) owned by a signature chain
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Items(const json::json& params, bool fHelp);


            /** EventsThread
             *
             *  Background thread to handle/suppress sigchain notifications.
             *
             **/
             void EventsThread();


             /** NotifyEvent
              *
              *  Notifies the events processor that an event has occurred so it
              *  can check and update it's state.
              *
              **/
              void NotifyEvent();



              /** GetOutstanding
               *
               *  Gets the currently outstanding transactions that have not been matched with a credit or claim.
               *
               *  @param[in] hashGenesis The genesis hash for the sig chain owner.
               *  @param[out] vContracts The array of outstanding contracts.
               *
               **/
              bool GetOutstanding(const uint256_t& hashGenesis,
                    std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts);


          private:


              /** get_events
               *
               *  Get the outstanding debits and transfer transactions.
               *
               *  @param[in] hashGenesis The genesis hash for the sig chain owner.
               *  @param[in] hashLast The hash of the last transaction to iterate.
               *  @param[out] vContracts The array of outstanding contracts.
               *
               **/
              bool get_events(const uint256_t& hashGenesis,
                    uint512_t hashLast, std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts);


              /** get_coinbases
               *
               *  Get the outstanding coinbases.
               *
               *  @param[in] hashGenesis The genesis hash for the sig chain owner.
               *  @param[in] hashLast The hash of the last transaction to iterate.
               *  @param[out] vContracts The array of outstanding contracts.
               *
               **/
              bool get_coinbases(const uint256_t& hashGenesis,
                    uint512_t hashLast, std::vector<std::pair<uint32_t, TAO::Operation::Contract>> &vContracts);


        };
    }
}
