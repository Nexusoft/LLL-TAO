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
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

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
     *  4. ASYNC DISPATCH: SetBest() enqueues the push event and returns immediately
     *     via EnqueuePushEvent().  A dedicated worker thread dequeues and calls
     *     DispatchPushEvent() so that a flood of Tritium peer GET requests can never
     *     delay the delivery of PRIME_BLOCK_AVAILABLE / HASH_BLOCK_AVAILABLE
     *     notifications to miners.
     *
     *  NOTE: Template refresh during dry spells is handled on the miner side.
     *  The miner detects stale templates and autonomously sends GET_BLOCK to
     *  request fresh work.
     *
     *  USAGE (state.cpp)
     *  =================
     *     LLP::MinerPushDispatcher::EnqueuePushEvent(nHeight, hash);
     *
     *  LIFECYCLE (global.cpp)
     *  ======================
     *     LLP::MinerPushDispatcher::StartPushWorker();   // at init
     *     LLP::MinerPushDispatcher::StopPushWorker();    // at shutdown
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
         *  This is now called from the dedicated push-worker thread rather than
         *  directly from SetBest().  Callers inside SetBest() should use
         *  EnqueuePushEvent() instead.
         *
         *  @param[in] nHeight      Unified blockchain height at the time of the new tip.
         *  @param[in] hashBestChain Current best-chain hash (uint1024_t).
         *
         **/
        static void DispatchPushEvent(uint32_t nHeight, const uint1024_t& hashBestChain);


        /** EnqueuePushEvent
         *
         *  Non-blocking entry-point called from SetBest() on the Tritium P2P data
         *  thread.  Enqueues the (nHeight, hashBestChain) pair into the async push
         *  queue and signals the push-worker thread to process it, then returns
         *  immediately so the Tritium data thread is never blocked by miner
         *  notification delivery.
         *
         *  If the push-worker has not been started (e.g. in unit-test contexts),
         *  falls back to calling DispatchPushEvent() synchronously.
         *
         *  @param[in] nHeight      Unified blockchain height at the time of the new tip.
         *  @param[in] hashBestChain Current best-chain hash (uint1024_t).
         *
         **/
        static void EnqueuePushEvent(uint32_t nHeight, const uint1024_t& hashBestChain);


        /** StartPushWorker
         *
         *  Starts the dedicated push-notification worker thread.
         *  Must be called once during node initialisation before any blocks are
         *  accepted (i.e. before the Tritium server is opened for connections).
         *
         **/
        static void StartPushWorker();


        /** StopPushWorker
         *
         *  Signals the push-worker thread to drain its queue and exit, then joins
         *  the thread.  Must be called during node shutdown after all peer servers
         *  have been stopped so no new events are enqueued after this returns.
         *
         **/
        static void StopPushWorker();


    private:

        /** Packed dedup key for one channel: high-32 = unified height, low-32 = hash prefix. */
        static std::atomic<uint64_t> s_nPrimeDedup;   /* dedup key for Prime channel */
        static std::atomic<uint64_t> s_nHashDedup;    /* dedup key for Hash  channel */

        /** Async push queue protected by s_pushMutex + s_pushCV.
         *  Each entry is a (unified_height, hashBestChain) pair. **/
        static std::queue<std::pair<uint32_t, uint1024_t>> s_pushQueue;
        static std::mutex                                   s_pushMutex;
        static std::condition_variable                      s_pushCV;

        /** The dedicated push-worker thread. **/
        static std::thread s_pushThread;

        /** Set to true while the push-worker thread is running. **/
        static std::atomic<bool> s_pushRunning;


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


        /** PushWorkerThread
         *
         *  Worker-thread body.  Drains s_pushQueue and calls DispatchPushEvent()
         *  for each event until s_pushRunning is false and the queue is empty.
         *
         **/
        static void PushWorkerThread();
    };

} // namespace LLP

#endif
