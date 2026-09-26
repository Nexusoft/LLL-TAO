/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/template_prewarmer.h>

#include <LLP/include/genesis_constants.h>
#include <LLP/include/mining_session_health.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

namespace LLP
{

    // ─────────────────────────────────────────────────────────────────────────
    // RecentRewardRegistry
    // ─────────────────────────────────────────────────────────────────────────

    RecentRewardRegistry& RecentRewardRegistry::Instance()
    {
        static RecentRewardRegistry s_instance;
        return s_instance;
    }


    std::size_t RecentRewardRegistry::ShardIndex(const uint256_t& hashRewardAddress)
    {
        return static_cast<std::size_t>(hashRewardAddress.Get64(0))
             & (kShards - 1);
    }


    namespace
    {
        /* Centralised bidirectional mapping between mining channel numbers
         * (1=Prime, 2=Hash) and the per-shard `maps` array index (0, 1).
         * Channels are validated by the caller (Register early-returns for
         * anything other than 1 or 2), so these helpers can assume the
         * input is in range. */
        constexpr std::size_t ChannelToMapIndex(uint32_t nChannel)
        {
            /* nChannel ∈ {1, 2} → {0, 1}. */
            return static_cast<std::size_t>(nChannel - 1u);
        }

        constexpr uint32_t MapIndexToChannel(std::size_t iMap)
        {
            /* iMap ∈ {0, 1} → {1, 2}. */
            return static_cast<uint32_t>(iMap + 1u);
        }
    }


    void RecentRewardRegistry::Register(uint32_t nChannel, const uint256_t& hashRewardAddress)
    {
        /* Channel 0 is PoS; it does not route through the mining lanes. */
        if(nChannel != 1 && nChannel != 2)
            return;

        if(hashRewardAddress == 0)
            return;

        /* Defence-in-depth: never register a reward whose type byte fails
         * the network consensus check; that would warm a template the
         * network would reject anyway. */
        if(!LLP::GenesisConstants::IsValidGenesisType(hashRewardAddress))
            return;

        const auto   tNow      = std::chrono::steady_clock::now();
        Shard&       shard     = m_shards[ShardIndex(hashRewardAddress)];
        const std::size_t iMap = ChannelToMapIndex(nChannel);

        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.maps[iMap][hashRewardAddress] = tNow;
    }


    std::vector<RecentRewardRegistry::Entry>
    RecentRewardRegistry::SnapshotFresh(std::chrono::seconds nTTL) const
    {
        const auto tNow = std::chrono::steady_clock::now();
        std::vector<Entry> out;

        for(std::size_t s = 0; s < kShards; ++s)
        {
            Shard& shard = m_shards[s];
            std::lock_guard<std::mutex> lock(shard.mutex);
            for(std::size_t iMap = 0; iMap < shard.maps.size(); ++iMap)
            {
                const uint32_t nChannel = MapIndexToChannel(iMap);
                for(const auto& kv : shard.maps[iMap])
                {
                    if(tNow - kv.second > nTTL)
                        continue;

                    Entry e;
                    e.nChannel          = nChannel;
                    e.hashRewardAddress = kv.first;
                    e.tLastSeen         = kv.second;
                    out.push_back(e);
                }
            }
        }
        return out;
    }


    void RecentRewardRegistry::Prune(std::chrono::seconds nTTL)
    {
        const auto tNow = std::chrono::steady_clock::now();
        for(std::size_t s = 0; s < kShards; ++s)
        {
            Shard& shard = m_shards[s];
            std::lock_guard<std::mutex> lock(shard.mutex);
            for(auto& map : shard.maps)
            {
                for(auto it = map.begin(); it != map.end(); )
                {
                    if(tNow - it->second > nTTL)
                        it = map.erase(it);
                    else
                        ++it;
                }
            }
        }
    }


    void RecentRewardRegistry::ClearForTesting()
    {
        for(std::size_t s = 0; s < kShards; ++s)
        {
            Shard& shard = m_shards[s];
            std::lock_guard<std::mutex> lock(shard.mutex);
            for(auto& map : shard.maps)
                map.clear();
        }
    }


    // ─────────────────────────────────────────────────────────────────────────
    // MiningTemplatePrewarmer
    // ─────────────────────────────────────────────────────────────────────────

    namespace
    {
        constexpr const char* kSessionNotFoundMsg = "Session not found";

        /* Operator-tunable parameters with conservative defaults. */
        std::chrono::seconds RewardTTL()
        {
            return std::chrono::seconds(config::GetArg("-prewarm.reward_ttl", 300));
        }

        std::size_t QueueMax()
        {
            const int64_t n = config::GetArg("-prewarm.queue_max", 64);
            if(n <= 0)
                return 64;
            return static_cast<std::size_t>(n);
        }

        bool PrewarmEnabled()
        {
            return config::GetBoolArg("-prewarm", true);
        }

        /* Worker pool size.  Default scales with hardware so a 16-core node
         * gets 8 prewarm workers and a 4-core node gets 2.  Bounded at 8 to
         * avoid contending with consensus/networking threads. */
        std::size_t WorkerCount()
        {
            const int64_t nExplicit = config::GetArg("-prewarm.workers", 0);
            if(nExplicit > 0)
                return static_cast<std::size_t>(std::min<int64_t>(nExplicit, 32));

            const unsigned int hw = std::thread::hardware_concurrency();
            std::size_t n = (hw > 0) ? static_cast<std::size_t>(hw / 2) : 2;
            if(n < 2) n = 2;
            if(n > 8) n = 8;
            return n;
        }
    }


    MiningTemplatePrewarmer& MiningTemplatePrewarmer::Instance()
    {
        static MiningTemplatePrewarmer s_instance;
        return s_instance;
    }


    void MiningTemplatePrewarmer::Start()
    {
        std::size_t nWorkers = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(m_running.load(std::memory_order_acquire))
                return;

            /* Join any leftover threads from a previous Stop() before
             * spawning the new pool, to keep std::vector<std::thread>
             * lifetime semantics clean. */
            for(auto& t : m_threads)
                if(t.joinable())
                    t.join();
            m_threads.clear();

            m_queue.clear();
            m_running.store(true, std::memory_order_release);

            nWorkers = WorkerCount();
            m_threads.reserve(nWorkers);
            for(std::size_t i = 0; i < nWorkers; ++i)
                m_threads.emplace_back(&MiningTemplatePrewarmer::WorkerLoop, this);
        }

        debug::log(0, FUNCTION, "[PREWARM] worker pool started (enabled=",
                   PrewarmEnabled() ? "true" : "false",
                   " workers=", nWorkers,
                   " ttl_s=", RewardTTL().count(),
                   " queue_max=", QueueMax(), ")");
    }


    void MiningTemplatePrewarmer::Stop()
    {
        std::vector<std::thread> vJoin;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_running.load(std::memory_order_acquire))
                return;
            m_running.store(false, std::memory_order_release);
            m_queue.clear();
            vJoin = std::move(m_threads);
            m_threads.clear();
        }
        m_cv.notify_all();

        for(auto& t : vJoin)
            if(t.joinable())
                t.join();

        debug::log(0, FUNCTION, "[PREWARM] worker pool stopped"
                   " enqueued=", m_enqueued_total.load(std::memory_order_relaxed),
                   " warmed=",   m_warmed_total.load(std::memory_order_relaxed),
                   " dropped=",  m_dropped_total.load(std::memory_order_relaxed),
                   " stale_skip=", m_stale_tip_skipped_total.load(std::memory_order_relaxed));
    }


    bool MiningTemplatePrewarmer::ShouldDeduplicate(const WarmupRequest& candidate) const
    {
        for(const auto& q : m_queue)
        {
            if(q.nChannel == candidate.nChannel
            && q.hashRewardAddress == candidate.hashRewardAddress
            && q.hashExpectedTip == candidate.hashExpectedTip)
                return true;
        }
        return false;
    }


    void MiningTemplatePrewarmer::NotifyTipAdvance(uint32_t nUnifiedHeight,
                                                    const uint1024_t& hashNewTip)
    {
        if(!PrewarmEnabled())
            return;

        if(!m_running.load(std::memory_order_acquire))
            return;

        if(hashNewTip == 0)
            return;

        bool fHasBuildHook = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            fHasBuildHook = static_cast<bool>(m_buildFn);
        }

        /* Unit tests inject m_buildFn and are intentionally decoupled from
         * wallet-session state so queue/pool behavior can be validated without
         * API session setup. */
        if(!fHasBuildHook && !LLP::IsDefaultSessionReady())
        {
            debug::log(3, FUNCTION, "[PREWARM] skip tip_advance: SESSION::DEFAULT not ready"
                       " height=", nUnifiedHeight,
                       " tip=", hashNewTip.SubString());
            return;
        }

        /* Snapshot the registry first to keep the queue lock short. */
        const auto vRewards = RecentRewardRegistry::Instance().SnapshotFresh(RewardTTL());
        if(vRewards.empty())
            return;

        const auto tNow      = std::chrono::steady_clock::now();
        const std::size_t cap = QueueMax();

        std::size_t nEnqueued = 0;
        std::size_t nDropped  = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_running.load(std::memory_order_acquire))
                return;

            for(const auto& r : vRewards)
            {
                WarmupRequest req;
                req.nChannel          = r.nChannel;
                req.hashRewardAddress = r.hashRewardAddress;
                req.hashExpectedTip   = hashNewTip;
                req.tEnqueuedAt       = tNow;

                if(ShouldDeduplicate(req))
                    continue;

                if(m_queue.size() >= cap)
                {
                    m_queue.pop_front();
                    ++nDropped;
                }

                m_queue.push_back(req);
                ++nEnqueued;
            }
        }
        if(nEnqueued > 0)
        {
            m_enqueued_total.fetch_add(nEnqueued, std::memory_order_relaxed);
            /* Wake every worker — N enqueues may keep N consumers busy in
             * parallel and notify_all is cheap relative to producer signing. */
            m_cv.notify_all();
        }
        if(nDropped > 0)
            m_dropped_total.fetch_add(nDropped, std::memory_order_relaxed);

        debug::log(2, FUNCTION, "[PREWARM] tip_advance height=", nUnifiedHeight,
                   " tip=", hashNewTip.SubString(),
                   " rewards=", vRewards.size(),
                   " enqueued=", nEnqueued,
                   " dropped=", nDropped);
    }


    void MiningTemplatePrewarmer::WorkerLoop()
    {
        while(true)
        {
            WarmupRequest req;
            BuildFn fnLocal;

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]
                {
                    return !m_queue.empty()
                        || !m_running.load(std::memory_order_acquire)
                        || config::fShutdown.load();
                });

                /* Unconditional break on shutdown / Stop():
                 *  - Stop() clears m_queue under the same mutex before
                 *    notify_all(), so under !m_running the queue is already
                 *    empty by construction.
                 *  - On global fShutdown, any queued tuples are stale and
                 *    must be discarded — draining them one-by-one with
                 *    `continue` would spin the worker in a tight loop
                 *    burning CPU until the queue empties, and would race
                 *    Stop()'s m_threads.clear().
                 * Matches the per-connection TemplateWorkerLoop pattern in
                 * miner.cpp / stateless_miner_connection.cpp which also
                 * breaks unconditionally on !running. */
                if(config::fShutdown.load()
                || !m_running.load(std::memory_order_acquire))
                    break;

                if(m_queue.empty())
                    continue;

                req = m_queue.front();
                m_queue.pop_front();
                fnLocal = m_buildFn;
            }

            if(config::fShutdown.load())
                continue;

            /* Tip-fence: skip stale requests whose tip is no longer current. */
            const uint1024_t hashChainTip = TAO::Ledger::ChainState::hashBestChain.load();
            if(req.hashExpectedTip != 0 && req.hashExpectedTip != hashChainTip)
            {
                m_stale_tip_skipped_total.fetch_add(1, std::memory_order_relaxed);
                debug::log(3, FUNCTION, "[PREWARM] stale-tip skip want=",
                           req.hashExpectedTip.SubString(),
                           " have=", hashChainTip.SubString(),
                           " channel=", req.nChannel,
                           " reward=", req.hashRewardAddress.SubString());
                continue;
            }

            try
            {
                if(fnLocal)
                {
                    /* Unit-test injection path. */
                    fnLocal(req.nChannel, req.hashRewardAddress, req.hashExpectedTip);
                }
                else
                {
                    if(!LLP::IsDefaultSessionReady())
                    {
                        debug::log(3, FUNCTION,
                                   "[PREWARM] skip warm: SESSION::DEFAULT not ready"
                                   " channel=", req.nChannel,
                                   " reward=", req.hashRewardAddress.SubString());
                        continue;
                    }

                    /* Production path: build a template; its side-effect is
                     * to populate tBlockCache[nChannel] for the wallet/reward
                     * tuple.  We discard the returned pointer because the
                     * cache is what the per-connection workers will pick up.
                     */
                    TAO::Ledger::TritiumBlock* pBlock =
                        TAO::Ledger::CreateBlockForStatelessMining(
                            req.nChannel,
                            /*nExtraNonce=*/0,
                            req.hashRewardAddress);
                    if(pBlock != nullptr)
                        delete pBlock;
                }

                m_warmed_total.fetch_add(1, std::memory_order_relaxed);
                debug::log(3, FUNCTION, "[PREWARM] warmed channel=", req.nChannel,
                           " tip=", req.hashExpectedTip.SubString(),
                           " reward=", req.hashRewardAddress.SubString());
            }
            catch(const std::exception& e)
            {
                /* Never propagate exceptions out of the worker thread —
                 * warming is best-effort.  Worst case: PUSH worker pays
                 * the producer signing cost itself, exactly as before. */
                const std::string strWhat = e.what();
                if(strWhat.find(kSessionNotFoundMsg) != std::string::npos)
                {
                    debug::log(3, FUNCTION, "[PREWARM] skip warm: ", strWhat,
                               " channel=", req.nChannel,
                               " reward=", req.hashRewardAddress.SubString());
                }
                else
                {
                    debug::error(FUNCTION, "[PREWARM] exception during warm: ", e.what(),
                                 " channel=", req.nChannel,
                                 " reward=", req.hashRewardAddress.SubString());
                }
            }
        }
    }


    MiningTemplatePrewarmer::Stats MiningTemplatePrewarmer::GetStats() const
    {
        Stats s;
        s.nEnqueued         = m_enqueued_total.load(std::memory_order_relaxed);
        s.nWarmed           = m_warmed_total.load(std::memory_order_relaxed);
        s.nDropped          = m_dropped_total.load(std::memory_order_relaxed);
        s.nStaleTipSkipped  = m_stale_tip_skipped_total.load(std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            s.nWorkers = m_threads.size();
        }
        return s;
    }


    void MiningTemplatePrewarmer::SetBuildFnForTesting(BuildFn fn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buildFn = std::move(fn);
    }


    void MiningTemplatePrewarmer::ResetBuildFnForTesting()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buildFn = nullptr;
    }

} // namespace LLP
