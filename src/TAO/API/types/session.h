/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>

#include <LLC/include/random.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** Session Class
         *
         *  Encapsulates the data held in memory for the duration of an API session (between user login and logout).
         *
         **/
        class Session
        {
            public:

                /** Default Constructor. **/
                Session();

                /** Copy constructor. **/
                Session(const Session& session) = delete;


                /** Move constructor. **/
                Session(Session&& session) noexcept ;


                /** Copy assignment. **/
                Session& operator=(const Session& session) = delete;


                /** Move assignment. **/
                Session& operator=(Session&& session) noexcept;


                /** Destructor. **/
                ~Session();


                /** Public mutex for creating transactions on signature chain. **/
                std::mutex CREATE_MUTEX;


                /** The txid of the transaction that was used to authenticate them on login.  This is cached so that we can
                    use this for subsequent authentication of transactions, unlock etc **/
                uint512_t hashAuth;


                /** Initialize
                 *
                 *  Initializes the session from username / password / pin
                 *
                 *  @param[in] pSigChain Signature chain of the user starting their session
                 *  @param[in] strPin Pin of the user starting their session
                 *  @param[in] nSessionID The ID for this session
                 *
                 *  @return The newly created session instance
                 *
                 **/
                void Initialize(const TAO::Ledger::SignatureChain& pUser,
                        const SecureString& strPin,
                        const uint256_t& nSessionID);

                /** IsNull
                 *
                 *  Returns true if the session has not been initialised.
                 *
                 **/
                bool IsNull() const;


                /** ID
                 *
                 *  Returns the session ID for this session.
                 *
                 **/
                uint256_t ID() const;


                /** GetActivePIN
                 *
                 *  Returns the active PIN for this session.
                 *
                 **/
                const memory::encrypted_ptr<TAO::Ledger::PinUnlock>& GetActivePIN() const;


                /** ClearActivePIN
                 *
                 *  Clears the active PIN for this session.
                 *
                 **/
                void ClearActivePIN() ;


                /** UpdatePassword
                 *
                 *  Updates the password stored in the internal sig chain
                 *
                 *  @param[in] strPassword The new password to set to update.
                 *
                 **/
                void UpdatePassword(const SecureString& strPassword);


                /** UpdatePIN
                 *
                 *  Updates the cached pin and its unlocked actions
                 *
                 *  @param[in] strPin The PIN to update.
                 *  @param[in] nUnlockedActions Bitmask of the unlocked actions for the pin.
                 *
                 **/
                void UpdatePIN(const SecureString& strPin, uint8_t nUnlockedActions);


                /** GetNetworkKey
                 *
                 *  Returns the cached network private key.
                 *
                 **/
                uint512_t GetNetworkKey() const;


                /** SetLastActive
                 *
                 *  Logs activity by updating the last active timestamp.
                 *
                 **/
                void SetLastActive();


                /** GetLastActive
                 *
                 *  Gets last active timestamp.
                 *
                 **/
                uint64_t GetLastActive() const;


                /** Locked
                 *
                 *  Determine if the currently active sig chain is locked.
                 *
                 **/
                bool Locked() const;


                /** CanTransact
                 *
                 *  Checks that the active sig chain has been unlocked to allow transactions.
                 *
                 **/
                bool CanTransact() const;


                /** CanMine
                 *
                 *  Checks that the active sig chain has been unlocked to allow mining
                 *
                 **/
                bool CanMine() const;


                /** CanStake
                 *
                 *  Checks that the active sig chain has been unlocked to allow staking
                 *
                 **/
                bool CanStake() const;


                /** CanProcessNotifications
                 *
                 *  Checks that the active sig chain has been unlocked to allow notifications to be processed
                 *
                 **/
                bool CanProcessNotifications() const;


                /** GetAccount
                 *
                 *  Returns the sigchain the account logged in.
                 *
                 *  @param[in] nSession The session identifier.
                 *
                 *  @return the signature chain.
                 *
                 **/
                const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& GetAccount() const;


                /** GetNetworkKey
                *
                *  Returns the private key for the network public key for a logged in session
                *
                *  @param[in] nSession The session identifier.
                *
                *  @return the private key for the auth public key
                *
                **/
                memory::encrypted_ptr<memory::encrypted_type<uint512_t>>& GetNetworkKey(const uint256_t& nSession) const;


                /** GetAuthAttempts
                 *
                 *  Returns the number of incorrect authentication attempts made in this session
                 *
                 **/
                uint8_t GetAuthAttempts() const;


                /** IncrementAuthAttempts
                 *
                 *  Increments the number of incorrect authentication attempts made in this session
                 *
                 **/
                void IncrementAuthAttempts() ;



                /** Save
                 *
                 *  Encrypts the current session and saves it to the local database
                 *
                 *  @param[in] strPin The pin to use to save the session.
                 *
                 **/
                void Save(const SecureString& strPin) const;



                /** Load
                 *
                 *  Decrypts and loads an existing session from disk
                 *
                 *  @param[in] nSessionID The new ID for this session
                 *  @param[in] hashGenesis The genesis hash of the user to load the session for.
                 *  @param[in] strPin The pin to use to load the session.
                 *
                 **/
                void Load(const uint256_t& nSessionID, const uint256_t& hashGenesis, const SecureString& strPin);



            private:

                /** The mutex for locking. **/
                mutable std::mutex MUTEX;


                /** The session ID **/
                uint256_t nID;


                /** Timstamp when the session started **/
                uint64_t nStarted;


                /** Timstamp when the session was last active **/
                uint64_t nLastActive;


                /** Number of incorrect authentication attempts recorded for this session **/
                uint8_t nAuthAttempts;


                /** Encrypted pointer of signature chain **/
                memory::encrypted_ptr<TAO::Ledger::SignatureChain> pSigChain;


                /** The active pin for sessionless API use **/
                memory::encrypted_ptr<TAO::Ledger::PinUnlock> pActivePIN;


                /** Cached network private key.  NOTE this is mutable as it is lazy loaded **/
                mutable memory::encrypted_ptr<memory::encrypted_type<uint512_t>> nNetworkKey;

        };

        /* null session reference for helper. */
        static Session null_session;

    }// end API namespace

}// end TAO namespace
