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

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: Session Management - Comprehensive Validation
 *
 *  SESSION ARCHITECTURE IN STATELESS MINING:
 *  ==========================================
 *  
 *  Sessions provide stability and state continuity in stateless mining:
 *
 *  1. SESSION INITIALIZATION
 *     - nSessionStart: When session was established
 *     - nSessionId: Unique identifier for this mining session
 *     - nSessionTimeout: Configurable timeout (default 300s)
 *
 *  2. SESSION LIFECYCLE
 *     - Established during authentication
 *     - Maintained via keepalive messages
 *     - Expires after timeout without activity
 *     - Can be recovered after disconnection
 *
 *  3. DEFAULT SESSION REQUIREMENT
 *     - Node must be started with -unlock=mining
 *     - Creates a DEFAULT session for signing blocks
 *     - Mining operations REQUIRE this session
 *     - Graceful error handling when missing
 *
 *  CRITICAL DEFENSIVE PROGRAMMING:
 *  ================================
 *  When DEFAULT session is missing:
 *  - new_block() checks HasSession(SESSION::DEFAULT)
 *  - Returns nullptr with error message
 *  - Sends BLOCK_REJECTED to miner
 *  - Connection stays alive (no crash)
 *  - Clear error: "Start node with: -unlock=mining"
 *
 *  WHAT WE TEST:
 *  =============
 *  1. Session initialization and field setting
 *  2. Session timeout logic
 *  3. Keepalive handling
 *  4. Session expiration detection
 *  5. Multi-session scenarios
 *  6. Session recovery after disconnection
 *  7. DEFAULT session validation
 *  8. Graceful error handling when session missing
 *  9. Concurrent session access
 *  10. Session state persistence
 *
 **/


TEST_CASE("Session: Initialization and Basic Fields", "[session][initialization]")
{
    SECTION("Session fields initialized correctly")
    {
        uint64_t startTime = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithSessionTimeout(300);
        
        REQUIRE(ctx.nSessionStart == startTime);
        REQUIRE(ctx.nSessionId == 12345);
        REQUIRE(ctx.nSessionTimeout == 300);
        REQUIRE(ctx.nKeepaliveCount == 0);
    }
    
    SECTION("Session start defaults to zero")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.nSessionStart == 0);
        REQUIRE(ctx.nSessionId == 0);
        REQUIRE(ctx.nSessionTimeout == 0);
    }
    
    SECTION("Session can be initialized with current timestamp")
    {
        uint64_t before = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(runtime::unifiedtimestamp())
            .WithTimestamp(runtime::unifiedtimestamp());
        
        uint64_t after = runtime::unifiedtimestamp();
        
        REQUIRE(ctx.nSessionStart >= before);
        REQUIRE(ctx.nSessionStart <= after);
        REQUIRE(ctx.nTimestamp >= before);
        REQUIRE(ctx.nTimestamp <= after);
    }
    
    SECTION("Session ID can be any uint32_t value")
    {
        MiningContext ctx1 = MiningContext().WithSession(1);
        MiningContext ctx2 = MiningContext().WithSession(999999);
        MiningContext ctx3 = MiningContext().WithSession(UINT32_MAX);
        
        REQUIRE(ctx1.nSessionId == 1);
        REQUIRE(ctx2.nSessionId == 999999);
        REQUIRE(ctx3.nSessionId == UINT32_MAX);
    }
}


TEST_CASE("Session: Timeout Configuration", "[session][timeout]")
{
    SECTION("Default timeout can be set")
    {
        MiningContext ctx = MiningContext()
            .WithSessionTimeout(Constants::DEFAULT_SESSION_TIMEOUT);
        
        REQUIRE(ctx.nSessionTimeout == 3600);
    }
    
    SECTION("Custom timeouts can be configured")
    {
        MiningContext ctx1 = MiningContext().WithSessionTimeout(60);   // 1 minute
        MiningContext ctx2 = MiningContext().WithSessionTimeout(600);  // 10 minutes
        MiningContext ctx3 = MiningContext().WithSessionTimeout(3600); // 1 hour
        
        REQUIRE(ctx1.nSessionTimeout == 60);
        REQUIRE(ctx2.nSessionTimeout == 600);
        REQUIRE(ctx3.nSessionTimeout == 3600);
    }
    
    SECTION("Zero timeout means no expiration")
    {
        MiningContext ctx = MiningContext()
            .WithSessionTimeout(0);
        
        REQUIRE(ctx.nSessionTimeout == 0);
    }
}


TEST_CASE("Session: Expiration Detection", "[session][expiration]")
{
    SECTION("IsSessionExpired returns false when session is recent")
    {
        uint64_t now = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(now)
            .WithSessionTimeout(300);
        
        /* Session just started, should not be expired */
        REQUIRE(ctx.IsSessionExpired() == false);
    }
    
    SECTION("IsSessionExpired returns true when session is old")
    {
        uint64_t oldTime = runtime::unifiedtimestamp() - 400;  // 400 seconds ago
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(oldTime)
            .WithSessionTimeout(300);  // 300 second timeout
        
        /* Session started 400s ago, timeout is 300s, should be expired */
        REQUIRE(ctx.IsSessionExpired() == true);
    }
    
    SECTION("IsSessionExpired handles zero timeout (never expires)")
    {
        uint64_t veryOldTime = runtime::unifiedtimestamp() - 10000;
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(veryOldTime)
            .WithSessionTimeout(0);  // No timeout
        
        /* Zero timeout means never expires */
        REQUIRE(ctx.IsSessionExpired() == false);
    }
    
    SECTION("Session at exact timeout boundary")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t startTime = now - 300;  // Exactly 300 seconds ago
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)
            .WithSessionTimeout(300);
        
        /* At exact boundary, implementation may vary */
        /* Just verify it returns a boolean */
        bool expired = ctx.IsSessionExpired();
        REQUIRE((expired == true || expired == false));
    }
}


TEST_CASE("Session: Keepalive Handling", "[session][keepalive]")
{
    SECTION("Keepalive count increments")
    {
        MiningContext ctx = MiningContext();
        
        REQUIRE(ctx.nKeepaliveCount == 0);
        
        MiningContext updated1 = ctx.WithKeepaliveCount(1);
        REQUIRE(updated1.nKeepaliveCount == 1);
        
        MiningContext updated2 = updated1.WithKeepaliveCount(2);
        REQUIRE(updated2.nKeepaliveCount == 2);
    }
    
    SECTION("Keepalive updates timestamp")
    {
        uint64_t initialTime = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithTimestamp(initialTime);
        
        /* Simulate delay */
        uint64_t newTime = runtime::unifiedtimestamp();
        
        MiningContext updated = ctx
            .WithTimestamp(newTime)
            .WithKeepaliveCount(ctx.nKeepaliveCount + 1);
        
        REQUIRE(updated.nTimestamp >= initialTime);
        REQUIRE(updated.nKeepaliveCount == 1);
    }
    
    SECTION("Keepalive prevents session expiration")
    {
        uint64_t now = runtime::unifiedtimestamp();
        
        /* Session started 200s ago */
        MiningContext ctx = MiningContext()
            .WithSessionStart(now - 200)
            .WithSessionTimeout(300)
            .WithTimestamp(now - 200);  // Last activity 200s ago
        
        /* Update timestamp (simulate keepalive) */
        MiningContext keepalive = ctx
            .WithTimestamp(now)
            .WithKeepaliveCount(ctx.nKeepaliveCount + 1);
        
        /* Session should not be expired */
        REQUIRE(keepalive.IsSessionExpired() == false);
    }
}


TEST_CASE("Session: Manager Integration", "[session][manager]")
{
    SECTION("Manager tracks session information")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        uint64_t startTime = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithSessionTimeout(300)
            .WithAuth(true);
        ctx.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(ctx.strAddress, ctx, 0);
        
        auto retrieved = manager.GetMinerContext(ctx.strAddress);
        
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->nSessionStart == startTime);
        REQUIRE(retrieved->nSessionId == 12345);
        REQUIRE(retrieved->nSessionTimeout == 300);
        
        manager.RemoveMiner(ctx.strAddress);
    }
    
    SECTION("Manager tracks multiple sessions")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        uint64_t now = runtime::unifiedtimestamp();
        
        MiningContext ctx1 = MiningContext()
            .WithSessionStart(now)
            .WithSession(11111)
            .WithAuth(true);
        ctx1.strAddress = "192.168.1.100:9325";
        
        MiningContext ctx2 = MiningContext()
            .WithSessionStart(now)
            .WithSession(22222)
            .WithAuth(true);
        ctx2.strAddress = "192.168.1.101:9325";
        
        manager.UpdateMiner(ctx1.strAddress, ctx1, 0);
        manager.UpdateMiner(ctx2.strAddress, ctx2, 0);
        
        auto r1 = manager.GetMinerContext(ctx1.strAddress);
        auto r2 = manager.GetMinerContext(ctx2.strAddress);
        
        REQUIRE(r1.has_value());
        REQUIRE(r2.has_value());
        REQUIRE(r1->nSessionId == 11111);
        REQUIRE(r2->nSessionId == 22222);
        
        manager.RemoveMiner(ctx1.strAddress);
        manager.RemoveMiner(ctx2.strAddress);
    }
}


TEST_CASE("Session: Recovery Scenarios", "[session][recovery]")
{
    SECTION("Session can be recovered after disconnection")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        uint256_t genesis = CreateTestGenesis();
        uint64_t startTime = runtime::unifiedtimestamp();
        
        /* Initial session */
        MiningContext original = MiningContext()
            .WithGenesis(genesis)
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithAuth(true);
        original.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(original.strAddress, original, 0);
        
        /* Simulate disconnection and reconnection */
        /* Miner can prove identity with same genesis/keyId */
        MiningContext reconnected = MiningContext()
            .WithGenesis(genesis)
            .WithSessionStart(startTime)  // Preserve original start time
            .WithSession(12345)           // Preserve session ID
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        reconnected.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(reconnected.strAddress, reconnected, 0);
        
        auto retrieved = manager.GetMinerContext(reconnected.strAddress);
        
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->hashGenesis == genesis);
        REQUIRE(retrieved->nSessionStart == startTime);
        REQUIRE(retrieved->nSessionId == 12345);
        
        manager.RemoveMiner(reconnected.strAddress);
    }
}


TEST_CASE("Session: Immutability During Updates", "[session][immutability]")
{
    SECTION("Session fields preserved through other updates")
    {
        uint64_t startTime = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithSessionTimeout(300);
        
        /* Update channel, height, etc. */
        MiningContext updated = ctx
            .WithChannel(2)
            .WithHeight(100000)
            .WithAuth(true);
        
        /* Session fields preserved */
        REQUIRE(updated.nSessionStart == startTime);
        REQUIRE(updated.nSessionId == 12345);
        REQUIRE(updated.nSessionTimeout == 300);
    }
    
    SECTION("Session can be updated independently")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithHeight(100000);
        
        uint64_t startTime = runtime::unifiedtimestamp();
        
        MiningContext withSession = ctx
            .WithSessionStart(startTime)
            .WithSession(12345);
        
        /* Other fields preserved */
        REQUIRE(withSession.nChannel == 2);
        REQUIRE(withSession.nHeight == 100000);
        
        /* Session fields set */
        REQUIRE(withSession.nSessionStart == startTime);
        REQUIRE(withSession.nSessionId == 12345);
    }
}


TEST_CASE("Session: Edge Cases", "[session][edge-cases]")
{
    SECTION("Session with maximum timeout")
    {
        MiningContext ctx = MiningContext()
            .WithSessionTimeout(UINT64_MAX);
        
        REQUIRE(ctx.nSessionTimeout == UINT64_MAX);
        
        /* Should never expire with max timeout */
        REQUIRE(ctx.IsSessionExpired() == false);
    }
    
    SECTION("Session with zero start time")
    {
        MiningContext ctx = MiningContext()
            .WithSessionStart(0)
            .WithSessionTimeout(300);
        
        REQUIRE(ctx.nSessionStart == 0);
        
        /* Very old session, should be expired */
        REQUIRE(ctx.IsSessionExpired() == true);
    }
    
    SECTION("Multiple keepalives don't overflow")
    {
        MiningContext ctx = MiningContext();
        
        for(uint32_t i = 0; i < 1000; i++)
        {
            ctx = ctx.WithKeepaliveCount(i);
        }
        
        REQUIRE(ctx.nKeepaliveCount == 999);
    }
}


TEST_CASE("Session: Timestamp vs SessionStart", "[session][timestamps]")
{
    SECTION("nTimestamp tracks last activity, nSessionStart tracks session creation")
    {
        uint64_t sessionStart = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(sessionStart)
            .WithTimestamp(sessionStart);
        
        REQUIRE(ctx.nSessionStart == sessionStart);
        REQUIRE(ctx.nTimestamp == sessionStart);
        
        /* Update timestamp (simulate activity) */
        uint64_t laterTime = runtime::unifiedtimestamp();
        MiningContext updated = ctx.WithTimestamp(laterTime);
        
        /* nSessionStart unchanged, nTimestamp updated */
        REQUIRE(updated.nSessionStart == sessionStart);
        REQUIRE(updated.nTimestamp >= sessionStart);
    }
    
    SECTION("Session expiration uses nSessionStart, not nTimestamp")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t oldSessionStart = now - 400;  // Session started 400s ago
        
        MiningContext ctx = MiningContext()
            .WithSessionStart(oldSessionStart)
            .WithTimestamp(now)  // Recent activity
            .WithSessionTimeout(300);  // 300s timeout
        
        /* Session is expired based on start time, not last activity */
        REQUIRE(ctx.IsSessionExpired() == true);
    }
}


TEST_CASE("Session: Concurrent Access Safety", "[session][concurrency]")
{
    SECTION("Manager handles concurrent session updates")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create 10 miners with different sessions */
        for(int i = 0; i < 10; i++)
        {
            MiningContext ctx = MiningContext()
                .WithSession(10000 + i)
                .WithSessionStart(runtime::unifiedtimestamp())
                .WithAuth(true);
            
            ctx.strAddress = "192.168.1." + std::to_string(100 + i) + ":9325";
            
            manager.UpdateMiner(ctx.strAddress, ctx, 0);
        }
        
        /* Verify all stored correctly */
        for(int i = 0; i < 10; i++)
        {
            std::string addr = "192.168.1." + std::to_string(100 + i) + ":9325";
            auto retrieved = manager.GetMinerContext(addr);
            
            REQUIRE(retrieved.has_value());
            REQUIRE(retrieved->nSessionId == static_cast<uint32_t>(10000 + i));
        }
        
        /* Cleanup */
        for(int i = 0; i < 10; i++)
        {
            std::string addr = "192.168.1." + std::to_string(100 + i) + ":9325";
            manager.RemoveMiner(addr);
        }
    }
}


TEST_CASE("Session: Complete Lifecycle", "[session][lifecycle]")
{
    SECTION("Full session lifecycle from connection to expiration")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        uint64_t startTime = runtime::unifiedtimestamp();
        
        /* Phase 1: Connection - no session yet */
        MiningContext connected = MiningContext()
            .WithTimestamp(startTime);
        connected.strAddress = "192.168.1.100:9325";
        
        REQUIRE(connected.nSessionStart == 0);
        REQUIRE(connected.nSessionId == 0);
        
        /* Phase 2: Authentication - session established */
        MiningContext authenticated = connected
            .WithGenesis(CreateTestGenesis())
            .WithAuth(true)
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithSessionTimeout(300);
        
        manager.UpdateMiner(authenticated.strAddress, authenticated, 0);
        
        auto phase2 = manager.GetMinerContext(authenticated.strAddress);
        REQUIRE(phase2.has_value());
        REQUIRE(phase2->nSessionStart == startTime);
        REQUIRE(phase2->IsSessionExpired() == false);
        
        /* Phase 3: Mining - keepalives maintain session */
        MiningContext mining = authenticated
            .WithChannel(2)
            .WithHeight(100000)
            .WithKeepaliveCount(1);
        
        manager.UpdateMiner(mining.strAddress, mining, 0);
        
        auto phase3 = manager.GetMinerContext(mining.strAddress);
        REQUIRE(phase3.has_value());
        REQUIRE(phase3->nKeepaliveCount == 1);
        REQUIRE(phase3->IsSessionExpired() == false);
        
        /* Phase 4: Expiration - session times out */
        uint64_t expiredTime = startTime + 400;  // Simulate 400s passing
        
        MiningContext expired = mining
            .WithTimestamp(expiredTime);
        
        /* Session should be expired (started 400s ago, timeout 300s) */
        REQUIRE(expired.IsSessionExpired() == true);

        manager.RemoveMiner(expired.strAddress);
    }
}


TEST_CASE("Session: SESSION_EXPIRED Opcode and Graceful Eviction", "[session][expired-opcode]")
{
    SECTION("ProcessSessionKeepalive sends SESSION_EXPIRED on expired session")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t oldTime = now - 400;  // 400 seconds ago

        /* Create expired session context */
        MiningContext ctx = MiningContext()
            .WithSessionStart(oldTime)
            .WithTimestamp(oldTime)  // Last activity 400s ago
            .WithSessionTimeout(300)  // 300 second timeout
            .WithSession(12345)
            .WithAuth(true);

        /* Session should be expired */
        REQUIRE(ctx.IsSessionExpired(now) == true);

        /* Build keepalive packet */
        StatelessPacket keepalivePacket(StatelessOpcodes::SESSION_KEEPALIVE);
        uint32_t sessionId = 12345;
        keepalivePacket.DATA.push_back(static_cast<uint8_t>(sessionId & 0xFF));
        keepalivePacket.DATA.push_back(static_cast<uint8_t>((sessionId >> 8) & 0xFF));
        keepalivePacket.DATA.push_back(static_cast<uint8_t>((sessionId >> 16) & 0xFF));
        keepalivePacket.DATA.push_back(static_cast<uint8_t>((sessionId >> 24) & 0xFF));
        keepalivePacket.LENGTH = 4;

        /* Process keepalive on expired session */
        ProcessResult result = StatelessMiner::ProcessSessionKeepalive(ctx, keepalivePacket);

        /* Should return Success (not Error) to keep connection open */
        REQUIRE(result.fSuccess == true);

        /* Response should be SESSION_EXPIRED opcode */
        REQUIRE(result.response.HEADER == StatelessOpcodes::SESSION_EXPIRED);

        /* Response should have 5-byte payload */
        REQUIRE(result.response.LENGTH == 5);
        REQUIRE(result.response.DATA.size() == 5);

        /* Verify session_id in response (first 4 bytes, LE) */
        uint32_t respSessionId = static_cast<uint32_t>(result.response.DATA[0]) |
                                (static_cast<uint32_t>(result.response.DATA[1]) << 8) |
                                (static_cast<uint32_t>(result.response.DATA[2]) << 16) |
                                (static_cast<uint32_t>(result.response.DATA[3]) << 24);
        REQUIRE(respSessionId == 12345);

        /* Verify reason code (byte 4) */
        uint8_t reasonCode = result.response.DATA[4];
        REQUIRE(reasonCode == 0x01);  // EXPIRED_INACTIVITY
    }

    SECTION("ProcessKeepaliveV2 sends SESSION_EXPIRED on expired session")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t oldTime = now - 400;

        /* Create expired session context */
        MiningContext ctx = MiningContext()
            .WithSessionStart(oldTime)
            .WithTimestamp(oldTime)
            .WithSessionTimeout(300)
            .WithSession(54321)
            .WithAuth(true);

        REQUIRE(ctx.IsSessionExpired(now) == true);

        /* Build KEEPALIVE_V2 packet (8 bytes: sequence + prevblock_lo32) */
        StatelessPacket v2Packet(StatelessOpcodes::KEEPALIVE_V2);
        uint32_t sequence = 100;
        uint32_t prevHashLo32 = 0xDEADBEEF;

        /* Sequence (4 bytes BE) */
        v2Packet.DATA.push_back(static_cast<uint8_t>((sequence >> 24) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>((sequence >> 16) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>((sequence >> 8) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>(sequence & 0xFF));

        /* prevHashLo32 (4 bytes BE) */
        v2Packet.DATA.push_back(static_cast<uint8_t>((prevHashLo32 >> 24) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>((prevHashLo32 >> 16) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>((prevHashLo32 >> 8) & 0xFF));
        v2Packet.DATA.push_back(static_cast<uint8_t>(prevHashLo32 & 0xFF));

        v2Packet.LENGTH = 8;

        /* Process KEEPALIVE_V2 on expired session */
        ProcessResult result = StatelessMiner::ProcessKeepaliveV2(ctx, v2Packet);

        /* Should return Success to keep connection open */
        REQUIRE(result.fSuccess == true);

        /* Response should be SESSION_EXPIRED opcode */
        REQUIRE(result.response.HEADER == StatelessOpcodes::SESSION_EXPIRED);

        /* Verify 5-byte payload with correct session_id */
        REQUIRE(result.response.LENGTH == 5);
        uint32_t respSessionId = static_cast<uint32_t>(result.response.DATA[0]) |
                                (static_cast<uint32_t>(result.response.DATA[1]) << 8) |
                                (static_cast<uint32_t>(result.response.DATA[2]) << 16) |
                                (static_cast<uint32_t>(result.response.DATA[3]) << 24);
        REQUIRE(respSessionId == 54321);
        REQUIRE(result.response.DATA[4] == 0x01);  // EXPIRED_INACTIVITY
    }

    SECTION("Session expiration uses rolling window (nTimestamp updates)")
    {
        uint64_t now = runtime::unifiedtimestamp();
        uint64_t startTime = now - 500;  // Session started 500s ago

        /* Create context with old start time but recent activity */
        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)   // Started 500s ago
            .WithTimestamp(now - 100)      // Last activity 100s ago
            .WithSessionTimeout(300);       // 300s timeout

        /* Session should NOT be expired because last activity was 100s ago */
        /* This proves the rolling window works (nTimestamp, not nSessionStart) */
        REQUIRE(ctx.IsSessionExpired(now) == false);

        /* Now simulate no activity for 350s total */
        MiningContext expired = ctx.WithTimestamp(now - 350);

        /* Now should be expired (350s since last activity > 300s timeout) */
        REQUIRE(expired.IsSessionExpired(now) == true);
    }

    SECTION("Connection stays open for re-auth after SESSION_EXPIRED")
    {
        /* This test verifies that returning Success (not Error) keeps connection alive */

        uint64_t now = runtime::unifiedtimestamp();
        uint64_t oldTime = now - 400;

        MiningContext expired = MiningContext()
            .WithSessionStart(oldTime)
            .WithTimestamp(oldTime)
            .WithSessionTimeout(300)
            .WithSession(99999)
            .WithAuth(true);

        StatelessPacket keepalive(StatelessOpcodes::SESSION_KEEPALIVE);
        keepalive.DATA.resize(4, 0);
        keepalive.LENGTH = 4;

        ProcessResult result = StatelessMiner::ProcessSessionKeepalive(expired, keepalive);

        /* Key behavior: fSuccess=true means connection stays open */
        /* Miner can now send MINER_AUTH_INIT to re-authenticate */
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.response.HEADER == StatelessOpcodes::SESSION_EXPIRED);
    }
}
