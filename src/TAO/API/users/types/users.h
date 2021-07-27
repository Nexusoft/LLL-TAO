/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>
#include <TAO/API/users/types/notifications_processor.h>
#include <TAO/API/types/session.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <TAO/Register/types/state.h>

#include <Legacy/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>

#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>

/* Global TAO namespace. */
namespace TAO::API
{

    /** Users API Class
     *
     *  Manages the function pointers for all Users commands.
     *
     **/
    class Users : public Derived<Users>
    {
    private:

        /** The auto login thread. **/
        std::thread LOGIN_THREAD;


        /** Shared processing queue for managing notifications and -autotx. **/
        util::atomic::lock_shared_ptr<std::queue<TAO::Operation::Contract>> vProcessQueue;


    public:


        /** Default Constructor. **/
        Users();


        /** Destructor. **/
        ~Users();


        /** Notifications processor for the uers API **/
        NotificationsProcessor* NOTIFICATIONS_PROCESSOR;


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "users";
        }



        /** GetCallersGenesis
         *
         *  Returns the genesis ID from the calling session or the the account logged in.
         *
         *  @param[in] params the parameters passed to the API method call.
         *
         *  @return The genesis ID if logged in.
         *
         **/
        uint256_t GetCallersGenesis(const encoding::json& jParams) const;


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
        uint256_t GetGenesis(const uint256_t& nSession, const bool fThrow = false) const;



        /** GetPin
         *
         *  If the API is running in sessionless mode this method will return the currently
         *  active PIN (if logged in and unlocked for the requested action) or the pin from the params.
         *  If not in sessionless mode then the method will return the pin from the params.  If no pin is available for the
         *  given unlock action then an appropriate Exception is thrown
         *
         *  @param[in] params The API method parameters.
         *  @param[in] nUnlockAction The unlock
         *
         *  @return the pin.
         *
         **/
        SecureString GetPin(const encoding::json& jParams, const uint8_t nUnlockAction) const;


        /** GetSession
         *
         *  If the API is running in sessionless mode this method will return the default
         *  session that is used to store the one and only session (ID 0). If the user is not
         *  logged in than an Exception is thrown.
         *  If not in sessionless mode then the method will return the session from the params.
         *  If the session is not is available in the params then an Exception is thrown.
         *
         *  @param[in] jsonParams The json array of parameters being passed to this method.
         *  @param[in] fThrow Flag to indicate whether this method should throw an exception
         *             if a valid session ID cannot be found.
         *  @param[in] fLogActivity Flag indicating that this call should update the session activity timestamp
         *
         *  @return the session id.
         *
         **/
        Session& GetSession(const encoding::json& jParams, bool fThrow = true, bool fLogActivity = true) const;


        /** GetSession
         *
         *  Gets the session ID for a given genesis, if it is logged in on this node.
         *
         *  @param[in] hashGenesis The genesis hash to search for.
         *  @param[in] fLogActivity Flag indicating that this call should update the session activity timestamp
         *
         *  @return The session if the genesis is logged in, otherwise throws an exception
         **/
        Session& GetSession(const uint256_t& hashGenesis, bool fLogActivity = true) const;


        /** LoggedIn
         *
         *  Determine if a sessionless user is logged in.
         *
         **/
        bool LoggedIn() const;


        /** LoggedIn
         *
         *  Determine if a particular genesis is logged in on this node.
         *
         *  @param[in] hashGenesis The genesis hash to search for.
         *
         *  @return True if the hashGenesis is logged in on this node
         **/
        bool LoggedIn(const uint256_t& hashGenesis) const;


        /** GetKey
         *
         *  Returns a key from the account logged in.
         *
         *  @param[in] nKey The key nonce used.
         *  @param[in] strSecret The secret phrase to use.
         *  @param[in] nSession The session identifier.
         *
         **/
        uint512_t GetKey(const uint32_t nKey, const SecureString& strSecret, const Session& session) const;


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
        encoding::json Login(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Unlock(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Lock(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Logout(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Update(const encoding::json& jParams, const bool fHelp);


        /** Recover
         *
         *  Recover a sig chain and set new credentials by supplying the recovery seed.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Recover(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Transactions(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Notifications(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Assets(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Tokens(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Accounts(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Names(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Namespaces(const encoding::json& jParams, const bool fHelp);


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
        encoding::json Items(const encoding::json& jParams, const bool fHelp);


        /** Status
         *
         *  Get status information for the currently logged in user
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Status(const encoding::json& jParams, const bool fHelp);


        /** Invoices
         *
         *  Get a list of invoices owned by a signature chain
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Invoices(const encoding::json& jParams, const bool fHelp);


        /** ProcessNotifications
         *
         *  Process any outstanding notifications for a particular sig chain
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json ProcessNotifications(const encoding::json& jParams, const bool fHelp);


        /** Save
         *
         *  Saves the users session into the local DB so that it can be resumed later after a crash
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Save(const encoding::json& jParams, const bool fHelp);


        /** Load
         *
         *  Loads and resumes the users session from the local DB
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Load(const encoding::json& jParams, const bool fHelp);


        /** Has
         *
         *  Checks to see if a saves session exists in the local DB for the given user
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Has(const encoding::json& jParams, const bool fHelp);


        /** Processed
         *
         *  Lists the currently processed notifications.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Processed(const encoding::json& jParams, const bool fHelp);


        /** Clear
         *
         *  Clears the currently processed notifications.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Clear(const encoding::json& jParams, const bool fHelp);


        /** LoginThread
         *
         *  Background thread to auto login user once connections are established .
         *
         **/
        void LoginThread();


        /** GetOutstanding
         *
         *  Gets the currently outstanding transactions that have not been matched with a credit or claim.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] fIncludeSuppressed Flag indicating whether suppresed notifications should be included
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool GetOutstanding(const uint256_t& hashGenesis,
            const bool& fIncludeSuppressed,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** GetOutstanding
         *
         *  Gets the currently outstanding legacy UTXO to register transactions that have not been matched with a credit.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] fIncludeSuppressed Flag indicating whether suppresed notifications should be included
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool GetOutstanding(const uint256_t& hashGenesis,
            const bool& fIncludeSuppressed,
            std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> &vContracts);


        /** GetExpired
         *
         *  Gets the any debit or transfer transactions that have expired and can be voided.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] fIncludeSuppressed Flag indicating whether suppresed notifications should be included
         *  @param[out] vContracts The array of expired contracts.
         *
         **/
        static bool GetExpired(const uint256_t& hashGenesis,
            const bool& fIncludeSuppressed,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** get_tokenized_debits
         *
         *  Get the outstanding debit transactions made to assets owned by tokens you hold.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool get_tokenized_debits(const uint256_t& hashGenesis,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** get_coinbases
         *
         *  Get the outstanding coinbases.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] hashLast The hash of the last transaction to iterate.
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool get_coinbases(const uint256_t& hashGenesis,
            uint512_t hashLast, std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** get_expired
         *
         *  Get any debit or transfer contracts that have expired
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[in] hashLast The hash of the last transaction to iterate.
         *  @param[out] vContracts The array of contracts.
         *
         **/
        static bool get_expired(const uint256_t& hashGenesis,
            uint512_t hashLast, std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** get_unclaimed
         *
         *  Get any unclaimed transfer transactions.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[out] vContracts Vector of unclaimed transfer contracts.
         *
         **/
        static bool get_unclaimed(const uint256_t& hashGenesis,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** BlocksToMaturity
        *
        *  Determines whether the signature chain has reached desired maturity after the last coinbase/coinstake transaction
        *
        *  @param[in] hashGenesis The genesis hash for the sig chain owner.
        *
        *  @return The number of blocks remaining until it is mature
        *
        **/
        static uint32_t BlocksToMaturity(const uint256_t hashGenesis);


        /** CreateTransaction
        *
        *  Create a new transaction object for signature chain, if allowed to do so
        *
        *  @param[in] user The signature chain to generate this tx
        *  @param[in] pin The pin number to generate with.
        *  @param[out] tx The traansaction object being created
        *
        *  @return True of the transaction was successfully created
        *
        **/
        static bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                           TAO::Ledger::Transaction& tx, const uint8_t nScheme = TAO::Ledger::SIGNATURE::RESERVED);


        /** Authenticate
        *
        *  Checks that the session/password/pin parameters have been provided (where necessary) and then verifies that the
        *  password and pin are correct.
        *  If authentication fails then the AuthAttempts counter in the callers session is incremented
        *
        *  @param[in] params The JSON request parameters
        *
        *  @return True if the request contains the required authentication parameters and that they are correct
        *
        **/
        bool Authenticate(const encoding::json& jParams);


        /** TerminateSession
         *
         *  Gracefully closes down a users session
         *
         *  @param[in] nSession The parameters from the API call.
         *
         *
         **/
        void TerminateSession(const uint256_t& nSession);


        /** DownloadSigChain
         *
         *  Used for client mode, this method will download the signature chain transactions and events for a given genesis
         *
         *  @param[in] hashGenesis The genesis hash of the sig chain to synchronize for
         *  @param[in] fSyncEvents Flag indicating whether or not to also download events for the sig chain
         *
         *  @return Boolean indicating whether the download was successful
         *
         **/
        static bool DownloadSigChain(const uint256_t& hashGenesis, bool fSyncEvents); //XXX: this should be encapsulated, use static for now


      private:


        /** get_events
         *
         *  Get the outstanding debits and transfer transactions.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool get_events(const uint256_t& hashGenesis,
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> &vContracts);


        /** get_events
         *
         *  Get the outstanding legacy UTXO to register transactions.
         *
         *  @param[in] hashGenesis The genesis hash for the sig chain owner.
         *  @param[out] vContracts The array of outstanding contracts.
         *
         **/
        static bool get_events(const uint256_t& hashGenesis,
            std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> &vContracts);


        /** create_sig_chain
         *
         *  Creates a signature chain for the given credentials and returns the transaction object if successful
         *
         *  @param[in] strUsername The username.
         *  @param[in] strPassword The password.
         *  @param[in] strPin The pin.
         *  @param[out] tx The transaction object that was created.
         *
         **/
        void create_sig_chain(const SecureString& strUsername, const SecureString& strPassword,  const SecureString& strPin,
            TAO::Ledger::Transaction &tx);


        /** sanitize_contract
        *
        *  Checks that the contract passes both Build() and Execute()
        *
        *  @param[in] contract The contract to sanitize
        *  @param[in] mapStates map of register states used by Build()
        *
        *  @return True if the contract was sanitized without errors.
        *
        **/
        bool sanitize_contract(TAO::Operation::Contract& contract, std::map<uint256_t, TAO::Register::State> &mapStates );



        /** validate_transaction
        *
        *  Used when in client mode, this method will send the transaction to a peer to validate it.  This will in turn check
        *  each contract in the transaction to verify that the conditions are met, the contract can be built, and executed.
        *  If any of the contracts in the transaction fail then the method will return the index of the failed contract.
        *
        *  @param[in] tx The transaction to validate
        *  @param[out] nContract ID of the first failed contract
        *
        *  @return True if the transaction was validated without errors, false if an error was encountered.
        *
        **/
        bool validate_transaction(const TAO::Ledger::Transaction& tx, uint32_t& nContract);


        /** auto_login
        *
        *  Automatically logs in the sig chain using the credentials configured in the config file.  Will also create the sig
        *  chain if it doesn't exist and configured with autocreate=1
        *
        **/
        void auto_login();


        /** auto_process_notifications
        *
        *  Process notifications for the currently logged in user(s)
        *
        **/
        void auto_process_notifications();


        /** update_crypto_keys
        *
        *  Generates new keys in the Crypto object register for a signature chain, using the specified pin, and adds the update
        *  contract to the transaction.
        *
        *  @param[in] user The signature chain to update
        *  @param[in] strPin The secret value to use to generate the new private keys.
        *  @param[out] tx The transaction reference to add the update contract to
        *
        *  @return void.
        *
        **/
        void update_crypto_keys(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPin, TAO::Ledger::Transaction& tx );


    };
}
