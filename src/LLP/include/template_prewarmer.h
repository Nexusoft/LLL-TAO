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

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
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
     *  Thread-safety: shard-striped to remove the single-mutex hot-spot.
     *  `Register()` is called on every miner's `new_block()` (≥ once per
     *  GET_BLOCK at hundreds of miners), so the registry is divided into
     *  `kShards == 16` independent shards.  Each shard owns its own
     *  `std::mutex` and per-channel `std::unordered_map<uint256_t, TimePoint>`.
     *  Hashing by the low 64 bits of the reward distributes load evenly,
     *  so peak contention is `N_register_calls / 16` rather than `N`.  The
     *  per-shard `unordered_map` is also O(1) average vs the previous
     *  `std::map`'s O(log N), eliminating the red-black-tree walk on the
     *  hot path.
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

        /* 16 shards: power of two so the shard index is a single AND.
         * Sized for hundreds of miners on a multi-core node — well past
         * the point where a single-mutex registry would contend.  Each
         * shard is two channels (1 and 2) × one unordered_map. */
        static constexpr std::size_t kShards = 16;

        struct Shard
        {
            mutable std::mutex mutex;
            struct Uint256Hash
            {
                std::size_t operator()(const uint256_t& h) const noexcept
                {
                    /* Low 64 bits of the genesis hash are already
                     * cryptographically uniform — no further mixing needed. */
                    return static_cast<std::size_t>(h.Get64(0));
                }
            };
            /* Index 0 = channel 1 (Prime), index 1 = channel 2 (Hash). */
            std::array<std::unordered_map<uint256_t,
                                          std::chrono::steady_clock::time_point,
                                          Uint256Hash>, 2> maps;
        };

        /* Heap-allocated array so the Shard objects are not copyable/movable
         * dependencies of this class's CTAD/aggregate init. */
        mutable std::array<Shard, kShards> m_shards;

        /* Pick a shard from the low 64 bits of the reward hash, then mask
         * to kShards-1.  uint256_t::Get64(0) returns the lowest 64-bit
         * limb which is already well-distributed for genesis hashes. */
        static std::size_t ShardIndex(const uint256_t& hashRewardAddress);
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
