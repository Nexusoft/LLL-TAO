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

#include <Util/include/runtime.h>

using namespace LLP;


/** Test Suite: Mining Template Integration
 *
 *  These tests verify the mining template integration functionality
 *  as specified in the requirements:
 *  
 *  1. Mining Template Integration - Dynamic template generation and secure distribution
 *  2. Legacy Mining Logic Connection - Wire miner.cpp to Stateless Miner architecture
 *  3. Stateless Manager Optimization - Unified flow for legacy and stateless miners
 *
 **/


TEST_CASE("StatelessMinerManager Template Feeding Tests", "[mining_template][integration]")
{
    /* Get the singleton manager */
    StatelessMinerManager& manager = StatelessMinerManager::Get();
    
    /* Clear any existing miners for clean test state */
    manager.CleanupInactive(0);
    
    SECTION("NotifyNewRound updates tracked height")
    {
        /* Set initial height */
        manager.SetCurrentHeight(1000);
        REQUIRE(manager.GetCurrentHeight() == 1000);
        
        /* Notify of new round */
        uint32_t nNotified = manager.NotifyNewRound(1001);
        
        /* Height should be updated */
        REQUIRE(manager.GetCurrentHeight() == 1001);
    }
    
    SECTION("NotifyNewRound returns 0 for same height")
    {
        /* Set height */
        manager.SetCurrentHeight(2000);
        
        /* Notify with same height */
        uint32_t nNotified = manager.NotifyNewRound(2000);
        
        /* Should return 0 (no change) */
        REQUIRE(nNotified == 0);
    }
    
    SECTION("NotifyNewRound updates all miner contexts")
    {
        /* Create test contexts using parameterized constructor */
        MiningContext ctx1(1, 1000, runtime::unifiedtimestamp(), "192.168.1.1", 1, true, 11111, uint256_t(0), uint256_t(0));
        MiningContext ctx2(2, 1000, runtime::unifiedtimestamp(), "192.168.1.2", 1, true, 22222, uint256_t(0), uint256_t(0));
        
        /* Register miners */
        manager.UpdateMiner("192.168.1.1", ctx1);
        manager.UpdateMiner("192.168.1.2", ctx2);
        
        /* Notify new round */
        uint32_t nNotified = manager.NotifyNewRound(1001);
        
        /* Both miners should be notified */
        REQUIRE(nNotified >= 2);
        
        /* Verify heights were updated */
        auto updatedCtx1 = manager.GetMinerContext("192.168.1.1");
        auto updatedCtx2 = manager.GetMinerContext("192.168.1.2");
        
        REQUIRE(updatedCtx1.has_value());
        REQUIRE(updatedCtx2.has_value());
        REQUIRE(updatedCtx1->nHeight == 1001);
        REQUIRE(updatedCtx2->nHeight == 1001);
        
        /* Cleanup */
        manager.RemoveMiner("192.168.1.1");
        manager.RemoveMiner("192.168.1.2");
    }
    
    SECTION("IsNewRound detects height change")
    {
        /* Set height */
        manager.SetCurrentHeight(3000);
        
        /* Same height - not a new round */
        REQUIRE_FALSE(manager.IsNewRound(3000));
        
        /* Different height - is a new round */
        REQUIRE(manager.IsNewRound(2999));
        REQUIRE(manager.IsNewRound(3001));
    }
    
    SECTION("GetMinersForChannel filters by channel")
    {
        /* Create test contexts for different channels */
        MiningContext ctxPrime = MiningContext(1, 5000, runtime::unifiedtimestamp(), "10.0.0.1", 1, true, 33333, uint256_t(0), uint256_t(0));
        MiningContext ctxHash = MiningContext(2, 5000, runtime::unifiedtimestamp(), "10.0.0.2", 1, true, 44444, uint256_t(0), uint256_t(0));
        
        manager.UpdateMiner("10.0.0.1", ctxPrime);
        manager.UpdateMiner("10.0.0.2", ctxHash);
        
        /* Get prime miners */
        auto primeMiners = manager.GetMinersForChannel(1);
        
        /* Should contain at least the prime miner */
        bool foundPrime = false;
        for(const auto& ctx : primeMiners)
        {
            if(ctx.strAddress == "10.0.0.1")
                foundPrime = true;
        }
        REQUIRE(foundPrime);
        
        /* Get hash miners */
        auto hashMiners = manager.GetMinersForChannel(2);
        
        /* Should contain at least the hash miner */
        bool foundHash = false;
        for(const auto& ctx : hashMiners)
        {
            if(ctx.strAddress == "10.0.0.2")
                foundHash = true;
        }
        REQUIRE(foundHash);
        
        /* Cleanup */
        manager.RemoveMiner("10.0.0.1");
        manager.RemoveMiner("10.0.0.2");
    }
}


TEST_CASE("StatelessMinerManager Template Statistics Tests", "[mining_template][statistics]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();
    
    SECTION("Templates served counter increments correctly")
    {
        uint64_t nBefore = manager.GetTotalTemplatesServed();
        
        manager.IncrementTemplatesServed();
        manager.IncrementTemplatesServed();
        manager.IncrementTemplatesServed();
        
        uint64_t nAfter = manager.GetTotalTemplatesServed();
        
        REQUIRE(nAfter == nBefore + 3);
    }
    
    SECTION("Blocks submitted counter increments correctly")
    {
        uint64_t nBefore = manager.GetTotalBlocksSubmitted();
        
        manager.IncrementBlocksSubmitted();
        manager.IncrementBlocksSubmitted();
        
        uint64_t nAfter = manager.GetTotalBlocksSubmitted();
        
        REQUIRE(nAfter == nBefore + 2);
    }
    
    SECTION("Blocks accepted counter increments correctly")
    {
        uint64_t nBefore = manager.GetTotalBlocksAccepted();
        
        manager.IncrementBlocksAccepted();
        
        uint64_t nAfter = manager.GetTotalBlocksAccepted();
        
        REQUIRE(nAfter == nBefore + 1);
    }
}


TEST_CASE("Mining Context Height Tracking Tests", "[mining_template][context]")
{
    SECTION("Context preserves height through updates")
    {
        MiningContext ctx = MiningContext()
            .WithHeight(12345)
            .WithChannel(1)
            .WithAuth(true);
        
        REQUIRE(ctx.nHeight == 12345);
        
        /* Update other fields - height should persist */
        MiningContext updated = ctx.WithTimestamp(runtime::unifiedtimestamp());
        
        REQUIRE(updated.nHeight == 12345);
        REQUIRE(updated.nChannel == 1);
        REQUIRE(updated.fAuthenticated == true);
    }
    
    SECTION("WithHeight creates new context with updated height")
    {
        MiningContext ctx = MiningContext()
            .WithHeight(100)
            .WithChannel(2);
        
        MiningContext newCtx = ctx.WithHeight(101);
        
        /* Original unchanged */
        REQUIRE(ctx.nHeight == 100);
        
        /* New context has updated height */
        REQUIRE(newCtx.nHeight == 101);
        
        /* Channel preserved */
        REQUIRE(newCtx.nChannel == 2);
    }
}


TEST_CASE("Legacy and Stateless Miner Unified Flow Tests", "[mining_template][unified]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();
    
    /* Clear existing state */
    manager.CleanupInactive(0);
    
    SECTION("Manager tracks both legacy-style and stateless miners")
    {
        /* Create a context simulating legacy miner (stateful session) */
        MiningContext legacyCtx = MiningContext(
            1,  // nChannel - Prime
            1000,
            runtime::unifiedtimestamp(),
            "127.0.0.1",  // Localhost - legacy stateful
            1,  // nProtocolVersion
            true,  // fAuthenticated
            55555,  // nSessionId
            uint256_t(0),
            uint256_t(0)
        );
        
        /* Create a context simulating stateless miner (Falcon auth) */
        uint256_t testKeyId;
        testKeyId.SetHex("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
        
        MiningContext statelessCtx = MiningContext(
            2,  // nChannel - Hash
            1000,
            runtime::unifiedtimestamp(),
            "192.168.1.100",  // Remote - stateless
            2,  // nProtocolVersion
            true,  // fAuthenticated
            66666,  // nSessionId
            testKeyId,
            uint256_t(0)
        );
        
        /* Register both */
        manager.UpdateMiner("127.0.0.1", legacyCtx);
        manager.UpdateMiner("192.168.1.100", statelessCtx);
        
        /* Verify both are tracked */
        REQUIRE(manager.GetMinerCount() >= 2);
        
        auto retrievedLegacy = manager.GetMinerContext("127.0.0.1");
        auto retrievedStateless = manager.GetMinerContext("192.168.1.100");
        
        REQUIRE(retrievedLegacy.has_value());
        REQUIRE(retrievedStateless.has_value());
        
        REQUIRE(retrievedLegacy->nChannel == 1);
        REQUIRE(retrievedStateless->nChannel == 2);
        
        /* Notify new round - both should be updated */
        uint32_t nNotified = manager.NotifyNewRound(1001);
        REQUIRE(nNotified >= 2);
        
        /* Verify height update for both */
        auto updatedLegacy = manager.GetMinerContext("127.0.0.1");
        auto updatedStateless = manager.GetMinerContext("192.168.1.100");
        
        REQUIRE(updatedLegacy->nHeight == 1001);
        REQUIRE(updatedStateless->nHeight == 1001);
        
        /* Cleanup */
        manager.RemoveMiner("127.0.0.1");
        manager.RemoveMiner("192.168.1.100");
    }
    
    SECTION("Authenticated count reflects both miner types")
    {
        size_t nBefore = manager.GetAuthenticatedCount();
        
        /* Add authenticated miner */
        MiningContext authCtx = MiningContext(
            1, 2000, runtime::unifiedtimestamp(), "10.10.10.1",
            1, true, 77777, uint256_t(0), uint256_t(0)
        );
        manager.UpdateMiner("10.10.10.1", authCtx);
        
        /* Count should increase */
        REQUIRE(manager.GetAuthenticatedCount() >= nBefore + 1);
        
        /* Cleanup */
        manager.RemoveMiner("10.10.10.1");
    }
}
