/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/node_cache.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/runtime.h>

using namespace LLP;

TEST_CASE("Node Cache Configuration Tests", "[node_cache]")
{
    SECTION("Default cache size is 500")
    {
        REQUIRE(NodeCache::DEFAULT_MAX_CACHE_SIZE == 500);
    }

    SECTION("Default keepalive interval is 24 hours")
    {
        REQUIRE(NodeCache::DEFAULT_KEEPALIVE_INTERVAL == 86400);
    }

    SECTION("Default purge timeout is 7 days")
    {
        REQUIRE(NodeCache::DEFAULT_CACHE_PURGE_TIMEOUT == 604800);
    }

    SECTION("Localhost purge timeout is 30 days")
    {
        REQUIRE(NodeCache::LOCALHOST_CACHE_PURGE_TIMEOUT == 2592000);
    }
}


TEST_CASE("Localhost Detection Tests", "[node_cache]")
{
    SECTION("Detects IPv4 localhost")
    {
        REQUIRE(NodeCache::IsLocalhost("127.0.0.1"));
        REQUIRE(NodeCache::IsLocalhost("127.0.0.1:8336"));
        REQUIRE(NodeCache::IsLocalhost("127.1.2.3"));
    }

    SECTION("Detects IPv6 localhost")
    {
        REQUIRE(NodeCache::IsLocalhost("::1"));
        REQUIRE(NodeCache::IsLocalhost("0:0:0:0:0:0:0:1"));
    }

    SECTION("Detects localhost name")
    {
        REQUIRE(NodeCache::IsLocalhost("localhost"));
        REQUIRE(NodeCache::IsLocalhost("LOCALHOST"));
    }

    SECTION("Rejects remote addresses")
    {
        REQUIRE_FALSE(NodeCache::IsLocalhost("192.168.1.1"));
        REQUIRE_FALSE(NodeCache::IsLocalhost("10.0.0.1"));
        REQUIRE_FALSE(NodeCache::IsLocalhost("8.8.8.8"));
        REQUIRE_FALSE(NodeCache::IsLocalhost("2001:0db8:85a3::1"));
    }
}


TEST_CASE("Purge Timeout Selection Tests", "[node_cache]")
{
    SECTION("Localhost gets extended timeout")
    {
        REQUIRE(NodeCache::GetPurgeTimeout("127.0.0.1") == NodeCache::LOCALHOST_CACHE_PURGE_TIMEOUT);
        REQUIRE(NodeCache::GetPurgeTimeout("::1") == NodeCache::LOCALHOST_CACHE_PURGE_TIMEOUT);
        REQUIRE(NodeCache::GetPurgeTimeout("localhost") == NodeCache::LOCALHOST_CACHE_PURGE_TIMEOUT);
    }

    SECTION("Remote addresses get standard timeout")
    {
        REQUIRE(NodeCache::GetPurgeTimeout("192.168.1.1") == NodeCache::DEFAULT_CACHE_PURGE_TIMEOUT);
        REQUIRE(NodeCache::GetPurgeTimeout("8.8.8.8") == NodeCache::DEFAULT_CACHE_PURGE_TIMEOUT);
    }
}


TEST_CASE("Cache Size Limit Enforcement Tests", "[node_cache]")
{
    /* Get fresh manager instance for testing */
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    SECTION("Cache limit enforcement removes oldest miners first")
    {
        /* Clear any existing miners */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }

        /* Add miners up to cache limit */
        for(size_t i = 0; i < 10; ++i)
        {
            std::string strAddress = "192.168.1." + std::to_string(i);
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - (10 - i) * 60);
            ctx.strAddress = strAddress;
            manager.UpdateMiner(strAddress, ctx);
        }

        REQUIRE(manager.GetMinerCount() == 10);

        /* Enforce lower limit to trigger removal */
        uint32_t nRemoved = manager.EnforceCacheLimit(5);
        
        REQUIRE(nRemoved == 5);
        REQUIRE(manager.GetMinerCount() == 5);

        /* Verify oldest miners were removed */
        for(size_t i = 0; i < 5; ++i)
        {
            std::string strAddress = "192.168.1." + std::to_string(i);
            REQUIRE_FALSE(manager.GetMinerContext(strAddress).has_value());
        }

        /* Verify newest miners remain */
        for(size_t i = 5; i < 10; ++i)
        {
            std::string strAddress = "192.168.1." + std::to_string(i);
            REQUIRE(manager.GetMinerContext(strAddress).has_value());
        }

        /* Cleanup */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }
    }

    SECTION("Cache limit prefers keeping localhost miners")
    {
        /* Clear cache */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }

        /* Add remote miners */
        for(size_t i = 0; i < 5; ++i)
        {
            std::string strAddress = "192.168.1." + std::to_string(i);
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - 100);
            ctx.strAddress = strAddress;
            manager.UpdateMiner(strAddress, ctx);
        }

        /* Add localhost miner (older timestamp but localhost) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - 1000);
            ctx.strAddress = "127.0.0.1";
            manager.UpdateMiner("127.0.0.1", ctx);
        }

        REQUIRE(manager.GetMinerCount() == 6);

        /* Enforce limit that should prefer keeping localhost */
        manager.EnforceCacheLimit(3);

        /* Localhost should still be present */
        REQUIRE(manager.GetMinerContext("127.0.0.1").has_value());

        /* Cleanup */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }
    }
}


TEST_CASE("Purge Inactive Miners Tests", "[node_cache]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    SECTION("Purge removes miners exceeding timeout")
    {
        /* Clear cache */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }

        /* Add inactive remote miner (8 days old - should be purged) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - (8 * 86400));
            ctx.strAddress = "192.168.1.100";
            manager.UpdateMiner("192.168.1.100", ctx);
        }

        /* Add active remote miner (1 day old - should remain) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - 86400);
            ctx.strAddress = "192.168.1.101";
            manager.UpdateMiner("192.168.1.101", ctx);
        }

        /* Add inactive localhost miner (8 days old - should remain due to extended timeout) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - (8 * 86400));
            ctx.strAddress = "127.0.0.1";
            manager.UpdateMiner("127.0.0.1", ctx);
        }

        REQUIRE(manager.GetMinerCount() == 3);

        /* Purge inactive */
        uint32_t nPurged = manager.PurgeInactiveMiners();

        /* Only the old remote miner should be purged */
        REQUIRE(nPurged == 1);
        REQUIRE(manager.GetMinerCount() == 2);
        REQUIRE_FALSE(manager.GetMinerContext("192.168.1.100").has_value());
        REQUIRE(manager.GetMinerContext("192.168.1.101").has_value());
        REQUIRE(manager.GetMinerContext("127.0.0.1").has_value());

        /* Cleanup */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }
    }
}


TEST_CASE("Keepalive Check Tests", "[node_cache]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    SECTION("Detects when keepalive is required")
    {
        /* Clear cache */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }

        /* Add miner with old timestamp (25 hours) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - (25 * 3600));
            ctx.strAddress = "192.168.1.1";
            manager.UpdateMiner("192.168.1.1", ctx);
        }

        /* Should require keepalive (24 hour default interval) */
        REQUIRE(manager.CheckKeepaliveRequired("192.168.1.1", NodeCache::DEFAULT_KEEPALIVE_INTERVAL));

        /* Add miner with recent timestamp (1 hour) */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - 3600);
            ctx.strAddress = "192.168.1.2";
            manager.UpdateMiner("192.168.1.2", ctx);
        }

        /* Should not require keepalive */
        REQUIRE_FALSE(manager.CheckKeepaliveRequired("192.168.1.2", NodeCache::DEFAULT_KEEPALIVE_INTERVAL));

        /* Cleanup */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }
    }
}


TEST_CASE("Cache DDOS Protection Integration Test", "[node_cache]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    SECTION("Adding miner when cache is full triggers automatic cleanup")
    {
        /* Clear cache */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }

        /* Fill cache to 500 entries */
        for(size_t i = 0; i < 500; ++i)
        {
            std::string strAddress = "10.0." + std::to_string(i / 256) + "." + std::to_string(i % 256);
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp() - (500 - i));
            ctx.strAddress = strAddress;
            manager.UpdateMiner(strAddress, ctx);
        }

        REQUIRE(manager.GetMinerCount() == 500);

        /* Add one more miner - should trigger cache enforcement */
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp());
            ctx.strAddress = "192.168.99.99";
            manager.UpdateMiner("192.168.99.99", ctx);
        }

        /* Cache should stay at or below limit */
        REQUIRE(manager.GetMinerCount() <= 501);

        /* The new miner should be in cache */
        REQUIRE(manager.GetMinerContext("192.168.99.99").has_value());

        /* Cleanup */
        while(manager.GetMinerCount() > 0)
        {
            auto miners = manager.ListMiners();
            if(!miners.empty())
                manager.RemoveMiner(miners[0].strAddress);
        }
    }
}
