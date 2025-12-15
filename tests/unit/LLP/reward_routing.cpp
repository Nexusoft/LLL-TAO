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

using namespace LLP;

/* Test constants */
namespace {
    /* Sample test genesis hash for reward routing tests */
    const char* TEST_GENESIS_HEX = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
    
    /* Helper to create test genesis hash */
    uint256_t GetTestGenesis()
    {
        uint256_t hash;
        hash.SetHex(TEST_GENESIS_HEX);
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
    
    SECTION("GetPayoutAddress returns zero when reward address not bound")
    {
        /* Create context without reward binding */
        MiningContext ctx;
        
        /* GetPayoutAddress should return zero */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
        REQUIRE(ctx.fRewardBound == false);
    }
    
    SECTION("Genesis is separate from reward address")
    {
        /* Create test hashes */
        uint256_t testGenesis = GetTestGenesis();
        uint256_t testReward;
        testReward.SetHex("b174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd133");
        
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
    SECTION("Authenticated miner must bind reward address")
    {
        uint256_t testGenesis = GetTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        /* Authenticated but reward address not bound yet */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("Reward address binding is required for mining")
    {
        uint256_t testGenesis = GetTestGenesis();
        uint256_t testReward = GetTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithRewardAddress(testReward);
        
        /* Only after binding reward address can mining proceed */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.GetPayoutAddress() == testReward);
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
