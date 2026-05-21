/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/template_prewarmer.h>

#include <LLP/include/genesis_constants.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>

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

        const auto tNow = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries[std::make_pair(nChannel, hashRewardAddress)] = tNow;
    }


    std::vector<RecentRewardRegistry::Entry>
    RecentRewardRegistry::SnapshotFresh(std::chrono::seconds nTTL) const
    {
        const auto tNow = std::chrono::steady_clock::now();
        std::vector<Entry> out;

        std::lock_guard<std::mutex> lock(m_mutex);
        out.reserve(m_entries.size());
        for(const auto& kv : m_entries)
        {
            if(tNow - kv.second > nTTL)
                continue;

            Entry e;
            e.nChannel           = kv.first.first;
            e.hashRewardAddress  = kv.first.second;
            e.tLastSeen          = kv.second;
            out.push_back(e);
        }
        return out;
    }


    void RecentRewardRegistry::Prune(std::chrono::seconds nTTL)
    {
        const auto tNow = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto it = m_entries.begin(); it != m_entries.end(); )
        {
            if(tNow - it->second > nTTL)
                it = m_entries.erase(it);
            else
                ++it;
        }
    }


    void RecentRewardRegistry::ClearForTesting()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
    }


    // ─────────────────────────────────────────────────────────────────────────
    // MiningTemplatePrewarmer
    // ─────────────────────────────────────────────────────────────────────────

    namespace
    {
        /* Operator-tunable parameters with conservative defaults. */
        std::chrono::seconds RewardTTL()
        {
            return std::chrono::seconds(config::GetArg("-prewarm.reward_ttl", 300));
        }

        std::size_t QueueMax()
        {
            const int64_t n = config::GetArg("-prewarm.queue_max", 16);
            if(n <= 0)
                return 16;
            return static_cast<std::size_t>(n);
        }

        bool PrewarmEnabled()
        {
            return config::GetBoolArg("-prewarm", true);
        }
    }


    MiningTemplatePrewarmer& MiningTemplatePrewarmer::Instance()
    {
        static MiningTemplatePrewarmer s_instance;
        return s_instance;
    }


    void MiningTemplatePrewarmer::Start()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if(m_running.load(std::memory_order_acquire))
            return;

        if(m_thread.joinable())
            m_thread.join();

        m_queue.clear();
        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&MiningTemplatePrewarmer::WorkerLoop, this);

        debug::log(0, FUNCTION, "[PREWARM] worker started (enabled=",
                   PrewarmEnabled() ? "true" : "false",
                   " ttl_s=", RewardTTL().count(),
                   " queue_max=", QueueMax(), ")");
    }


    void MiningTemplatePrewarmer::Stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_running.load(std::memory_order_acquire))
                return;
            m_running.store(false, std::memory_order_release);
            m_queue.clear();
        }
        m_cv.notify_all();

        if(m_thread.joinable())
            m_thread.join();

        debug::log(0, FUNCTION, "[PREWARM] worker stopped"
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
            m_cv.notify_one();
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

                if(config::fShutdown.load()
                || !m_running.load(std::memory_order_acquire))
                {
                    if(m_queue.empty())
                        break;
                }

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
                debug::error(FUNCTION, "[PREWARM] exception during warm: ", e.what(),
                             " channel=", req.nChannel,
                             " reward=", req.hashRewardAddress.SubString());
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
