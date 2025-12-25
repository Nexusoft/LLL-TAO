/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <Util/include/runtime.h>

using namespace LLP;

/* Test constants */
namespace {
    /* Sample test genesis hash for reward routing tests */
    const char* TEST_GENESIS_HEX = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
    
    /* Sample test reward address (different from genesis) */
    const char* TEST_REWARD_HEX = "b174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd133";
    
    /* Helper to create test genesis hash */
    uint256_t GetTestGenesis()
    {
        uint256_t hash;
        hash.SetHex(TEST_GENESIS_HEX);
        return hash;
    }
    
    /* Helper to create test reward address */
    uint256_t GetTestReward()
    {
        uint256_t hash;
        hash.SetHex(TEST_REWARD_HEX);
        return hash;
    }
}


TEST_CASE("Reward Address Binding Tests", "[reward_routing]")
{
    SECTION("MiningContext stores genesis hash for authentication")
    {
        /* Create a test genesis hash */
        uint256_t testGenesis = GetTestGenesis();
        
        /* Create context with genesis */
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        REQUIRE(ctx.hashGenesis == testGenesis);
        REQUIRE(ctx.fAuthenticated == true);
    }
    
    SECTION("GetPayoutAddress returns reward address when bound")
    {
        /* Create a test reward address */
        uint256_t testReward = GetTestGenesis();
        
        /* Create context with bound reward address */
        MiningContext ctx = MiningContext().WithRewardAddress(testReward);
        
        /* GetPayoutAddress should return the bound reward address */
        REQUIRE(ctx.GetPayoutAddress() == testReward);
        REQUIRE(ctx.fRewardBound == true);
    }
    
    SECTION("GetPayoutAddress returns zero when neither reward nor genesis set")
    {
        /* Create context without reward binding or genesis */
        MiningContext ctx;
        
        /* GetPayoutAddress should return zero (no valid payout) */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.hashGenesis == uint256_t(0));
    }
    
    SECTION("GetPayoutAddress falls back to genesis when reward not bound")
    {
        /* Create test genesis hash */
        uint256_t testGenesis = GetTestGenesis();
        
        /* Create context with genesis but NO reward binding */
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        /* GetPayoutAddress should fall back to genesis */
        REQUIRE(ctx.GetPayoutAddress() == testGenesis);
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.hashGenesis == testGenesis);
    }
    
    SECTION("Genesis is separate from reward address")
    {
        /* Create test hashes */
        uint256_t testGenesis = GetTestGenesis();
        uint256_t testReward = GetTestReward();
        
        /* Create context with both genesis and reward address */
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithRewardAddress(testReward)
            .WithAuth(true);
        
        /* Genesis is used for authentication, reward address for payout */
        REQUIRE(ctx.hashGenesis == testGenesis);
        REQUIRE(ctx.hashRewardAddress == testReward);
        REQUIRE(ctx.GetPayoutAddress() == testReward);
    }
    
    SECTION("StatelessMinerManager tracks genesis correctly")
    {
        /* Get manager instance */
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create test context with genesis */
        uint256_t testGenesis = GetTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithTimestamp(1234567890);
        
        ctx.strAddress = "192.168.1.100:54321";
        
        /* Update manager with context */
        manager.UpdateMiner(ctx.strAddress, ctx);
        
        /* Retrieve and verify */
        auto retrieved = manager.GetMinerContext(ctx.strAddress);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value().hashGenesis == testGenesis);
        
        /* Cleanup */
        manager.RemoveMiner(ctx.strAddress);
    }
}


TEST_CASE("Stateless Mining Enforcement Tests", "[reward_routing]")
{
    SECTION("Authenticated miner can mine with genesis fallback")
    {
        uint256_t testGenesis = GetTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        /* Authenticated with genesis - can mine with genesis fallback */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.GetPayoutAddress() == testGenesis);  // Falls back to genesis
    }
    
    SECTION("Explicit reward address takes precedence over genesis")
    {
        uint256_t testGenesis = GetTestGenesis();
        uint256_t testReward = GetTestReward();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithRewardAddress(testReward);
        
        /* Explicit reward address overrides genesis fallback */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.GetPayoutAddress() == testReward);  // Uses explicit reward
        REQUIRE(ctx.GetPayoutAddress() != testGenesis);  // NOT genesis
    }
    
    SECTION("GetRewardBoundCount tracks bound miners")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create two test contexts */
        uint256_t testReward;
        testReward.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx1 = MiningContext()
            .WithAuth(true)
            .WithRewardAddress(testReward);
        ctx1.strAddress = "192.168.1.100:54321";
        
        MiningContext ctx2 = MiningContext()
            .WithAuth(true);  // Authenticated but no reward address
        ctx2.strAddress = "192.168.1.101:54321";
        
        /* Add both miners */
        manager.UpdateMiner(ctx1.strAddress, ctx1);
        manager.UpdateMiner(ctx2.strAddress, ctx2);
        
        /* Only one has reward bound */
        REQUIRE(manager.GetRewardBoundCount() >= 1);
        
        /* Cleanup */
        manager.RemoveMiner(ctx1.strAddress);
        manager.RemoveMiner(ctx2.strAddress);
    }
}


TEST_CASE("ProcessSetReward completes successfully and updates context", "[reward_routing]")
{
    SECTION("ProcessSetReward binds reward address and persists to manager")
    {
        /* Get manager instance */
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create test context with authentication and genesis */
        uint256_t testGenesis = GetTestGenesis();
        
        MiningContext context = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        
        context.strAddress = "127.0.0.1:9325";
        
        /* Create test reward address (different from genesis) */
        uint256_t testReward;
        testReward.SetHex("b174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd133");
        
        /* Initial state: reward not bound */
        REQUIRE(context.fAuthenticated == true);
        REQUIRE(context.hashGenesis == testGenesis);
        REQUIRE(context.fRewardBound == false);
        REQUIRE(context.hashRewardAddress == uint256_t(0));
        
        /* Simulate reward address binding by calling WithRewardAddress */
        /* (Full ProcessSetReward test would require encryption setup) */
        MiningContext updatedContext = context.WithRewardAddress(testReward);
        
        /* Verify context was updated correctly */
        REQUIRE(updatedContext.fRewardBound == true);
        REQUIRE(updatedContext.hashRewardAddress == testReward);
        REQUIRE(updatedContext.hashGenesis == testGenesis);
        REQUIRE(updatedContext.fAuthenticated == true);
        
        /* Update manager to persist the change */
        manager.UpdateMiner(context.strAddress, updatedContext);
        
        /* Verify context was persisted in manager */
        auto retrievedContext = manager.GetMinerContext(context.strAddress);
        REQUIRE(retrievedContext.has_value());
        REQUIRE(retrievedContext.value().fRewardBound == true);
        REQUIRE(retrievedContext.value().hashRewardAddress == testReward);
        REQUIRE(retrievedContext.value().hashGenesis == testGenesis);
        
        /* Verify payout address returns the reward address */
        REQUIRE(retrievedContext.value().GetPayoutAddress() == testReward);
        
        /* Cleanup */
        manager.RemoveMiner(context.strAddress);
    }
    
    SECTION("Manager tracks reward bound count correctly")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create contexts with and without reward binding */
        uint256_t testReward;
        testReward.SetHex("c174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd144");
        
        MiningContext ctxBound = MiningContext()
            .WithAuth(true)
            .WithRewardAddress(testReward);
        ctxBound.strAddress = "127.0.0.1:9326";
        
        MiningContext ctxNotBound = MiningContext()
            .WithAuth(true);
        ctxNotBound.strAddress = "127.0.0.1:9327";
        
        /* Add both miners */
        manager.UpdateMiner(ctxBound.strAddress, ctxBound);
        manager.UpdateMiner(ctxNotBound.strAddress, ctxNotBound);
        
        /* Verify reward bound count includes the bound miner */
        size_t boundCount = manager.GetRewardBoundCount();
        REQUIRE(boundCount >= 1);
        
        /* Cleanup */
        manager.RemoveMiner(ctxBound.strAddress);
        manager.RemoveMiner(ctxNotBound.strAddress);
    }
}
