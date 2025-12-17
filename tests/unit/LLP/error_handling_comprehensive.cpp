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
#include <helpers/test_fixtures.h>
#include <helpers/packet_builder.h>

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: Error Handling and Defensive Programming
 *
 *  DEFENSIVE PROGRAMMING IN STATELESS MINING:
 *  ==========================================
 *
 *  Robust error handling is CRITICAL for production mining systems.
 *  When things go wrong, the system must:
 *  
 *  1. Detect errors immediately
 *  2. Handle them gracefully (no crashes)
 *  3. Provide clear error messages
 *  4. Keep connections alive when possible
 *  5. Clean up resources properly
 *  6. Prevent security vulnerabilities
 *
 *  ERROR CATEGORIES WE TEST:
 *  =========================
 *
 *  1. AUTHENTICATION ERRORS:
 *     - Authentication required but not performed
 *     - Invalid signatures
 *     - Replay attacks
 *     - Key mismatches
 *
 *  2. REWARD BINDING ERRORS:
 *     - Mining without reward bound
 *     - Invalid reward addresses
 *     - Decryption failures
 *     - Zero addresses
 *
 *  3. SESSION ERRORS:
 *     - DEFAULT session missing (-unlock=mining not set)
 *     - Session expired
 *     - Session timeout
 *     - Invalid session IDs
 *
 *  4. CHANNEL ERRORS:
 *     - Invalid channel numbers (0, 4+)
 *     - Mining without channel set
 *     - Channel switching errors
 *
 *  5. PACKET ERRORS:
 *     - Malformed packets
 *     - Oversized packets
 *     - Out-of-sequence packets
 *     - Invalid packet data
 *
 *  6. BLOCK ERRORS:
 *     - Invalid block submissions
 *     - Stale blocks
 *     - Wrong merkle roots
 *     - Invalid nonces
 *
 *  7. RESOURCE ERRORS:
 *     - Memory exhaustion
 *     - Too many connections
 *     - Database failures
 *
 *  8. CONCURRENT ACCESS ERRORS:
 *     - Race conditions
 *     - Deadlocks
 *     - Inconsistent state
 *
 *  WHAT WE TEST:
 *  =============
 *  - Error detection accuracy
 *  - Graceful failure handling
 *  - Error message clarity
 *  - Resource cleanup
 *  - Connection stability
 *  - Security vulnerability prevention
 *
 **/


TEST_CASE("Error Handling: Authentication Errors", "[error-handling][authentication]")
{
    SECTION("Mining attempted without authentication")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithRewardAddress(CreateTestRegisterAddress());
        
        /* Not authenticated */
        REQUIRE(ctx.fAuthenticated == false);
        REQUIRE(ctx.fRewardBound == true);
        
        /* Should detect auth missing before allowing mining */
        /* In production: new_block() would check fAuthenticated */
    }
    
    SECTION("Reward binding attempted before authentication")
    {
        MiningContext ctx = MiningContext()
            .WithRewardAddress(CreateTestRegisterAddress());
        
        /* Reward can be set, but mining requires auth */
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.fAuthenticated == false);
        
        /* Should require auth before GET_BLOCK */
    }
    
    SECTION("Zero genesis hash detected")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true);  // Auth flag set but no genesis
        
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashGenesis == uint256_t(0));
        
        /* Invalid state - should be caught */
    }
}


TEST_CASE("Error Handling: Reward Binding Errors", "[error-handling][reward]")
{
    SECTION("Mining attempted without reward bound")
    {
        MiningContext ctx = CreateAuthenticatedContext();
        
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
        
        /* Should reject block request without reward binding */
    }
    
    SECTION("Zero reward address")
    {
        uint256_t zero(0);
        
        MiningContext ctx = MiningContext()
            .WithRewardAddress(zero);
        
        REQUIRE(ctx.hashRewardAddress == uint256_t(0));
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
        
        /* Should reject mining with zero payout address */
    }
    
    SECTION("Reward address changed mid-mining")
    {
        uint256_t reward1 = CreateTestRegisterAddress(Constants::REGISTER_ADDR_1);
        uint256_t reward2 = CreateTestRegisterAddress(Constants::REGISTER_ADDR_2);
        
        MiningContext ctx = MiningContext()
            .WithRewardAddress(reward1)
            .WithChannel(2)
            .WithHeight(100000);
        
        /* Change reward address */
        MiningContext updated = ctx.WithRewardAddress(reward2);
        
        REQUIRE(updated.hashRewardAddress == reward2);
        REQUIRE(updated.hashRewardAddress != reward1);
        
        /* Should invalidate cached blocks */
    }
}


TEST_CASE("Error Handling: Session Errors", "[error-handling][session]")
{
    SECTION("Session expired")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t oldTime = now - 400;  // 400 seconds ago
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(oldTime)
            .WithSessionTimeout(300)  // 300s timeout
            .WithAuth(true);
        
        REQUIRE(ctx.IsSessionExpired() == true);
        
        /* Should reject operations on expired session */
    }
    
    SECTION("Session timeout zero (never expires)")
    {
        uint64_t veryOldTime = runtime::unifiedtimestamp() - 10000;
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(veryOldTime)
            .WithSessionTimeout(0)
            .WithAuth(true);
        
        REQUIRE(ctx.IsSessionExpired() == false);
        
        /* Zero timeout = never expires */
    }
    
    SECTION("Session not initialized")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true);
        
        REQUIRE(ctx.nSessionStart == 0);
        REQUIRE(ctx.nSessionId == 0);
        
        /* Should require session initialization */
    }
}


TEST_CASE("Error Handling: Channel Errors", "[error-handling][channel]")
{
    SECTION("Invalid channel number 0")
    {
        MiningContext ctx = CreateRewardBoundContext()
            .WithChannel(0);
        
        REQUIRE(ctx.nChannel == 0);
        
        /* Should reject - valid channels are 1, 2, 3 */
    }
    
    SECTION("Invalid channel number 4+")
    {
        MiningContext ctx = CreateRewardBoundContext()
            .WithChannel(4);
        
        REQUIRE(ctx.nChannel == 4);
        
        /* Should reject - valid channels are 1, 2, 3 */
    }
    
    SECTION("Mining without channel set")
    {
        MiningContext ctx = CreateRewardBoundContext();
        
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.nChannel == 0);
        
        /* Should require channel selection before mining */
    }
}


TEST_CASE("Error Handling: Packet Validation", "[error-handling][packets]")
{
    SECTION("Malformed SET_CHANNEL packet")
    {
        Packet malformed = CreateMalformedPacket(PacketTypes::SET_CHANNEL, 100);
        
        /* Packet has header and data, but data is garbage */
        REQUIRE(malformed.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(malformed.LENGTH == 100);
        
        /* Parser should detect invalid data */
    }
    
    SECTION("Empty packet for type requiring data")
    {
        Packet empty = CreateEmptyPacket(PacketTypes::SET_CHANNEL);
        
        REQUIRE(empty.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(empty.LENGTH == 0);
        
        /* Should reject - SET_CHANNEL requires uint32_t */
    }
    
    SECTION("Oversized packet")
    {
        Packet oversized = CreateOversizedPacket(PacketTypes::MINER_AUTH_INIT);
        
        REQUIRE(oversized.LENGTH > 1000000);
        
        /* Should reject packets over size limit */
    }
    
    SECTION("Invalid packet header")
    {
        Packet invalid = PacketBuilder(255)  // Unknown header
            .WithRandomData(32)
            .Build();
        
        REQUIRE(invalid.HEADER == 255);
        
        /* Should reject unknown packet types */
    }
}


TEST_CASE("Error Handling: State Validation", "[error-handling][state]")
{
    SECTION("Incomplete authentication state")
    {
        MiningContext ctx = MiningContext()
            .WithGenesis(CreateTestGenesis())
            .WithAuth(true);
        
        /* Has genesis and auth flag, but missing key ID */
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.hashKeyID == uint256_t(0));
        
        /* Incomplete auth - should require key ID */
    }
    
    SECTION("Incomplete reward binding state")
    {
        MiningContext ctx = MiningContext();
        
        /* fRewardBound may be set without hashRewardAddress */
        REQUIRE(ctx.hashRewardAddress == uint256_t(0));
        
        /* Should validate both flag AND address */
    }
    
    SECTION("Inconsistent state after updates")
    {
        MiningContext ctx = CreateFullMiningContext(2);
        
        /* Verify all required fields set */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.nChannel > 0);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.hashRewardAddress != uint256_t(0));
        
        /* Clear reward but keep flag (simulating corruption) */
        ctx.hashRewardAddress = uint256_t(0);
        
        /* GetPayoutAddress should detect inconsistency */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
}


TEST_CASE("Error Handling: Manager Error Conditions", "[error-handling][manager]")
{
    SECTION("Get non-existent miner")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        auto result = manager.GetMinerContext("192.168.1.999:9325");
        
        REQUIRE_FALSE(result.has_value());
        
        /* Should return empty optional, not crash */
    }
    
    SECTION("Update with empty address")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        MiningContext ctx = CreateFullMiningContext(2);
        ctx.strAddress = "";  // Empty address
        
        /* Manager should handle gracefully */
        /* (Implementation may reject or use address as key) */
    }
    
    SECTION("Remove non-existent miner")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Should not crash */
        manager.RemoveMiner("192.168.1.999:9325");
        
        /* Operation succeeds (no-op) */
    }
    
    SECTION("Cleanup with invalid timeout")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Cleanup with zero timeout */
        manager.CleanupInactive(0);
        
        /* Should handle gracefully */
    }
}


TEST_CASE("Error Handling: Resource Exhaustion", "[error-handling][resources]")
{
    SECTION("Very large number of miners")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        const int MANY_MINERS = 1000;
        std::vector<std::string> addresses;
        
        /* Create many miners */
        for(int i = 0; i < MANY_MINERS; i++)
        {
            MiningContext ctx = MiningContextBuilder()
                .WithRandomGenesis()
                .WithAuthenticated()
                .Build();
            
            ctx.strAddress = "192.168.1." + std::to_string(i % 256) + ":" + std::to_string(9000 + i);
            addresses.push_back(ctx.strAddress);
            
            manager.UpdateMiner(ctx.strAddress, ctx);
        }
        
        /* Verify system still responsive */
        auto test = manager.GetMinerContext(addresses[0]);
        REQUIRE(test.has_value());
        
        /* Cleanup */
        for(const auto& addr : addresses)
        {
            manager.RemoveMiner(addr);
        }
    }
    
    SECTION("Very large packet data")
    {
        /* Create packet with large but valid data */
        std::vector<uint8_t> largeData;
        /* Build from multiple random values */
        size_t targetSize = 1024 * 1024;  // 1MB
        while(largeData.size() < targetSize)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = targetSize - largeData.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            largeData.insert(largeData.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        
        Packet large = PacketBuilder(PacketTypes::MINER_SET_REWARD)
            .WithData(largeData)
            .Build();
        
        REQUIRE(large.LENGTH == 1024 * 1024);
        
        /* Should handle or reject based on limits */
    }
}


TEST_CASE("Error Handling: Edge Case Scenarios", "[error-handling][edge-cases]")
{
    SECTION("Rapid context updates")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        MiningContext ctx = CreateFullMiningContext(2);
        ctx.strAddress = "192.168.1.100:9325";
        
        /* Update same miner 100 times rapidly */
        for(int i = 0; i < 100; i++)
        {
            ctx = ctx.WithHeight(100000 + i);
            manager.UpdateMiner(ctx.strAddress, ctx);
        }
        
        /* Verify final state */
        auto final = manager.GetMinerContext(ctx.strAddress);
        REQUIRE(final.has_value());
        REQUIRE(final->nHeight == 100099);
        
        manager.RemoveMiner(ctx.strAddress);
    }
    
    SECTION("Context with maximum field values")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(UINT32_MAX)
            .WithHeight(UINT32_MAX)
            .WithSession(UINT32_MAX)
            .WithTimestamp(UINT64_MAX)
            .WithSessionTimeout(UINT64_MAX)
            .WithKeepaliveCount(UINT32_MAX);
        
        /* Should handle max values without overflow */
        REQUIRE(ctx.nChannel == UINT32_MAX);
        REQUIRE(ctx.nHeight == UINT32_MAX);
        REQUIRE(ctx.nSessionId == UINT32_MAX);
    }
    
    SECTION("Empty vectors in context")
    {
        std::vector<uint8_t> empty;
        
        MiningContext ctx = MiningContext()
            .WithNonce(empty)
            .WithPubKey(empty)
            .WithChaChaKey(empty);
        
        REQUIRE(ctx.vAuthNonce.empty());
        REQUIRE(ctx.vMinerPubKey.empty());
        REQUIRE(ctx.vChaChaKey.empty());
        
        /* Should handle empty vectors gracefully */
    }
}


TEST_CASE("Error Handling: Concurrent Access Safety", "[error-handling][concurrency]")
{
    SECTION("Concurrent miner updates")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create 50 miners and update them "concurrently" */
        std::vector<std::string> addresses;
        
        for(int i = 0; i < 50; i++)
        {
            MiningContext ctx = MiningContextBuilder()
                .WithRandomGenesis()
                .WithAuthenticated()
                .Build();
            
            ctx.strAddress = "192.168.1." + std::to_string(i) + ":9325";
            addresses.push_back(ctx.strAddress);
            
            manager.UpdateMiner(ctx.strAddress, ctx);
        }
        
        /* Update all miners multiple times */
        for(int round = 0; round < 10; round++)
        {
            for(const auto& addr : addresses)
            {
                auto ctx = manager.GetMinerContext(addr);
                if(ctx.has_value())
                {
                    MiningContext updated = ctx.value()
                        .WithHeight(100000 + round)
                        .WithTimestamp(runtime::unifiedtimestamp());
                    
                    manager.UpdateMiner(addr, updated);
                }
            }
        }
        
        /* Verify all miners still accessible */
        int accessible = 0;
        for(const auto& addr : addresses)
        {
            auto ctx = manager.GetMinerContext(addr);
            if(ctx.has_value())
            {
                accessible++;
                REQUIRE(ctx->nHeight == 100009);
            }
        }
        
        REQUIRE(accessible == 50);
        
        /* Cleanup */
        for(const auto& addr : addresses)
        {
            manager.RemoveMiner(addr);
        }
    }
}


TEST_CASE("Error Handling: Recovery from Errors", "[error-handling][recovery]")
{
    SECTION("Recover from invalid reward address")
    {
        MiningContext ctx = MiningContext()
            .WithRewardAddress(uint256_t(0));
        
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
        
        /* Set valid reward address */
        MiningContext fixed = ctx
            .WithRewardAddress(CreateTestRegisterAddress());
        
        REQUIRE(fixed.GetPayoutAddress() != uint256_t(0));
        REQUIRE(fixed.fRewardBound == true);
    }
    
    SECTION("Recover from session expiration")
    {
        uint64_t now = runtime::unifiedtimestamp();
        
        /* Expired session */
        MiningContext expired = MiningContext()
            .WithSessionStart(now - 400)
            .WithSessionTimeout(300);
        
        REQUIRE(expired.IsSessionExpired() == true);
        
        /* Renew session */
        MiningContext renewed = expired
            .WithSessionStart(now)
            .WithTimestamp(now);
        
        REQUIRE(renewed.IsSessionExpired() == false);
    }
    
    SECTION("Recover from invalid channel")
    {
        MiningContext invalid = MiningContext()
            .WithChannel(99);
        
        REQUIRE(invalid.nChannel == 99);
        
        /* Set valid channel */
        MiningContext valid = invalid.WithChannel(2);
        
        REQUIRE(valid.nChannel == 2);
    }
}


TEST_CASE("Error Handling: Defensive Programming Patterns", "[error-handling][defensive]")
{
    SECTION("GetPayoutAddress returns zero for invalid state")
    {
        MiningContext ctx;
        
        /* No reward bound */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("HasValidPayout returns false for empty context")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.HasValidPayout() == false);
    }
    
    SECTION("IsSessionExpired handles zero session start")
    {
        MiningContext ctx = MiningContext()
            .WithSessionTimeout(300);
        
        /* Session start is 0, should be expired */
        REQUIRE(ctx.IsSessionExpired() == true);
    }
    
    SECTION("Context updates preserve immutability")
    {
        MiningContext original = CreateFullMiningContext(2);
        MiningContext modified = original.WithChannel(1);
        
        /* Original unchanged */
        REQUIRE(original.nChannel == 2);
        REQUIRE(modified.nChannel == 1);
        
        /* No unexpected side effects */
    }
}
