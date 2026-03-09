/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_RECOVERY_H
#define NEXUS_LLP_INCLUDE_SESSION_RECOVERY_H

#include <LLP/include/stateless_miner.h>
#include <Util/templates/concurrent_hashmap.h>

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <optional>

namespace LLP
{
    /** MinerSessionContainer
     *
     *  Authoritative per-miner session container for recovery after connectivity drops.
     *  Binds authentication, session identity, reward binding, ChaCha20 state,
     *  and lane-switch state into one recoverable record.
     *
     **/
    struct MinerSessionContainer
    {
        std::string miner_id;                   // Stable miner identity for diagnostics
        uint32_t nSessionId;                    // Session identifier
        uint256_t hashKeyID;                    // Falcon key identifier
        uint256_t hashGenesis;                  // Tritium genesis hash
        uint32_t nChannel;                      // Mining channel
        uint64_t nLastActivity;                 // Last activity timestamp
        uint64_t nCreated;                      // Session creation time
        std::vector<uint8_t> vPubKey;           // Miner's Falcon public key
        std::string strAddress;                 // Original connection address
        uint32_t nReconnectCount;               // Number of reconnection attempts
        bool fAuthenticated;                    // Authentication status
        bool fFreshAuth;                        // true = new auth on this node; false = session recovered
        uint8_t nLastLane;                      // Last known lane (0=Legacy, 1=Stateless)
        ProtocolLane nProtocolLane;             // Canonical protocol lane for this session
        uint256_t hashRewardAddress;            // Bound reward payout identity
        bool fRewardBound;                      // Whether reward binding is present
        std::vector<uint8_t> vChaCha20Key;      // Canonical ChaCha20 session key bytes
        bool fEncryptionReady;                  // Whether ChaCha20 is ready for use
        uint256_t hashChaCha20Key;              // Diagnostic/compatibility mirror of vChaCha20Key
                                                // Invariant: when vChaCha20Key is populated,
                                                // hashChaCha20Key mirrors uint256_t(vChaCha20Key).
                                                // When vChaCha20Key is empty, hashChaCha20Key may
                                                // still hold legacy cached state for fallback restore.
        uint64_t nChaCha20Nonce;                // ChaCha20 nonce counter
        std::vector<uint8_t> vDisposablePubKey; // Disposable Falcon session public key
        uint256_t hashDisposableKeyID;          // Disposable Falcon session key ID

        /** Default Constructor **/
        MinerSessionContainer();

        /** Constructor from MiningContext **/
        explicit MinerSessionContainer(const MiningContext& context);

        /** MergeContext
         *
         *  Merge live connection state into this authoritative container.
         *  Preserves already-captured reward/crypto state when the live context
         *  doesn't currently carry it.
         *
         **/
        void MergeContext(const MiningContext& context);

        /** ToContext
         *
         *  Restore a MiningContext from recovery data.
         *
         *  @return Restored MiningContext
         *
         **/
        MiningContext ToContext() const;

        /** IsExpired
         *
         *  Check if the session has expired.
         *
         *  @param[in] nTimeoutSec Timeout in seconds
         *
         *  @return true if expired
         *
         **/
        bool IsExpired(uint64_t nTimeoutSec = 3600) const;

        /** UpdateActivity
         *
         *  Update last activity timestamp.
         *
         **/
        void UpdateActivity();
    };

    using SessionRecoveryData = MinerSessionContainer;


    /** SessionRecoveryManager
     *
     *  Manages session state for reconnection support.
     *  Allows miners to resume sessions after temporary connectivity drops.
     *
     *  Features:
     *  - Thread-safe session storage using concurrent hash map
     *  - Automatic expiration of stale sessions
     *  - Falcon key-based session identification
     *  - Reconnection without re-authentication
     *
     **/
    class SessionRecoveryManager
    {
    public:
        /** Get
         *
         *  Get the global manager instance.
         *
         *  @return Reference to singleton manager
         *
         **/
        static SessionRecoveryManager& Get();

        /** SaveSession
         *
         *  Save session state for potential recovery.
         *
         *  @param[in] context Current session context
         *
         *  @return true if saved successfully
         *
         **/
        bool SaveSession(const MiningContext& context);

        /** MarkFreshAuth
         *
         *  Mark a saved session as having been authenticated fresh on this node
         *  (i.e., a full Falcon handshake, not a session recovery).
         *
         *  Called after a successful failover authentication to tag the session
         *  so that the Colin mining agent report can show `fresh_auth=true`.
         *
         *  @param[in] hashKeyID Falcon key identifier of the session to update
         *
         *  @return true if session was found and updated
         *
         **/
        bool MarkFreshAuth(const uint256_t& hashKeyID);

        /** RecoverSession
         *
         *  Attempt to recover a session by Falcon key ID.
         *
         *  @param[in] hashKeyID Falcon key identifier
         *  @param[out] context Recovered context if found
         *
         *  @return true if session recovered
         *
         **/
        bool RecoverSession(const uint256_t& hashKeyID, MiningContext& context);

        /** RecoverSessionByAddress
         *
         *  Attempt to recover a session by network address.
         *
         *  @param[in] strAddress Network address
         *  @param[out] context Recovered context if found
         *
         *  @return true if session recovered
         *
         **/
        bool RecoverSessionByAddress(const std::string& strAddress, MiningContext& context);

        /** RecoverSessionByAddress
         *
         *  Attempt to recover a session by network address.
         *
         *  @param[in] strAddress Network address
         *
         *  @return Optional recovery data if found
         *
         **/
        std::optional<SessionRecoveryData> RecoverSessionByAddress(const std::string& strAddress);

        /** RemoveSession
         *
         *  Remove a session from recovery storage.
         *
         *  @param[in] hashKeyID Falcon key identifier
         *
         *  @return true if removed
         *
         **/
        bool RemoveSession(const uint256_t& hashKeyID);

        /** UpdateSession
         *
         *  Update session with new context state.
         *
         *  @param[in] context Updated context
         *
         *  @return true if updated
         *
         **/
        bool UpdateSession(const MiningContext& context);

        /** SaveChaCha20State
         *
         *  Persist ChaCha20 session key and nonce for cross-lane recovery.
         *
         **/
        bool SaveChaCha20State(const uint256_t& hashKeyID, const uint256_t& hashKey, uint64_t nNonce);

        /** RestoreChaCha20State
         *
         *  Restore ChaCha20 session key and nonce for cross-lane recovery.
         *
         **/
        bool RestoreChaCha20State(const uint256_t& hashKeyID, uint256_t& hashKey, uint64_t& nNonce);

        /** SaveDisposableKey
         *
         *  Persist disposable Falcon session key data.
         *
         **/
        bool SaveDisposableKey(const uint256_t& hashKeyID, const std::vector<uint8_t>& vPubKey,
                               const uint256_t& hashDisposableKeyID);

        /** RestoreDisposableKey
         *
         *  Restore disposable Falcon session key data.
         *
         **/
        bool RestoreDisposableKey(const uint256_t& hashKeyID, std::vector<uint8_t>& vPubKey,
                                  uint256_t& hashDisposableKeyID);

        /** UpdateLane
         *
         *  Persist last known lane for session recovery coordination.
         *
         **/
        bool UpdateLane(const uint256_t& hashKeyID, uint8_t nNewLane);

        /** HasSession
         *
         *  Check if a recoverable session exists.
         *
         *  @param[in] hashKeyID Falcon key identifier
         *
         *  @return true if session exists and is not expired
         *
         **/
        bool HasSession(const uint256_t& hashKeyID) const;

        /** CleanupExpired
         *
         *  Remove expired sessions from storage.
         *
         *  @param[in] nTimeoutSec Session timeout in seconds
         *
         *  @return Number of sessions removed
         *
         **/
        uint32_t CleanupExpired(uint64_t nTimeoutSec = 3600);

        /** GetSessionCount
         *
         *  Get number of active recoverable sessions.
         *
         *  @return Number of sessions
         *
         **/
        size_t GetSessionCount() const;

        /** GetSessionTimeout
         *
         *  Get the session timeout in seconds.
         *
         *  @return Timeout value
         *
         **/
        uint64_t GetSessionTimeout() const;

        /** SetSessionTimeout
         *
         *  Set the session timeout in seconds.
         *
         *  @param[in] nTimeoutSec New timeout value
         *
         **/
        void SetSessionTimeout(uint64_t nTimeoutSec);

        /** GetMaxReconnects
         *
         *  Get maximum allowed reconnection attempts.
         *
         *  @return Max reconnects
         *
         **/
        uint32_t GetMaxReconnects() const;

        /** SetMaxReconnects
         *
         *  Set maximum allowed reconnection attempts.
         *
         *  @param[in] nMax New max value
         *
         **/
        void SetMaxReconnects(uint32_t nMax);

    private:
        /** Private constructor for singleton **/
        SessionRecoveryManager();

        /** Session storage by key ID **/
        util::ConcurrentHashMap<uint256_t, SessionRecoveryData> mapSessionsByKey;

        /** Address to key ID mapping for IP-based recovery **/
        util::ConcurrentHashMap<std::string, uint256_t> mapAddressToKey;

        /** Session timeout in seconds **/
        std::atomic<uint64_t> nSessionTimeout;

        /** Maximum reconnection attempts **/
        std::atomic<uint32_t> nMaxReconnects;
    };


    /** Hash function for uint256_t for use in unordered containers **/
    struct Uint256Hash
    {
        size_t operator()(const uint256_t& key) const
        {
            /* Combine multiple 64-bit segments for better distribution */
            size_t hash = static_cast<size_t>(key.Get64(0));
            hash ^= static_cast<size_t>(key.Get64(1)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<size_t>(key.Get64(2)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<size_t>(key.Get64(3)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

} // namespace LLP

#endif
