/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLP/types/p2p.h>

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


                /** Initialize
                 *
                 *  Initializes the session from username / password / pin
                 * 
                 *  @param[strUsername] Username of the user starting their session
                 *  @param[strPassword] Password of the user starting their session
                 *  @param[strPin] Pin of the user starting their session
                 *  @param[nSessionID] The ID for this session
                 * 
                 *  @return The newly created session instance
                 * 
                 **/
                void Initialize(const SecureString& strUsername, 
                        const SecureString& strPassword, 
                        const SecureString& strPin,
                        const uint256_t& nSessionID);


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


                /** AddP2PRequest
                *
                *  Adds a new P2P Request. 
                * 
                *  @param[in] request The request to add.
                *  @param[in] fIncoming Flag indicating whether this is an incoming or outgoing request .
                *
                **/
                void AddP2PRequest(const LLP::P2P::ConnectionRequest& request, bool fIncoming);


                /** GetP2PRequest
                *
                *  Gets P2P Request matching the app id / hashPeer criteria.  
                * 
                *  @param[in] request The application ID of the request to search for.
                *  @param[in] request The peer genesis hash of the request to search for.
                *  @param[in] fIncoming Flag indicating whether this is an incoming or outgoing request .
                *
                *  @return The connection request.  
                *
                **/
                LLP::P2P::ConnectionRequest GetP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming) const;


                /** HasP2PRequest
                *
                *  Checks to see if a P2P Request matching the app id / hashPeer criteria exists.  
                * 
                *  @param[in] request The application ID of the request to search for.
                *  @param[in] request The peer genesis hash of the request to search for.
                *  @param[in] fIncoming Flag indicating whether this is an incoming or outgoing request .
                *
                *  @return True if a matching connection exists.  
                *
                **/
                bool HasP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming) const;


                /** DeleteP2PRequest
                *
                *  Deletes the P2P Request matching the app id / hashPeer criteria exists.  
                * 
                *  @param[in] request The application ID of the request to search for.
                *  @param[in] request The peer genesis hash of the request to search for.
                *  @param[in] fIncoming Flag indicating whether this is an incoming or outgoing request .
                *
                **/
                void DeleteP2PRequest(const std::string& strAppID, const uint256_t& hashPeer, bool fIncoming);


                /** GetP2PRequests
                *
                *  Returns a vector of all connection requests.  NOTE: This will copy the internal vector to protect against 
                *  direct manipulation by calling code
                * 
                *  @param[in] fIncoming Flag indicating whether to return incoming or outgoing requests .
                *
                *  @return The vector of connection requests.  
                *
                **/
                const std::vector<LLP::P2P::ConnectionRequest> GetP2PRequests(bool fIncoming) const;



            private:

                /** The mutex for locking. **/
                mutable std::mutex MUTEX;

                /** The session ID **/
                uint256_t nID;

                
                /** Timstamp when the session started **/
                uint64_t nStarted;
                

                /** Timstamp when the session was last active **/
                uint64_t nLastActive;
                

                /** Encrypted pointer of signature chain **/
                memory::encrypted_ptr<TAO::Ledger::SignatureChain> pSigChain;


                /** The active pin for sessionless API use **/
                memory::encrypted_ptr<TAO::Ledger::PinUnlock> pActivePIN;


                /** Cached network private key **/
                memory::encrypted_ptr<memory::encrypted_type<uint512_t>> nNetworkKey;


                /** Vector of P2P connection requests that have been made TO this sig chain but not yet established **/
                std::vector<LLP::P2P::ConnectionRequest> vP2PIncoming; 


                /** Vector of P2P connection requests that have been made BY this sig chain but not yet established **/
                std::vector<LLP::P2P::ConnectionRequest> vP2POutgoing;


        };

        /* null session reference for helper. */
        static Session null_session;

    }// end API namespace

}// end TAO namespace