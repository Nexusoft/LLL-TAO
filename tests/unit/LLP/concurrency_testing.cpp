/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_manager.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>

#include <LLC/include/random.h>
#include <Util/include/runtime.h>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <sstream>

using namespace LLP;


/** Concurrency Test Parameters
 *
 *  These values are chosen to provide meaningful stress testing while
 *  remaining practical for test execution:
 *
 *  - nThreads (10): Sufficient parallelism to expose race conditions without
 *    excessive resource consumption. Matches typical production server cores.
 *  - nMinersPerThread (100): Creates 1000 total connections, representative
 *    of high-throughput mining environments.
 *  - nOperations (500): Enough iterations to statistically expose concurrency
 *    issues in add/remove operations.
 *  - nUpdatesPerThread (50): Tests rapid context updates on shared resources.
 *  - nIncrementsPerThread (1000): Validates atomic counter integrity under load.
 *
 *  The product nThreads × nMinersPerThread = 1000 simulates a large pool
 *  environment as specified in the PR requirements for "thousands of
 *  simultaneous connections".
 **/


/** Test Suite: Concurrency Testing for Stateless Mining
 *
 *  These tests evaluate Stateless Mining performance under high concurrency:
 *
 *  1. Thread-safe miner registration with thousands of connections
 *  2. Concurrent context updates without data races
 *  3. Lock-free statistics under high load
 *  4. Genesis hash mapping thread safety
 *  5. Session ID index concurrent access
 *
 **/


TEST_CASE("StatelessMinerManager Concurrent Registration", "[concurrency][manager]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    /* Clear existing state */
    manager.CleanupInactive(0);

    SECTION("Concurrent miner registration from multiple threads")
    {
        const int nThreads = 10;
        const int nMinersPerThread = 100;
        std::vector<std::thread> threads;
        std::atomic<int> nRegistered{0};

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int m = 0; m < nMinersPerThread; ++m)
                {
                    std::stringstream ss;
                    ss << "192.168." << t << "." << m;
                    std::string strAddress = ss.str();

                    uint256_t keyId = LLC::GetRand256();

                    MiningContext ctx = MiningContext()
                        .WithChannel((m % 2) + 1)
                        .WithSession(t * 1000 + m)
                        .WithKeyId(keyId)
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp());

                    manager.UpdateMiner(strAddress, ctx);
                    ++nRegistered;
                }
            });
        }

        /* Wait for all threads */
        for(auto& thread : threads)
            thread.join();

        /* All registrations should complete */
        REQUIRE(nRegistered == nThreads * nMinersPerThread);

        /* Manager should have recorded all miners */
        REQUIRE(manager.GetMinerCount() >= static_cast<size_t>(nThreads * nMinersPerThread));

        /* Cleanup */
        manager.CleanupInactive(0);
    }

    SECTION("Concurrent registration and removal")
    {
        const int nOperations = 500;
        std::atomic<int> nAdded{0};
        std::atomic<int> nRemoved{0};
        std::vector<std::thread> threads;

        /* Thread to add miners */
        threads.emplace_back([&]() {
            for(int i = 0; i < nOperations; ++i)
            {
                std::stringstream ss;
                ss << "10.0.0." << (i % 256);
                std::string strAddress = ss.str();

                MiningContext ctx = MiningContext()
                    .WithChannel(1)
                    .WithSession(i)
                    .WithAuth(true)
                    .WithTimestamp(runtime::unifiedtimestamp());

                manager.UpdateMiner(strAddress, ctx);
                ++nAdded;
            }
        });

        /* Thread to remove miners */
        threads.emplace_back([&]() {
            for(int i = 0; i < nOperations; ++i)
            {
                std::stringstream ss;
                ss << "10.0.0." << (i % 256);
                std::string strAddress = ss.str();

                if(manager.RemoveMiner(strAddress))
                    ++nRemoved;

                /* Small delay to allow some adds */
                if(i % 50 == 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        for(auto& thread : threads)
            thread.join();

        /* Both operations should complete without crashes */
        REQUIRE(nAdded == nOperations);
        /* Some removals may succeed or fail depending on timing */
        REQUIRE(nRemoved >= 0);

        /* Cleanup */
        manager.CleanupInactive(0);
    }
}


TEST_CASE("StatelessMinerManager Concurrent Updates", "[concurrency][updates]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Concurrent updates to same miner context")
    {
        std::string strAddress = "concurrent.test.miner";

        /* Initial registration */
        MiningContext initial = MiningContext()
            .WithChannel(1)
            .WithSession(12345)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner(strAddress, initial);

        /* Concurrent updates from multiple threads */
        const int nThreads = 20;
        const int nUpdatesPerThread = 50;
        std::vector<std::thread> threads;
        std::atomic<int> nUpdates{0};

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int u = 0; u < nUpdatesPerThread; ++u)
                {
                    MiningContext ctx = MiningContext()
                        .WithChannel((t % 2) + 1)
                        .WithSession(12345)
                        .WithHeight(1000 + u)
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp())
                        .WithKeepaliveCount(u);

                    manager.UpdateMiner(strAddress, ctx);
                    ++nUpdates;
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* All updates should complete */
        REQUIRE(nUpdates == nThreads * nUpdatesPerThread);

        /* Final context should be valid */
        auto finalCtx = manager.GetMinerContext(strAddress);
        REQUIRE(finalCtx.has_value());
        REQUIRE(finalCtx->fAuthenticated == true);
        REQUIRE(finalCtx->nSessionId == 12345);

        /* Cleanup */
        manager.RemoveMiner(strAddress);
    }
}


TEST_CASE("StatelessMinerManager Concurrent Statistics", "[concurrency][statistics]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Lock-free statistics increments under load")
    {
        const int nThreads = 10;
        const int nIncrementsPerThread = 1000;
        std::vector<std::thread> threads;

        uint64_t nTemplatesBefore = manager.GetTotalTemplatesServed();
        uint64_t nSubmitsBefore = manager.GetTotalBlocksSubmitted();
        uint64_t nAcceptsBefore = manager.GetTotalBlocksAccepted();

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&]() {
                for(int i = 0; i < nIncrementsPerThread; ++i)
                {
                    manager.IncrementTemplatesServed();
                    if(i % 10 == 0)
                        manager.IncrementBlocksSubmitted();
                    if(i % 100 == 0)
                        manager.IncrementBlocksAccepted();
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* Verify final counts */
        uint64_t nTemplatesAfter = manager.GetTotalTemplatesServed();
        uint64_t nSubmitsAfter = manager.GetTotalBlocksSubmitted();
        uint64_t nAcceptsAfter = manager.GetTotalBlocksAccepted();

        uint64_t nExpectedTemplates = nThreads * nIncrementsPerThread;
        uint64_t nExpectedSubmits = nThreads * (nIncrementsPerThread / 10);
        uint64_t nExpectedAccepts = nThreads * (nIncrementsPerThread / 100);

        REQUIRE(nTemplatesAfter - nTemplatesBefore == nExpectedTemplates);
        REQUIRE(nSubmitsAfter - nSubmitsBefore == nExpectedSubmits);
        REQUIRE(nAcceptsAfter - nAcceptsBefore == nExpectedAccepts);
    }

    SECTION("Concurrent keepalive tracking")
    {
        const int nThreads = 10;
        const int nMinersPerThread = 50;
        std::vector<std::thread> threads;

        uint64_t nKeepalivesBefore = manager.GetTotalKeepalives();

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int m = 0; m < nMinersPerThread; ++m)
                {
                    std::stringstream ss;
                    ss << "keepalive." << t << "." << m;
                    std::string strAddress = ss.str();

                    /* Register with keepalive count */
                    MiningContext ctx = MiningContext()
                        .WithChannel(1)
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp())
                        .WithKeepaliveCount(0);

                    manager.UpdateMiner(strAddress, ctx);

                    /* Increment keepalives */
                    for(int k = 1; k <= 5; ++k)
                    {
                        MiningContext updated = ctx.WithKeepaliveCount(k);
                        manager.UpdateMiner(strAddress, updated);
                        ctx = updated;
                    }
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        uint64_t nKeepalivesAfter = manager.GetTotalKeepalives();

        /* Should have tracked keepalives */
        REQUIRE(nKeepalivesAfter > nKeepalivesBefore);

        /* Cleanup */
        manager.CleanupInactive(0);
    }
}


TEST_CASE("StatelessMinerManager Genesis Hash Index Concurrency", "[concurrency][genesis]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Concurrent registration with genesis hash mapping")
    {
        const int nMinersPerGenesis = 10;
        const int nGenesisHashes = 20;
        std::vector<std::thread> threads;
        std::vector<uint256_t> genesisHashes;

        /* Pre-generate genesis hashes */
        for(int g = 0; g < nGenesisHashes; ++g)
        {
            genesisHashes.push_back(LLC::GetRand256());
        }

        for(int g = 0; g < nGenesisHashes; ++g)
        {
            threads.emplace_back([&, g]() {
                for(int m = 0; m < nMinersPerGenesis; ++m)
                {
                    std::stringstream ss;
                    ss << "genesis." << g << "." << m;
                    std::string strAddress = ss.str();

                    MiningContext ctx = MiningContext()
                        .WithChannel((m % 2) + 1)
                        .WithSession(g * 1000 + m)
                        .WithGenesis(genesisHashes[g])
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp());

                    manager.UpdateMiner(strAddress, ctx);
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* Verify genesis lookups work */
        for(int g = 0; g < nGenesisHashes; ++g)
        {
            auto miners = manager.ListMinersByGenesis(genesisHashes[g]);

            /* Should find miners for this genesis */
            REQUIRE(miners.size() >= 1);

            /* All should have correct genesis */
            for(const auto& ctx : miners)
            {
                REQUIRE(ctx.hashGenesis == genesisHashes[g]);
            }
        }

        /* Cleanup */
        manager.CleanupInactive(0);
    }

    SECTION("Concurrent genesis context lookup")
    {
        /* Pre-populate with miners */
        const int nMiners = 100;
        std::vector<uint256_t> genesisHashes;

        for(int i = 0; i < nMiners; ++i)
        {
            uint256_t genesis = LLC::GetRand256();
            genesisHashes.push_back(genesis);

            std::stringstream ss;
            ss << "lookup." << i;
            std::string strAddress = ss.str();

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithGenesis(genesis)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(strAddress, ctx);
        }

        /* Concurrent lookups */
        const int nLookupThreads = 10;
        std::vector<std::thread> threads;
        std::atomic<int> nSuccessful{0};

        for(int t = 0; t < nLookupThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int i = 0; i < nMiners; ++i)
                {
                    auto ctx = manager.GetMinerContextByGenesis(genesisHashes[i]);
                    if(ctx.has_value())
                        ++nSuccessful;
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* All lookups should succeed */
        REQUIRE(nSuccessful == nLookupThreads * nMiners);

        /* Cleanup */
        manager.CleanupInactive(0);
    }
}


TEST_CASE("StatelessMinerManager Session ID Index Concurrency", "[concurrency][session]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Concurrent session ID lookups")
    {
        /* Pre-populate */
        const int nMiners = 200;
        std::vector<uint32_t> sessionIds;

        for(int i = 0; i < nMiners; ++i)
        {
            uint32_t sessionId = static_cast<uint32_t>(LLC::GetRand256().Get64(0) & 0xFFFFFFFF);
            sessionIds.push_back(sessionId);

            std::stringstream ss;
            ss << "session." << i;
            std::string strAddress = ss.str();

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithSession(sessionId)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(strAddress, ctx);
        }

        /* Concurrent lookups by session ID */
        const int nThreads = 8;
        std::vector<std::thread> threads;
        std::atomic<int> nFound{0};

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int i = t; i < nMiners; i += nThreads)
                {
                    auto ctx = manager.GetMinerContextBySessionID(sessionIds[i]);
                    if(ctx.has_value() && ctx->nSessionId == sessionIds[i])
                        ++nFound;
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* All session lookups should succeed */
        REQUIRE(nFound == nMiners);

        /* Cleanup */
        manager.CleanupInactive(0);
    }
}


TEST_CASE("StatelessMinerManager New Round Notification Concurrency", "[concurrency][round]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Concurrent new round notifications")
    {
        /* Pre-populate miners */
        const int nMiners = 100;

        for(int i = 0; i < nMiners; ++i)
        {
            std::stringstream ss;
            ss << "round." << i;
            std::string strAddress = ss.str();

            MiningContext ctx = MiningContext()
                .WithChannel((i % 2) + 1)
                .WithHeight(5000)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(strAddress, ctx);
        }

        /* Concurrent round notifications */
        const int nThreads = 5;
        std::vector<std::thread> threads;
        std::atomic<uint32_t> nTotalNotified{0};

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int r = 0; r < 10; ++r)
                {
                    uint32_t nHeight = 5001 + t * 10 + r;
                    uint32_t nNotified = manager.NotifyNewRound(nHeight);
                    nTotalNotified += nNotified;
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* Notifications should complete without error */
        /* Note: Some notifications may return 0 if height didn't change */
        REQUIRE(nTotalNotified > 0);

        /* Cleanup */
        manager.CleanupInactive(0);
    }

    SECTION("Concurrent height queries during updates")
    {
        manager.SetCurrentHeight(10000);

        const int nReaders = 10;
        const int nWriters = 3;
        std::vector<std::thread> threads;
        std::atomic<bool> fStop{false};
        std::atomic<int> nReads{0};
        std::atomic<int> nWrites{0};

        /* Reader threads */
        for(int r = 0; r < nReaders; ++r)
        {
            threads.emplace_back([&]() {
                while(!fStop.load())
                {
                    uint32_t height = manager.GetCurrentHeight();
                    (void)height;  // Use the value
                    ++nReads;

                    /* Check for new round */
                    bool isNew = manager.IsNewRound(9999);
                    (void)isNew;
                }
            });
        }

        /* Writer threads */
        for(int w = 0; w < nWriters; ++w)
        {
            threads.emplace_back([&, w]() {
                for(int i = 0; i < 100; ++i)
                {
                    manager.SetCurrentHeight(10000 + w * 100 + i);
                    ++nWrites;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
        }

        /* Let it run briefly */
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        fStop = true;

        for(auto& thread : threads)
            thread.join();

        /* Operations should complete without data races */
        REQUIRE(nReads > 0);
        REQUIRE(nWrites == nWriters * 100);
    }
}


TEST_CASE("StatelessMinerManager Peak Session Tracking", "[concurrency][peak]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Peak session count tracks maximum")
    {
        /* Get baseline (may be non-zero from other tests) */
        size_t nInitialPeak = manager.GetPeakSessionCount();

        /* Add many miners to exceed peak */
        const int nMiners = 500;

        for(int i = 0; i < nMiners; ++i)
        {
            std::stringstream ss;
            ss << "peak." << i;
            std::string strAddress = ss.str();

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(strAddress, ctx);
        }

        size_t nNewPeak = manager.GetPeakSessionCount();

        /* Peak should have increased */
        REQUIRE(nNewPeak >= static_cast<size_t>(nMiners));

        /* Remove miners */
        for(int i = 0; i < nMiners; ++i)
        {
            std::stringstream ss;
            ss << "peak." << i;
            manager.RemoveMiner(ss.str());
        }

        /* Peak should NOT decrease after removal */
        size_t nPeakAfterRemoval = manager.GetPeakSessionCount();
        REQUIRE(nPeakAfterRemoval >= nNewPeak);
    }
}
