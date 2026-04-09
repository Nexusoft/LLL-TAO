/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CANONICAL_SESSION_H
#define NEXUS_LLP_INCLUDE_CANONICAL_SESSION_H

#include <LLP/include/canonical_chain_state.h>
#include <LLP/include/push_notification.h>      /* ProtocolLane */
#include <LLP/include/stateless_miner.h>         /* MinerSessionState, SessionBinding, CryptoContext, MiningContext */
#include <LLC/types/uint1024.h>
#include <LLC/include/flkey.h>                   /* LLC::FalconVersion */

#include <array>
#include <cstdint>
#include <string>
#include <vector>


namespace LLP
{

    /** CanonicalSession
     *
     *  Unified session state for a single mining identity (one Falcon key).
     *  Merges all fields from MiningContext and NodeSessionEntry
     *  into a single canonical record keyed by hashKeyID.
     *
     *  This struct is the value type for the primary SessionStore map:
     *      ConcurrentHashMap<uint256_t, CanonicalSession, Uint256Hash>
     *
     *  DESIGN PRINCIPLES:
     *  ==================
     *  - One Falcon identity (hashKeyID) → one canonical record.
     *  - All mutations go through SessionStore::Transform() (per-bucket lock).
     *  - No std::atomic fields needed — bucket lock provides atomicity.
     *  - Recovery is a flag flip on the same record, not a copy to a separate store.
     *  - Health tracking (failed packets, disconnected flag) is inline, not external.
     *
     *  COMPATIBILITY:
     *  ==============
     *  ToMiningContext() produces a MiningContext for callers that still expect
     *  the old type.  FromMiningContext() constructs a CanonicalSession from
     *  a MiningContext (used during migration).
     *
     **/
    struct CanonicalSession
    {
        /* ── Identity (immutable after creation) ────────────────────────────── */
        uint256_t   hashKeyID;               ///< Primary key (Falcon public key hash)
        uint32_t    nSessionId        = 0;   ///< Lower 32 bits of hashKeyID
        uint256_t   hashGenesis;             ///< Tritium genesis hash
        std::string strUserName;             ///< Username for trust-based addressing

        /* ── Connection ─────────────────────────────────────────────────────── */
        std::string  strAddress;             ///< IP:port (mutable on reconnect)
        std::string  strIP;                  ///< Port-agnostic IP (derived from strAddress)
        ProtocolLane nProtocolLane = ProtocolLane::STATELESS;
        bool         fStatelessLive   = false;
        bool         fLegacyLive      = false;

        /* ── Lifecycle (state machine) ──────────────────────────────────────── */
        MinerSessionState nSessionState = MinerSessionState::CONNECTED;
        bool     fAuthenticated   = false;   ///< Deprecated alias — use nSessionState
        bool     fEncryptionReady = false;   ///< Deprecated alias — use nSessionState
        uint64_t nSessionStart    = 0;
        uint64_t nLastActivity    = 0;
        uint64_t nTimestamp        = 0;

        /* ── Channel / Mining ───────────────────────────────────────────────── */
        uint32_t    nChannel                  = 0;
        std::string strChannelName;
        uint32_t    nHeight                   = 0;
        uint32_t    nLastTemplateUnifiedHeight = 0;
        uint1024_t  hashLastBlock;
        CanonicalChainState canonical_snap;
        uint32_t    nStakeHeight              = 0;
        std::array<uint8_t, 4> nMinerPrevblockSuffix = {0, 0, 0, 0};

        /* ── Auth / Crypto ──────────────────────────────────────────────────── */
        std::vector<uint8_t> vAuthNonce;
        std::vector<uint8_t> vMinerPubKey;
        LLC::FalconVersion   nFalconVersion        = LLC::FalconVersion::FALCON_512;
        bool                 fFalconVersionDetected = false;
        std::vector<uint8_t> vChaChaKey;
        uint64_t             nChaCha20Nonce         = 0;
        std::vector<uint8_t> vDisposablePubKey;
        uint256_t            hashDisposableKeyID;
        uint256_t            hashChaCha20Key;        ///< Diagnostic mirror

        /* ── Reward ─────────────────────────────────────────────────────────── */
        uint256_t hashRewardAddress;
        bool      fRewardBound = false;

        /* ── Notifications / Push ───────────────────────────────────────────── */
        bool     fSubscribedToNotifications = false;
        uint32_t nSubscribedChannel         = 0;
        uint64_t nLastNotificationTime      = 0;
        uint64_t nNotificationsSent         = 0;

        /* ── Keepalive ──────────────────────────────────────────────────────── */
        uint32_t nKeepaliveCount    = 0;
        uint32_t nKeepaliveSent     = 0;
        uint64_t nLastKeepaliveTime = 0;


        /* ── Health (replaces ActiveSessionBoard) ───────────────────────────── */
        uint32_t nFailedPackets      = 0;
        uint64_t nLastSuccessfulSend = 0;
        bool     fMarkedDisconnected = false;

        /* ── Protocol ───────────────────────────────────────────────────────── */
        uint32_t nProtocolVersion = 0;


        /* ── Constructors ───────────────────────────────────────────────────── */

        /** Default Constructor **/
        CanonicalSession() = default;

        /** Construct from identity scalars. **/
        CanonicalSession(
            const uint256_t& hashKeyID_,
            const uint256_t& hashGenesis_,
            const std::string& strAddress_);


        /* ── Projections (compatibility) ────────────────────────────────────── */

        /** ToMiningContext
         *
         *  Project this canonical record into a MiningContext for callers that
         *  still use the old type.  Copies all overlapping fields.
         *
         **/
        MiningContext ToMiningContext() const;

        /** FromMiningContext
         *
         *  Construct a CanonicalSession from a MiningContext.
         *  Fields not present in MiningContext are left at defaults.
         *
         **/
        static CanonicalSession FromMiningContext(const MiningContext& ctx);

        /** GetSessionBinding **/
        SessionBinding GetSessionBinding() const;

        /** GetCryptoContext **/
        CryptoContext GetCryptoContext() const;


        /* ── Queries ────────────────────────────────────────────────────────── */

        /** AnyPortLive — true if either protocol lane is connected. **/
        bool AnyPortLive() const { return fStatelessLive || fLegacyLive; }


        /** IsExpired — true if no ports live and activity older than timeout. **/
        bool IsExpired(uint64_t nTimeoutSec, uint64_t nNow) const;

        /** IsConsideredInactive — mirrors MiningContext::IsConsideredInactive. **/
        bool IsConsideredInactive(uint64_t nNow, uint64_t nTimeoutSec) const;


        /* ── Helpers ────────────────────────────────────────────────────────── */

        /** DeriveIP — extract IP from strAddress (strip :port). **/
        static std::string DeriveIP(const std::string& strAddress_);
    };

} // namespace LLP

#endif
