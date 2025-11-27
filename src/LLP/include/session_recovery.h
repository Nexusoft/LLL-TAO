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

namespace LLP
{
    /** SessionRecoveryData
     *
     *  Stores session state for recovery after connectivity drops.
     *  Allows miners to reconnect without full re-initialization.
     *
     **/
    struct SessionRecoveryData
    {
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

        /** Default Constructor **/
        SessionRecoveryData();

        /** Constructor from MiningContext **/
        explicit SessionRecoveryData(const MiningContext& context);

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
         *  Used when miner reconnects from same IP.
         *
         *  @param[in] strAddress Network address
         *  @param[out] context Recovered context if found
         *
         *  @return true if session recovered
         *
         **/
        bool RecoverSessionByAddress(const std::string& strAddress, MiningContext& context);

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
            /* Use first 8 bytes as hash */
            return static_cast<size_t>(key.Get64(0));
        }
    };

} // namespace LLP

#endif
