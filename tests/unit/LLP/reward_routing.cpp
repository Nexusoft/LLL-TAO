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


TEST_CASE("Dynamic Reward Routing Tests", "[reward_routing]")
{
    SECTION("MiningContext stores genesis hash for reward routing")
    {
        /* Create a test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        /* Create context with genesis */
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        REQUIRE(ctx.hashGenesis == testGenesis);
        REQUIRE(ctx.fAuthenticated == true);
    }
    
    SECTION("GetPayoutAddress returns genesis when set")
    {
        /* Create a test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        /* Create context with genesis */
        MiningContext ctx = MiningContext().WithGenesis(testGenesis);
        
        /* GetPayoutAddress should return the genesis hash */
        REQUIRE(ctx.GetPayoutAddress() == testGenesis);
    }
    
    SECTION("GetPayoutAddress returns zero for unauthenticated context")
    {
        /* Create context without genesis */
        MiningContext ctx;
        
        /* GetPayoutAddress should return zero */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("HasValidPayout returns true when genesis is set")
    {
        /* Create a test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        /* Create context with genesis */
        MiningContext ctx = MiningContext().WithGenesis(testGenesis);
        
        REQUIRE(ctx.HasValidPayout() == true);
    }
    
    SECTION("HasValidPayout returns false when genesis is zero")
    {
        /* Create context without genesis */
        MiningContext ctx;
        
        REQUIRE(ctx.HasValidPayout() == false);
    }
    
    SECTION("StatelessMinerManager tracks genesis correctly")
    {
        /* Get manager instance */
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create test context with genesis */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
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


TEST_CASE("Reward Routing Mode Tests", "[reward_routing]")
{
    SECTION("Authenticated miner with genesis uses dynamic mode")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithAuth(true);
        
        /* Dynamic mode when both authenticated and genesis set */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.HasValidPayout() == true);
    }
    
    SECTION("Unauthenticated miner falls back to static mode")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(false);
        
        /* Static mode when not authenticated */
        REQUIRE(ctx.fAuthenticated == false);
        REQUIRE(ctx.hashGenesis == uint256_t(0));
    }
    
    SECTION("Authenticated miner with zero genesis falls back to static mode")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithGenesis(uint256_t(0));
        
        /* Static mode when genesis is zero even if authenticated */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashGenesis == uint256_t(0));
        REQUIRE(ctx.HasValidPayout() == false);
    }
}
