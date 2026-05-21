/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_TEMPLATE_PREWARMER_H
#define NEXUS_LLP_INCLUDE_TEMPLATE_PREWARMER_H

#include <LLC/types/uint1024.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace LLP
{

    /** RecentRewardRegistry
     *
     *  Tracks recently observed (channel, hashRewardAddress) pairs across both
     *  the legacy lane (port 8323) and the stateless lane (port 9323) so the
     *  MiningTemplatePrewarmer can decide which (channel, reward) tuples to
     *  warm whenever the chain tip advances.
     *
     *  Entries are touched on every reward-bound new_block() resolution and
     *  pruned by SnapshotFresh() based on a configurable TTL
     *  (-prewarm.reward_ttl, default 300 s).  The registry is intentionally
     *  best-effort and resilient to lost updates: a missed register call only
     *  means the prewarmer will not warm one reward for one tip, never an
     *  incorrect block.
     *
     *  Thread-safety: every public method is internally synchronised.
     *
     **/
    class RecentRewardRegistry
    {
    public:
        struct Entry
        {
            uint32_t nChannel{0};
            uint256_t hashRewardAddress;
            std::chrono::steady_clock::time_point tLastSeen;
        };

        /** Process-wide singleton accessor. */
        static RecentRewardRegistry& Instance();

        /** Record that a miner has resolved this (channel, reward) tuple.
         *
         *  Safe to call from any thread.  Channel 0 and zero reward addresses
         *  are ignored (channel 0 is PoS, which mines through the stake
         *  minter, not these lanes).
         *
         *  @param[in] nChannel             Mining channel (1=Prime, 2=Hash).
         *  @param[in] hashRewardAddress    Reward genesis hash.
         **/
        void Register(uint32_t nChannel, const uint256_t& hashRewardAddress);

        /** Return the entries seen within the last TTL window. */
        std::vector<Entry> SnapshotFresh(std::chrono::seconds nTTL) const;

        /** Remove entries older than nTTL. */
        void Prune(std::chrono::seconds nTTL);

        /** Test-only: remove every recorded entry. */
        void ClearForTesting();

    private:
        RecentRewardRegistry() = default;

        mutable std::mutex m_mutex;
        std::map<std::pair<uint32_t, uint256_t>, std::chrono::steady_clock::time_point> m_entries;
    };


    /** MiningTemplatePrewarmer
     *
     *  Single-threaded background warmer that, on every chain tip advance,
     *  invokes the wallet-signed block template builder for each recently
     *  seen (channel, reward) tuple.  The builder writes into the per-channel
     *  mining template cache.  When the lane's per-connection async PUSH
     *  worker subsequently calls into the same builder, the cache is already
     *  populated and the producer signing cost is skipped.
     *
     *  Bounded queue with drop-oldest on overflow keeps memory and work
     *  amplification under control during reorg storms.
     *
     *  Operator flags:
     *    -prewarm                = true              (master switch)
     *    -prewarm.reward_ttl     = 300               (seconds)
     *    -prewarm.queue_max      = 64                (requests)
     *    -prewarm.workers        = auto              (worker thread count)
     *
     *  Worker pool sizing:
     *    `-prewarm.workers` defaults to `max(2, min(8, hardware_concurrency/2))`
     *    so that on a node with 100–200 miners the prewarmer can sign
     *    several producers in parallel and still keep up with the Prime
     *    channel's ~50 s tip cadence.  Each worker pops from the same
     *    bounded deque, so adding workers is a pure throughput knob with
     *    no coalescing impact.
     *
     **/
    class MiningTemplatePrewarmer
    {
    public:
        struct Stats
        {
            uint64_t nEnqueued{0};
            uint64_t nWarmed{0};
            uint64_t nDropped{0};
            uint64_t nStaleTipSkipped{0};
            uint64_t nWorkers{0};
        };

        static MiningTemplatePrewarmer& Instance();

        /** Start the background worker thread pool.  Idempotent. */
        void Start();

        /** Stop all workers, drain pending requests, and join all threads. */
        void Stop();

        /** Whether the worker pool is currently running.  Lock-free. */
        bool IsRunning() const { return m_running.load(std::memory_order_acquire); }

        /** Called from BlockState::SetBest() after EnqueuePushEvent.
         *
         *  For every fresh registered (channel, reward) tuple, enqueues a
         *  warm-up request targeting the new tip.  Returns immediately;
         *  builder cost is paid on the worker thread.
         **/
        void NotifyTipAdvance(uint32_t nUnifiedHeight, const uint1024_t& hashNewTip);

        /** Telemetry snapshot.  Lock-free. */
        Stats GetStats() const;

        /** Hook for unit tests: replace the real CreateBlockForStatelessMining
         *  call with a callback so tests can observe warm requests without a
         *  live blockchain. */
        using BuildFn = std::function<void(uint32_t /*nChannel*/,
                                            const uint256_t& /*hashRewardAddress*/,
                                            const uint1024_t& /*hashExpectedTip*/)>;
        void SetBuildFnForTesting(BuildFn fn);
        void ResetBuildFnForTesting();

    private:
        MiningTemplatePrewarmer() = default;
        MiningTemplatePrewarmer(const MiningTemplatePrewarmer&) = delete;
        MiningTemplatePrewarmer& operator=(const MiningTemplatePrewarmer&) = delete;

        struct WarmupRequest
        {
            uint32_t nChannel{0};
            uint256_t hashRewardAddress;
            uint1024_t hashExpectedTip;
            std::chrono::steady_clock::time_point tEnqueuedAt;
        };

        void WorkerLoop();
        bool ShouldDeduplicate(const WarmupRequest& candidate) const;

        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::vector<std::thread> m_threads;
        std::atomic<bool> m_running{false};
        std::deque<WarmupRequest> m_queue;

        BuildFn m_buildFn;

        /* Telemetry counters (lock-free reads). */
        std::atomic<uint64_t> m_enqueued_total{0};
        std::atomic<uint64_t> m_warmed_total{0};
        std::atomic<uint64_t> m_dropped_total{0};
        std::atomic<uint64_t> m_stale_tip_skipped_total{0};
    };

} // namespace LLP

#endif
