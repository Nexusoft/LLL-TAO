/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_session_health.h>
#include <LLP/include/template_prewarmer.h>

#include <Util/include/args.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    /* Helper: build a non-zero uint256 with a recognisable Tritium Genesis
     * type byte (0xA1) so the registry's IsValidGenesisType filter accepts it. */
    uint256_t MakeReward(uint8_t nDistinguisher)
    {
        uint256_t r = 0;
        /* uint256 stores little-endian; byte 0 is the type byte for
         * GetType().  TritiumGenesis == 0xA1. */
        std::vector<uint8_t> bytes(32, 0);
        bytes[0] = 0xA1;
        bytes[1] = nDistinguisher;
        bytes[31] = 0xCC;
        r.SetBytes(bytes);
        return r;
    }

    uint1024_t MakeTip(uint8_t nDistinguisher)
    {
        uint1024_t r = 0;
        std::vector<uint8_t> bytes(128, 0);
        bytes[0] = nDistinguisher;
        bytes[127] = 0xDD;
        r.SetBytes(bytes);
        return r;
    }
}


TEST_CASE("RecentRewardRegistry deduplicates and respects TTL",
          "[template_prewarmer][registry]")
{
    auto& reg = LLP::RecentRewardRegistry::Instance();
    reg.ClearForTesting();

    const uint256_t rA = MakeReward(1);
    const uint256_t rB = MakeReward(2);

    /* Channel 0 (PoS) is rejected. */
    reg.Register(0, rA);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).empty());

    /* Zero reward is rejected. */
    reg.Register(1, uint256_t(0));
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).empty());

    /* Two distinct (channel, reward) tuples land as two entries; a repeat
     * of the same tuple does not duplicate. */
    reg.Register(1, rA);
    reg.Register(2, rA);
    reg.Register(1, rA);

    auto vEntries = reg.SnapshotFresh(std::chrono::seconds(60));
    REQUIRE(vEntries.size() == 2);

    /* TTL = 0 nanoseconds discards every entry. */
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(0)).empty());

    reg.Register(1, rB);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).size() == 3);

    /* Prune with TTL=0 must clear everything. */
    reg.Prune(std::chrono::seconds(0));
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).empty());

    reg.ClearForTesting();
}


TEST_CASE("MiningTemplatePrewarmer warms one request per registered tuple per tip",
          "[template_prewarmer][warm]")
{
    /* Ensure deterministic behaviour even if a previous test mutated args. */
    config::mapArgs.erase("-prewarm");
    config::mapArgs.erase("-prewarm.reward_ttl");
    config::mapArgs.erase("-prewarm.queue_max");

    auto& reg     = LLP::RecentRewardRegistry::Instance();
    auto& warmer  = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();

    /* Record build callbacks. */
    struct Record
    {
        uint32_t   nChannel;
        uint256_t  hashReward;
        uint1024_t hashTip;
    };
    std::mutex                m_records_mutex;
    std::condition_variable   m_records_cv;
    std::vector<Record>       records;
    std::atomic<std::size_t>  nBuilds{0};

    warmer.SetBuildFnForTesting([&](uint32_t nChannel,
                                    const uint256_t& hashReward,
                                    const uint1024_t& hashTip)
    {
        std::lock_guard<std::mutex> lock(m_records_mutex);
        records.push_back({nChannel, hashReward, hashTip});
        nBuilds.fetch_add(1, std::memory_order_release);
        m_records_cv.notify_all();
    });

    warmer.Start();
    REQUIRE(warmer.IsRunning());

    const uint256_t rA   = MakeReward(11);
    const uint256_t rB   = MakeReward(12);
    const uint1024_t tip = MakeTip(0x10);

    reg.Register(1, rA);
    reg.Register(2, rA);
    reg.Register(1, rB);

    const auto statsBefore = warmer.GetStats();
    warmer.NotifyTipAdvance(/*nUnifiedHeight=*/100, tip);

    /* Wait for the worker to drain the three requests we just enqueued. */
    {
        std::unique_lock<std::mutex> lock(m_records_mutex);
        const bool ok = m_records_cv.wait_for(lock, std::chrono::seconds(2),
            [&] { return nBuilds.load(std::memory_order_acquire) >= 3; });
        REQUIRE(ok);
    }

    /* Validate we warmed exactly the three registered tuples — duplicates
     * are coalesced and PoS (channel 0) entries never reach the registry. */
    std::map<std::pair<uint32_t, uint256_t>, std::size_t> counts;
    {
        std::lock_guard<std::mutex> lock(m_records_mutex);
        for(const auto& r : records)
        {
            REQUIRE(r.hashTip == tip);
            ++counts[{r.nChannel, r.hashReward}];
        }
    }
    REQUIRE(counts.size() == 3);
    REQUIRE(counts[{1, rA}] == 1);
    REQUIRE(counts[{2, rA}] == 1);
    REQUIRE(counts[{1, rB}] == 1);

    const auto statsAfter = warmer.GetStats();
    REQUIRE(statsAfter.nEnqueued >= statsBefore.nEnqueued + 3);
    REQUIRE(statsAfter.nWarmed   >= statsBefore.nWarmed   + 3);

    warmer.Stop();
    REQUIRE_FALSE(warmer.IsRunning());

    /* Cleanup for sibling tests. */
    warmer.ResetBuildFnForTesting();
    reg.ClearForTesting();
}


TEST_CASE("MiningTemplatePrewarmer deduplicates same-tip requests in the queue",
          "[template_prewarmer][coalesce]")
{
    config::mapArgs.erase("-prewarm");
    config::mapArgs.erase("-prewarm.reward_ttl");
    config::mapArgs.erase("-prewarm.queue_max");
    /* Pin to a single worker so the queue-coalescing semantics are
     * deterministic.  Pool-sizing is exercised by the dedicated
     * [template_prewarmer][pool] test below. */
    config::mapArgs["-prewarm.workers"] = "1";

    auto& reg    = LLP::RecentRewardRegistry::Instance();
    auto& warmer = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();

    /* Throttle the worker so multiple NotifyTipAdvance() calls land in the
     * queue before the first one is drained.  We do that by holding the
     * builder until the test releases it. */
    std::mutex              m_gate_mutex;
    std::condition_variable m_gate_cv;
    std::atomic<bool>       fReleased{false};
    std::atomic<std::size_t> nBuilds{0};

    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        std::unique_lock<std::mutex> lock(m_gate_mutex);
        m_gate_cv.wait(lock, [&] { return fReleased.load(std::memory_order_acquire); });
        nBuilds.fetch_add(1, std::memory_order_release);
    });

    warmer.Start();

    const uint256_t rA   = MakeReward(21);
    const uint1024_t tip = MakeTip(0x21);
    reg.Register(1, rA);

    const auto statsBefore = warmer.GetStats();

    /* Fire three notifications for the same tip+reward; the second and
     * third must coalesce against the first (still pending) request. */
    warmer.NotifyTipAdvance(200, tip);
    warmer.NotifyTipAdvance(200, tip);
    warmer.NotifyTipAdvance(200, tip);

    /* Release the gate so the worker drains. */
    {
        std::lock_guard<std::mutex> lock(m_gate_mutex);
        fReleased.store(true, std::memory_order_release);
    }
    m_gate_cv.notify_all();

    /* Give the worker a moment to settle. */
    for(int i = 0; i < 100 && nBuilds.load(std::memory_order_acquire) < 1; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const auto statsAfter = warmer.GetStats();
    REQUIRE(statsAfter.nEnqueued == statsBefore.nEnqueued + 1);
    REQUIRE(nBuilds.load(std::memory_order_acquire) == 1);

    warmer.Stop();
    warmer.ResetBuildFnForTesting();
    reg.ClearForTesting();
    config::mapArgs.erase("-prewarm.workers");
}


TEST_CASE("MiningTemplatePrewarmer worker pool warms in parallel",
          "[template_prewarmer][pool]")
{
    config::mapArgs.erase("-prewarm");
    config::mapArgs.erase("-prewarm.reward_ttl");
    config::mapArgs.erase("-prewarm.queue_max");
    /* Force a 4-worker pool so the test is deterministic regardless of the
     * host machine's hardware concurrency. */
    config::mapArgs["-prewarm.workers"] = "4";

    auto& reg    = LLP::RecentRewardRegistry::Instance();
    auto& warmer = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();

    /* Block every worker in the builder until we observe peak concurrency,
     * then release them all.  This proves that more than one builder ran
     * at the same wall-clock instant — the whole point of the thread pool. */
    std::mutex                m_gate_mutex;
    std::condition_variable   m_observed_cv;
    std::condition_variable   m_release_cv;
    std::atomic<std::size_t>  nConcurrent{0};
    std::atomic<std::size_t>  nPeakConcurrent{0};
    std::atomic<bool>         fRelease{false};
    std::atomic<std::size_t>  nBuilds{0};

    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        const std::size_t now = nConcurrent.fetch_add(1, std::memory_order_acq_rel) + 1;
        std::size_t prev = nPeakConcurrent.load(std::memory_order_relaxed);
        /* Standard CAS retry: keep trying to publish `now` as the new
         * peak until either we win the race or another thread has already
         * pushed the peak past `now`. */
        while(now > prev
           && !nPeakConcurrent.compare_exchange_weak(prev, now,
                                                      std::memory_order_acq_rel))
            ;
        {
            std::lock_guard<std::mutex> lock(m_gate_mutex);
            m_observed_cv.notify_all();
        }
        {
            std::unique_lock<std::mutex> lock(m_gate_mutex);
            m_release_cv.wait(lock, [&] { return fRelease.load(std::memory_order_acquire); });
        }
        nConcurrent.fetch_sub(1, std::memory_order_acq_rel);
        nBuilds.fetch_add(1, std::memory_order_release);
    });

    warmer.Start();

    /* Stats expose the pool size so the operator can verify it from RPC. */
    REQUIRE(warmer.GetStats().nWorkers == 4);

    /* Register four distinct rewards so all four workers can pick up work
     * in parallel. */
    for(uint8_t i = 0; i < 4; ++i)
        reg.Register(1, MakeReward(51 + i));

    warmer.NotifyTipAdvance(500, MakeTip(0x50));

    /* Wait until peak concurrency reaches 4 (the whole pool is busy in
     * the builder simultaneously). */
    {
        std::unique_lock<std::mutex> lock(m_gate_mutex);
        const bool ok = m_observed_cv.wait_for(lock, std::chrono::seconds(2),
            [&] { return nPeakConcurrent.load(std::memory_order_acquire) >= 4; });
        REQUIRE(ok);
    }
    REQUIRE(nPeakConcurrent.load(std::memory_order_acquire) == 4);

    /* Release the gate and wait for all builds to complete. */
    {
        std::lock_guard<std::mutex> lock(m_gate_mutex);
        fRelease.store(true, std::memory_order_release);
    }
    m_release_cv.notify_all();

    for(int i = 0; i < 200 && nBuilds.load(std::memory_order_acquire) < 4; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(nBuilds.load(std::memory_order_acquire) == 4);

    warmer.Stop();
    warmer.ResetBuildFnForTesting();
    reg.ClearForTesting();
    config::mapArgs.erase("-prewarm.workers");
}


TEST_CASE("MiningTemplatePrewarmer drops oldest when queue exceeds cap",
          "[template_prewarmer][drop_oldest]")
{
    /* Tiny cap forces immediate overflow. */
    config::mapArgs["-prewarm.queue_max"] = "2";
    /* Pin to a single worker so requests pile up in the queue
     * deterministically before the worker drains them. */
    config::mapArgs["-prewarm.workers"] = "1";

    auto& reg    = LLP::RecentRewardRegistry::Instance();
    auto& warmer = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();

    std::mutex              m_gate_mutex;
    std::condition_variable m_gate_cv;
    std::atomic<bool>       fReleased{false};
    std::atomic<std::size_t> nBuilds{0};

    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        std::unique_lock<std::mutex> lock(m_gate_mutex);
        m_gate_cv.wait(lock, [&] { return fReleased.load(std::memory_order_acquire); });
        nBuilds.fetch_add(1, std::memory_order_release);
    });

    warmer.Start();

    /* Five distinct rewards on one channel. */
    for(uint8_t i = 0; i < 5; ++i)
        reg.Register(1, MakeReward(31 + i));

    const auto statsBefore = warmer.GetStats();
    warmer.NotifyTipAdvance(300, MakeTip(0x31));

    /* Release the gate. */
    {
        std::lock_guard<std::mutex> lock(m_gate_mutex);
        fReleased.store(true, std::memory_order_release);
    }
    m_gate_cv.notify_all();

    /* Allow the worker to drain whatever survived. */
    for(int i = 0; i < 200 && nBuilds.load(std::memory_order_acquire) < 2; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const auto statsAfter = warmer.GetStats();
    /* With cap=2 and 5 candidates, exactly 3 must have been dropped before
     * the worker started servicing.  We tolerate the case where the worker
     * picks one off before the next NotifyTipAdvance entry slides in (which
     * keeps the queue tip-coalesced), so the lower bound is what matters. */
    REQUIRE(statsAfter.nDropped >= statsBefore.nDropped + 3);
    REQUIRE(nBuilds.load(std::memory_order_acquire) >= 2);

    warmer.Stop();
    warmer.ResetBuildFnForTesting();
    reg.ClearForTesting();
    config::mapArgs.erase("-prewarm.queue_max");
    config::mapArgs.erase("-prewarm.workers");
}


TEST_CASE("MiningTemplatePrewarmer is a no-op when -prewarm=false",
          "[template_prewarmer][disabled]")
{
    config::mapArgs["-prewarm"] = "0";

    auto& reg    = LLP::RecentRewardRegistry::Instance();
    auto& warmer = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();

    std::atomic<std::size_t> nBuilds{0};
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        nBuilds.fetch_add(1, std::memory_order_release);
    });

    warmer.Start();
    reg.Register(1, MakeReward(41));
    warmer.NotifyTipAdvance(400, MakeTip(0x41));

    /* Give the worker time it would have used to warm. */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(nBuilds.load(std::memory_order_acquire) == 0);

    warmer.Stop();
    warmer.ResetBuildFnForTesting();
    reg.ClearForTesting();
    config::mapArgs.erase("-prewarm");
}


TEST_CASE("MiningTemplatePrewarmer skips notify/build when default session is unavailable",
          "[template_prewarmer][session]")
{
    config::mapArgs.erase("-prewarm");
    config::mapArgs.erase("-prewarm.reward_ttl");
    config::mapArgs.erase("-prewarm.queue_max");
    config::mapArgs.erase("-prewarm.workers");

    REQUIRE_FALSE(LLP::IsDefaultSessionReady());

    auto& reg    = LLP::RecentRewardRegistry::Instance();
    auto& warmer = LLP::MiningTemplatePrewarmer::Instance();
    reg.ClearForTesting();
    warmer.ResetBuildFnForTesting();

    warmer.Start();
    const auto statsBefore = warmer.GetStats();

    reg.Register(1, MakeReward(42));
    warmer.NotifyTipAdvance(420, MakeTip(0x42));

    /* Allow worker wakeups to occur if anything was enqueued unexpectedly. */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto statsAfter = warmer.GetStats();
    REQUIRE(statsAfter.nEnqueued == statsBefore.nEnqueued);
    REQUIRE(statsAfter.nWarmed == statsBefore.nWarmed);

    warmer.Stop();
    reg.ClearForTesting();
}
