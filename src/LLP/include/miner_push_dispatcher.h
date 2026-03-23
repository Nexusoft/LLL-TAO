/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINER_PUSH_DISPATCHER_H
#define NEXUS_LLP_INCLUDE_MINER_PUSH_DISPATCHER_H

#include <atomic>
#include <cstdint>

#include <LLC/types/uint1024.h>

namespace LLP
{

    /** MinerPushDispatcher
     *
     *  Canonical single entry-point for all miner push-notification broadcasts.
     *
     *  DESIGN GOALS
     *  ============
     *  1. EXACTLY 4 lane-level sends per push event:
     *       Prime → Legacy lane
     *       Prime → Stateless lane
     *       Hash  → Legacy lane
     *       Hash  → Stateless lane
     *
     *  2. DEDUPLICATION: a (unified_height, hash-prefix-4B, channel) triplet is
     *     recorded atomically after the first dispatch.  Any re-entry with the
     *     same key (e.g. accidental double call from SetBest) is silently dropped.
     *
     *  3. LOGGING: at verbosity 0, emit one line per (channel, lane) plus a
     *     per-channel summary line so operators can verify both individual
     *     sends and overall counts.
     *
     *  4. HEARTBEAT REFRESH: when no push notification has been sent for
     *     FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS (480 s), the node
     *     proactively re-pushes the current template to prevent miners from
     *     entering degraded mode during dry spells on slow channels (e.g. Prime).
     *
     *  USAGE (state.cpp)
     *  =================
     *     LLP::MinerPushDispatcher::DispatchPushEvent(nHeight, hash);
     *
     *  HEARTBEAT USAGE (server.cpp Meter thread)
     *  =========================================
     *     LLP::MinerPushDispatcher::HeartbeatRefreshCheck();
     *
     **/
    class MinerPushDispatcher
    {
    public:

        /** DispatchPushEvent
         *
         *  Broadcast push notifications for BOTH Prime and Hash channels on BOTH
         *  Legacy and Stateless lanes.  Internally deduplicates by
         *  (unified_height, hash_prefix4, channel) so that accidental double-calls
         *  produce exactly one broadcast per (channel, lane) pair.
         *
         *  Also records the dispatch timestamp per channel (used by
         *  HeartbeatRefreshCheck to detect dry spells).
         *
         *  @param[in] nHeight      Unified blockchain height at the time of the new tip.
         *  @param[in] hashBestChain Current best-chain hash (uint1024_t).
         *
         **/
        static void DispatchPushEvent(uint32_t nHeight, const uint1024_t& hashBestChain);


        /** HeartbeatRefreshCheck
         *
         *  Proactively re-push templates to prevent miners from timing out during
         *  long dry spells (no new blocks mined for an extended period).
         *
         *  Called periodically from Server<ProtocolType>::Meter() for miner
         *  protocols at MiningConstants::HEARTBEAT_CHECK_INTERVAL_SECONDS cadence.
         *
         *  Behaviour per channel (Prime and Hash):
         *    - Logs operator warnings at 300 s, 450 s, and 550 s since last push.
         *    - At FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS (480 s) fires
         *      BroadcastChannel() to re-push the current template to all subscribed
         *      miners, bypassing the normal height/hash dedup guard.
         *    - Uses CAS on the per-channel dispatch timestamp to prevent duplicate
         *      heartbeats when both the Legacy and Stateless Meter threads run
         *      concurrently.
         *
         **/
        static void HeartbeatRefreshCheck();


    private:

        /** Packed dedup key for one channel: high-32 = unified height, low-32 = hash prefix. */
        static std::atomic<uint64_t> s_nPrimeDedup;   /* dedup key for Prime channel */
        static std::atomic<uint64_t> s_nHashDedup;    /* dedup key for Hash  channel */

        /** UNIX timestamps (seconds) of the last successful dispatch per channel.
         *
         *  Updated by DispatchPushEvent after each broadcast.  Read by
         *  HeartbeatRefreshCheck to detect dry spells and decide whether to
         *  fire a heartbeat re-push.  Zero means no dispatch has occurred yet
         *  since node start (treated as "never dispatched"; heartbeat skipped). */
        static std::atomic<uint64_t> s_nLastPrimeDispatchTime;
        static std::atomic<uint64_t> s_nLastHashDispatchTime;

        /** BroadcastChannel
         *
         *  Internal helper: broadcast one channel on both lanes and log results.
         *
         *  @param[in] nChannel     Mining channel (1=Prime, 2=Hash).
         *  @param[in] nHeight      Unified height (for logging).
         *  @param[in] hashPrefix4  First 4 bytes of hashBestChain (for logging).
         *  @param[in] fHeartbeat   When true, adds a [HEARTBEAT] log annotation and
         *                          resets per-connection rate-limit state via
         *                          PrepareHeartbeatNotification() before each send so
         *                          the push and the miner's subsequent GET_BLOCK are
         *                          not suppressed by the 2-second cooldown floor.
         *
         **/
        static void BroadcastChannel(uint32_t nChannel, uint32_t nHeight, uint32_t hashPrefix4,
                                     bool fHeartbeat = false);
    };

} // namespace LLP

#endif
