/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/session_recovery.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/falcon_auth.h>

#include <Util/include/runtime.h>

#include <thread>
#include <vector>
#include <atomic>

using namespace LLP;


/** Test Suite: Session Error Handling and Fault-Tolerant Reconnections
 *
 *  These tests cover scenarios where Stateless miners disconnect mid-session
 *  and require fault-tolerant reconnections:
 *
 *  1. Mid-session disconnection recovery
 *  2. Reconnection with valid session state
 *  3. Reconnection limits and expiry handling
 *  4. Address-based session recovery
 *  5. Concurrent session recovery under load
 *
 **/


TEST_CASE("Session Recovery - Mid-Session Disconnect", "[session_recovery][fault_tolerance]")
{
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();

    /* Clear existing sessions */
    manager.CleanupExpired(0);

    /* Set reasonable timeout for tests */
    manager.SetSessionTimeout(3600);  // 1 hour
    manager.SetMaxReconnects(10);

    SECTION("Session saved on disconnect preserves authentication state")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1111222233334444555566667777888899990000aaaabbbbccccddddeeeeffff");

        uint256_t testGenesis;
        testGenesis.SetHex("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");

        /* Create authenticated session context */
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithHeight(50000)
            .WithSession(111222)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Simulate disconnect - save session */
        bool fSaved = manager.SaveSession(ctx);
        REQUIRE(fSaved == true);

        /* Verify session exists */
        REQUIRE(manager.HasSession(testKeyId) == true);

        /* Simulate reconnect - recover session */
        MiningContext recoveredCtx;
        bool fRecovered = manager.RecoverSession(testKeyId, recoveredCtx);

        REQUIRE(fRecovered == true);
        REQUIRE(recoveredCtx.nChannel == 2);
        REQUIRE(recoveredCtx.nSessionId == 111222);
        REQUIRE(recoveredCtx.hashKeyID == testKeyId);
        REQUIRE(recoveredCtx.hashGenesis == testGenesis);
        REQUIRE(recoveredCtx.fAuthenticated == true);

        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }

    SECTION("Multiple reconnects within limit succeed")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("aabbccdd11223344aabbccdd11223344aabbccdd11223344aabbccdd11223344");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(333444)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.SaveSession(ctx);

        /* Simulate multiple reconnects */
        const int nMaxReconnects = static_cast<int>(manager.GetMaxReconnects());

        for(int i = 0; i < nMaxReconnects - 1; ++i)
        {
            MiningContext recovered;
            bool fRecovered = manager.RecoverSession(testKeyId, recovered);
            REQUIRE(fRecovered == true);
        }

        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }

    SECTION("Reconnects exceeding limit are rejected")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("5555666677778888999900001111222233334444555566667777888899990000");

        /* Set low reconnect limit for testing */
        manager.SetMaxReconnects(3);

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(444555)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.SaveSession(ctx);

        /* Use up all reconnects */
        for(int i = 0; i < 3; ++i)
        {
            MiningContext recovered;
            manager.RecoverSession(testKeyId, recovered);
        }

        /* Next reconnect should fail */
        MiningContext finalRecovery;
        bool fRecovered = manager.RecoverSession(testKeyId, finalRecovery);
        REQUIRE(fRecovered == false);

        /* Reset to default */
        manager.SetMaxReconnects(10);
    }

    SECTION("Expired session cannot be recovered")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210");

        /* Set very short timeout for testing */
        manager.SetSessionTimeout(1);  // 1 second

        /* Create old session */
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithSession(555666)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp() - 10);  // 10 seconds ago

        manager.SaveSession(ctx);

        /* Wait a moment to ensure expiry */
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        /* Recovery should fail due to expiry */
        MiningContext recovered;
        bool fRecovered = manager.RecoverSession(testKeyId, recovered);
        REQUIRE(fRecovered == false);

        /* Reset timeout */
        manager.SetSessionTimeout(3600);
    }
}


TEST_CASE("Session Recovery - Address-Based Lookup", "[session_recovery][address]")
{
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();

    /* Clear and configure */
    manager.CleanupExpired(0);
    manager.SetSessionTimeout(3600);

    SECTION("Session recovered by network address")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("0123012301230123012301230123012301230123012301230123012301230123");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(666777)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Set address manually (simulating connection) */
        ctx.strAddress = "192.168.50.100";

        manager.SaveSession(ctx);

        /* Recover by address (miner reconnects from same IP) */
        MiningContext recovered;
        bool fRecovered = manager.RecoverSessionByAddress("192.168.50.100", recovered);

        REQUIRE(fRecovered == true);
        REQUIRE(recovered.nSessionId == 666777);
        REQUIRE(recovered.hashKeyID == testKeyId);

        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }

    SECTION("Address lookup fails for unknown address")
    {
        MiningContext recovered;
        bool fRecovered = manager.RecoverSessionByAddress("10.10.10.99", recovered);

        REQUIRE(fRecovered == false);
    }
}


TEST_CASE("Session Recovery - State Update Scenarios", "[session_recovery][state]")
{
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();

    manager.CleanupExpired(0);
    manager.SetSessionTimeout(3600);

    SECTION("Session update preserves key fields")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("aaaa1111bbbb2222cccc3333dddd4444eeee5555ffff6666777788889999aaaa");

        /* Initial session */
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(777888)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.SaveSession(ctx);

        /* Update with new channel */
        MiningContext updated = ctx.WithChannel(2);
        manager.UpdateSession(updated);

        /* Recover and verify update */
        MiningContext recovered;
        bool fRecovered = manager.RecoverSession(testKeyId, recovered);

        REQUIRE(fRecovered == true);
        REQUIRE(recovered.nChannel == 2);  // Updated
        REQUIRE(recovered.nSessionId == 777888);  // Preserved
        REQUIRE(recovered.fAuthenticated == true);  // Preserved

        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }

    SECTION("Update of non-existent session fails gracefully")
    {
        uint256_t unknownKeyId;
        unknownKeyId.SetHex("9999888877776666555544443333222211110000ffffeeeeddddccccbbbbaaaa");

        MiningContext ctx = MiningContext()
            .WithKeyId(unknownKeyId);

        bool fUpdated = manager.UpdateSession(ctx);

        REQUIRE(fUpdated == false);
    }
}


TEST_CASE("Session Recovery - Concurrent Access", "[session_recovery][concurrent]")
{
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();

    manager.CleanupExpired(0);
    manager.SetSessionTimeout(3600);
    manager.SetMaxReconnects(1000);  // High limit for stress test

    SECTION("Concurrent session saves are thread-safe")
    {
        const int nThreads = 10;
        const int nSessionsPerThread = 20;
        std::vector<std::thread> threads;
        std::atomic<int> nSuccessful{0};

        for(int t = 0; t < nThreads; ++t)
        {
            threads.emplace_back([&, t]() {
                for(int s = 0; s < nSessionsPerThread; ++s)
                {
                    /* Create unique key for each session */
                    uint256_t keyId = LLC::GetRand256();

                    MiningContext ctx = MiningContext()
                        .WithChannel((s % 2) + 1)
                        .WithSession(t * 1000 + s)
                        .WithKeyId(keyId)
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp());

                    if(manager.SaveSession(ctx))
                        ++nSuccessful;
                }
            });
        }

        /* Wait for all threads */
        for(auto& thread : threads)
            thread.join();

        /* All saves should succeed */
        REQUIRE(nSuccessful == nThreads * nSessionsPerThread);

        /* Cleanup - remove all sessions */
        manager.CleanupExpired(0);
    }

    SECTION("Concurrent recovery and save operations")
    {
        /* Pre-populate some sessions */
        std::vector<uint256_t> keyIds;
        for(int i = 0; i < 50; ++i)
        {
            uint256_t keyId = LLC::GetRand256();
            keyIds.push_back(keyId);

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithSession(i)
                .WithKeyId(keyId)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.SaveSession(ctx);
        }

        /* Concurrent reads and writes */
        std::atomic<int> nRecoveries{0};
        std::atomic<int> nSaves{0};
        std::vector<std::thread> threads;

        /* Recovery threads */
        for(int t = 0; t < 5; ++t)
        {
            threads.emplace_back([&]() {
                for(int i = 0; i < 10; ++i)
                {
                    size_t idx = static_cast<size_t>(i % keyIds.size());
                    MiningContext ctx;
                    if(manager.RecoverSession(keyIds[idx], ctx))
                        ++nRecoveries;
                }
            });
        }

        /* Save threads */
        for(int t = 0; t < 5; ++t)
        {
            threads.emplace_back([&]() {
                for(int i = 0; i < 10; ++i)
                {
                    uint256_t newKey = LLC::GetRand256();
                    MiningContext ctx = MiningContext()
                        .WithChannel(2)
                        .WithSession(1000 + i)
                        .WithKeyId(newKey)
                        .WithAuth(true)
                        .WithTimestamp(runtime::unifiedtimestamp());

                    if(manager.SaveSession(ctx))
                        ++nSaves;
                }
            });
        }

        for(auto& thread : threads)
            thread.join();

        /* Operations should complete without crashes */
        REQUIRE(nRecoveries > 0);
        REQUIRE(nSaves == 50);

        /* Cleanup */
        manager.CleanupExpired(0);
    }
}


TEST_CASE("Session Recovery - Cleanup Operations", "[session_recovery][cleanup]")
{
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();

    manager.CleanupExpired(0);

    SECTION("CleanupExpired removes old sessions")
    {
        /* Set short timeout */
        manager.SetSessionTimeout(1);

        /* Create sessions with old timestamps */
        for(int i = 0; i < 10; ++i)
        {
            uint256_t keyId = LLC::GetRand256();

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithSession(i)
                .WithKeyId(keyId)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp() - 100);  // 100 seconds old

            manager.SaveSession(ctx);
        }

        /* Verify sessions exist */
        REQUIRE(manager.GetSessionCount() >= 10);

        /* Cleanup with 1 second timeout */
        uint32_t nRemoved = manager.CleanupExpired(1);

        /* All old sessions should be removed */
        REQUIRE(nRemoved >= 10);

        /* Reset timeout */
        manager.SetSessionTimeout(3600);
    }

    SECTION("GetSessionCount reflects actual count")
    {
        /* Clear all */
        manager.CleanupExpired(0);

        size_t nInitial = manager.GetSessionCount();
        REQUIRE(nInitial == 0);

        /* Add sessions */
        for(int i = 0; i < 5; ++i)
        {
            uint256_t keyId = LLC::GetRand256();

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithSession(i)
                .WithKeyId(keyId)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.SaveSession(ctx);
        }

        REQUIRE(manager.GetSessionCount() == 5);

        /* Cleanup */
        manager.CleanupExpired(0);
        REQUIRE(manager.GetSessionCount() == 0);
    }
}
