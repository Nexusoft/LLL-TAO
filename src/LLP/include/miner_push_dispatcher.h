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
     *  USAGE (state.cpp)
     *  =================
     *     LLP::MinerPushDispatcher::DispatchPushEvent(nHeight, hash);
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
         *  @param[in] nHeight      Unified blockchain height at the time of the new tip.
         *  @param[in] hashBestChain Current best-chain hash (uint1024_t).
         *
         **/
        static void DispatchPushEvent(uint32_t nHeight, const uint1024_t& hashBestChain);


    private:

        /** Packed dedup key for one channel: high-32 = unified height, low-32 = hash prefix. */
        static std::atomic<uint64_t> s_nPrimeDedup;   /* dedup key for Prime channel */
        static std::atomic<uint64_t> s_nHashDedup;    /* dedup key for Hash  channel */

        /** BroadcastChannel
         *
         *  Internal helper: broadcast one channel on both lanes and log results.
         *
         *  @param[in] nChannel     Mining channel (1=Prime, 2=Hash).
         *  @param[in] nHeight      Unified height (for logging).
         *  @param[in] hashPrefix4  First 4 bytes of hashBestChain (for logging).
         *
         **/
        static void BroadcastChannel(uint32_t nChannel, uint32_t nHeight, uint32_t hashPrefix4);
    };

} // namespace LLP

#endif
