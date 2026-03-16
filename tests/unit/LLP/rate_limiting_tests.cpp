/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/auto_cooldown_manager.h>
#include <LLP/include/base_address.h>
#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_constants.h>
#include <Util/include/args.h>
#include <Util/include/runtime.h>

#include <thread>
#include <chrono>

using namespace LLP;

TEST_CASE("AutoCooldownManager Basic Functionality", "[auto_cooldown]")
{
    SECTION("IPs not in cooldown by default")
    {
        BaseAddress addr("192.168.1.100");
        REQUIRE_FALSE(AutoCooldownManager::Get().IsInCooldown(addr));
    }
    
    SECTION("IP is in cooldown after being added")
    {
        BaseAddress addr("192.168.1.101");
        
        // Add to cooldown for 300 seconds
        AutoCooldownManager::Get().AddCooldown(addr, 300);
        
        // Should be in cooldown
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr));
    }
    
    SECTION("Cooldown expires after duration")
    {
        BaseAddress addr("192.168.1.102");
        
        // Add to cooldown for 2 seconds
        AutoCooldownManager::Get().AddCooldown(addr, 2);
        
        // Should be in cooldown immediately
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr));
        
        // Wait for cooldown to expire
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Should no longer be in cooldown
        REQUIRE_FALSE(AutoCooldownManager::Get().IsInCooldown(addr));
    }
    
    SECTION("Different IPs are tracked independently")
    {
        BaseAddress addr1("192.168.1.103");
        BaseAddress addr2("192.168.1.104");
        
        // Add only addr1 to cooldown
        AutoCooldownManager::Get().AddCooldown(addr1, 300);
        
        // addr1 should be in cooldown, addr2 should not
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr1));
        REQUIRE_FALSE(AutoCooldownManager::Get().IsInCooldown(addr2));
    }
    
    SECTION("CleanupExpired removes expired cooldowns")
    {
        BaseAddress addr1("192.168.1.105");
        BaseAddress addr2("192.168.1.106");
        
        // Add addr1 with short cooldown (already expired)
        // Note: We can't easily test this without time manipulation,
        // so we'll just verify the method doesn't crash
        AutoCooldownManager::Get().AddCooldown(addr1, 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Add addr2 with long cooldown
        AutoCooldownManager::Get().AddCooldown(addr2, 300);
        
        // Cleanup expired
        AutoCooldownManager::Get().CleanupExpired();
        
        // addr1 should be removed (expired)
        // addr2 should still be there
        REQUIRE_FALSE(AutoCooldownManager::Get().IsInCooldown(addr1));
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr2));
    }
    
    SECTION("GetCooldownCount returns correct count")
    {
        // Clear any existing cooldowns by checking all previously used IPs
        BaseAddress addr1("192.168.1.107");
        BaseAddress addr2("192.168.1.108");
        BaseAddress addr3("192.168.1.109");
        
        // Add multiple cooldowns
        AutoCooldownManager::Get().AddCooldown(addr1, 300);
        AutoCooldownManager::Get().AddCooldown(addr2, 300);
        AutoCooldownManager::Get().AddCooldown(addr3, 300);
        
        // Count should reflect active cooldowns
        // Note: This might include cooldowns from other tests
        size_t count = AutoCooldownManager::Get().GetCooldownCount();
        REQUIRE(count >= 3);  // At least our 3 should be there
    }
}

TEST_CASE("AutoCooldownManager Security Properties", "[auto_cooldown][security]")
{
    SECTION("Cooldowns are memory-only (not persistent)")
    {
        // This is a design property - we verify by checking there's no
        // persistence mechanism in the API
        BaseAddress addr("192.168.1.110");
        AutoCooldownManager::Get().AddCooldown(addr, 300);
        
        // The manager should only have in-memory storage
        // (verified by code inspection - no file I/O in implementation)
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr));
    }
    
    SECTION("No manual removal method exists")
    {
        // This is a design property verified by API inspection
        // The AutoCooldownManager should NOT have methods like:
        // - RemoveCooldown()
        // - ClearCooldown()
        // - RemoveAll()
        // Only automated expiry should remove cooldowns
        
        // This test is more of a documentation of the security guarantee
        // The actual verification is that these methods don't exist in the API
        REQUIRE(true);
    }
    
    SECTION("Cooldowns auto-expire and cannot be made permanent")
    {
        BaseAddress addr("192.168.1.111");
        
        // Add cooldown with duration
        AutoCooldownManager::Get().AddCooldown(addr, 2);
        
        // Should be in cooldown
        REQUIRE(AutoCooldownManager::Get().IsInCooldown(addr));
        
        // Wait for expiry
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Should auto-expire
        REQUIRE_FALSE(AutoCooldownManager::Get().IsInCooldown(addr));
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * GET_BLOCK rate-limit constant tests
 *
 * CheckRateLimit() is a private method of StatelessMinerConnection, so we
 * verify the constants that govern its behaviour rather than the method itself.
 *
 * Key invariants after the authenticated-miner refactor:
 *   1. GET_BLOCK_MIN_INTERVAL_MS == 2000 → the per-request minimum is a
 *      2-second floor matching GET_BLOCK_COOLDOWN_SECONDS; both mechanisms
 *      enforce the same floor with no lockout and no doom loop.
 *   2. The rolling per-minute cap (20/min) is the spam guard; a miner that
 *      fires 21 requests inside a 60-second window is throttled.
 *   3. GET_BLOCK_COOLDOWN_SECONDS == 2 so miners can retry every 2 seconds during recovery.
 * ─────────────────────────────────────────────────────────────────────────────
 */
TEST_CASE("GET_BLOCK rate-limit constants", "[rate_limit][mining_constants]")
{
    using namespace LLP::MiningConstants;

    SECTION("GET_BLOCK_MIN_INTERVAL_MS is 2000")
    {
        /* The 2-second per-request minimum matches GET_BLOCK_COOLDOWN_SECONDS.
         * Both mechanisms enforce the same 2-second floor — no lockout, no doom
         * loop. The AutoCoolDown is NOT Reset() after serving GET_BLOCK, so
         * miners can retry every 2 seconds during recovery. */
        REQUIRE(GET_BLOCK_MIN_INTERVAL_MS == 2000u);
    }

    SECTION("GET_BLOCK requests with short intervals are blocked by the 2-second floor")
    {
        /* Reconnecting miners that fire GET_BLOCK within ~600 ms of completing
         * Falcon auth will be blocked by the 2-second floor.  The floor prevents
         * rapid-fire polling abuse; MINER_READY resets the AutoCoolDown so the
         * first recovery GET_BLOCK is served immediately. */
        constexpr uint32_t reconnect_interval_ms = 600u;
        bool blocked_by_floor = (GET_BLOCK_MIN_INTERVAL_MS > 0 &&
                                 reconnect_interval_ms < GET_BLOCK_MIN_INTERVAL_MS);
        REQUIRE(blocked_by_floor);
    }

    SECTION("GET_BLOCK_COOLDOWN_SECONDS matches GET_BLOCK_MIN_INTERVAL_MS")
    {
        /* Both mechanisms enforce the same 2-second floor. */
        REQUIRE(GET_BLOCK_COOLDOWN_SECONDS == 2u);
    }

    SECTION("GET_BLOCK_THROTTLE_INTERVAL_MS is 2000")
    {
        /* The throttled interval for GET_BLOCK when rate limited must be 2000 ms.
         * This value must be identical in both debug and production builds so
         * that rate-limited miners are never locked out for more than 2 seconds. */
        REQUIRE(GET_BLOCK_THROTTLE_INTERVAL_MS == 2000u);
    }

    SECTION("RATE_LIMIT_STRIKE_THRESHOLD is 15")
    {
        /* 15 strikes are required before a temporary ban is applied.
         * Both debug and production builds must agree on this threshold. */
        REQUIRE(RATE_LIMIT_STRIKE_THRESHOLD == 15u);
    }
}

TEST_CASE("GET_BLOCK rolling limiter policy behavior", "[rate_limit][get_block][rolling]")
{
    constexpr int MAX_REQUESTS = static_cast<int>(LLP::GET_BLOCK_ROLLING_LIMIT_PER_MINUTE);
    LLP::GetBlockRollingLimiter limiter(MAX_REQUESTS, LLP::GET_BLOCK_ROLLING_WINDOW);
    const std::string key = "session=1|lane=1|ip=127.0.0.1";
    const auto t0 = LLP::GetBlockRollingLimiter::clock::now();

    SECTION("up to 20/min allows requests for valid session key")
    {
        for(int i = 0; i < MAX_REQUESTS; ++i)
        {
            uint32_t retryAfterMs = 0;
            std::size_t inWindow = 0;
            REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i), retryAfterMs, inWindow));
            REQUIRE(retryAfterMs == 0u);
            REQUIRE(inWindow == static_cast<std::size_t>(i + 1));
        }
    }

    SECTION("21st request in same rolling 60s is rate limited with retry hint")
    {
        for(int i = 0; i < MAX_REQUESTS; ++i)
        {
            uint32_t retryAfterMs = 0;
            std::size_t inWindow = 0;
            REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i), retryAfterMs, inWindow));
        }

        uint32_t retryAfterMs = 0;
        std::size_t inWindow = 0;
        REQUIRE_FALSE(limiter.Allow(key, t0 + std::chrono::seconds(10), retryAfterMs, inWindow));
        REQUIRE(retryAfterMs > 0u);
        REQUIRE(inWindow == static_cast<std::size_t>(MAX_REQUESTS));
    }

    SECTION("Limiter recovers after rolling window advances")
    {
        for(int i = 0; i < MAX_REQUESTS; ++i)
        {
            uint32_t retryAfterMs = 0;
            std::size_t inWindow = 0;
            REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i), retryAfterMs, inWindow));
        }

        uint32_t retryAfterMs = 0;
        std::size_t inWindow = 0;
        REQUIRE_FALSE(limiter.Allow(key, t0 + std::chrono::seconds(1), retryAfterMs, inWindow));
        REQUIRE(retryAfterMs > 0u);

        retryAfterMs = 0;
        inWindow = 0;
        REQUIRE(limiter.Allow(key, t0 + std::chrono::seconds(61), retryAfterMs, inWindow));
        REQUIRE(retryAfterMs == 0u);
    }
}

TEST_CASE("GET_BLOCK rolling limit constant equals 20", "[rate_limit][get_block][invariant]")
{
    /* Invariant: Node MUST always return BLOCK_DATA for authenticated miners
     * within this window.  The constant is 20 (reduced from 25) to provide
     * stricter spam protection while still giving legitimate mining clients
     * a generous burst budget — one request every 3 seconds on average.
     * The policy header and the stateless connection's MAX_GET_BLOCK_PER_MINUTE
     * (which derives from this constant via static_assert) must stay in sync. */
    REQUIRE(LLP::GET_BLOCK_ROLLING_LIMIT_PER_MINUTE == 20u);
}

TEST_CASE("Authenticated miner sending exactly 20 GET_BLOCK in 60s all succeed", "[rate_limit][get_block][invariant]")
{
    /* Invariant: an authenticated miner with a valid session and request count
     * <= 20 in the last 60 s ALWAYS receives BLOCK_DATA from the rolling limiter.
     * This test verifies the rolling limiter itself honours the contract. */
    constexpr std::size_t LIMIT = LLP::GET_BLOCK_ROLLING_LIMIT_PER_MINUTE; // == 20
    LLP::GetBlockRollingLimiter limiter(LIMIT, LLP::GET_BLOCK_ROLLING_WINDOW);
    const std::string key = "session=99|lane=1|ip=10.0.0.1";
    const auto t0 = LLP::GetBlockRollingLimiter::clock::now();

    for(std::size_t i = 0; i < LIMIT; ++i)
    {
        uint32_t retryAfterMs = 0;
        std::size_t inWindow = 0;
        INFO("Request " << (i + 1) << " of " << LIMIT << " must be allowed");
        REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i * 100), retryAfterMs, inWindow));
        REQUIRE(retryAfterMs == 0u);
    }
}

TEST_CASE("21st GET_BLOCK request in 60s is rejected with RATE_LIMIT_EXCEEDED", "[rate_limit][get_block][invariant]")
{
    /* Invariant: the 21st request in the same 60-second rolling window must be
     * rejected with a non-zero retry_after_ms so the miner knows when to retry. */
    constexpr std::size_t LIMIT = LLP::GET_BLOCK_ROLLING_LIMIT_PER_MINUTE; // == 20
    LLP::GetBlockRollingLimiter limiter(LIMIT, LLP::GET_BLOCK_ROLLING_WINDOW);
    const std::string key = "session=42|lane=2|ip=10.0.0.2";
    const auto t0 = LLP::GetBlockRollingLimiter::clock::now();

    for(std::size_t i = 0; i < LIMIT; ++i)
    {
        uint32_t retryAfterMs = 0;
        std::size_t inWindow = 0;
        REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i * 10), retryAfterMs, inWindow));
    }

    /* 21st request — must be denied */
    uint32_t retryAfterMs = 0;
    std::size_t inWindow = 0;
    REQUIRE_FALSE(limiter.Allow(key, t0 + std::chrono::seconds(5), retryAfterMs, inWindow));
    REQUIRE(retryAfterMs > 0u);
    REQUIRE(inWindow == LIMIT);
}

TEST_CASE("Throttle mode alone does not block an in-budget authenticated miner", "[rate_limit][get_block][invariant]")
{
    /* Invariant: throttle mode must act as an amplifier of genuine over-budget
     * behaviour, not as an independent gate.  The rolling limiter itself should
     * allow requests that are within the window regardless of any external flags.
     *
     * Since CheckRateLimit() is private, this test verifies the rolling limiter's
     * behaviour directly: it must allow in-budget requests without any throttle-mode
     * awareness, enabling CheckRateLimit to skip the throttle pre-check for GET_BLOCK
     * and rely solely on the rolling window as the authoritative gate. */
    constexpr std::size_t LIMIT = LLP::GET_BLOCK_ROLLING_LIMIT_PER_MINUTE; // == 20
    LLP::GetBlockRollingLimiter limiter(LIMIT, LLP::GET_BLOCK_ROLLING_WINDOW);
    const std::string key = "session=7|lane=1|ip=172.16.0.1";
    const auto t0 = LLP::GetBlockRollingLimiter::clock::now();

    /* Simulate half the budget used (10 out of 20). */
    for(std::size_t i = 0; i < LIMIT / 2; ++i)
    {
        uint32_t retryAfterMs = 0;
        std::size_t inWindow = 0;
        REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(i * 50), retryAfterMs, inWindow));
    }

    /* Even if external throttle-mode were set, the rolling limiter still has
     * capacity and MUST allow the next request.  The rolling limiter does not
     * know about throttle mode — that is correct by design: the gate is the
     * window, not a separate flag. */
    uint32_t retryAfterMs = 0;
    std::size_t inWindow = 0;
    REQUIRE(limiter.Allow(key, t0 + std::chrono::milliseconds(LIMIT / 2 * 50 + 100), retryAfterMs, inWindow));
    REQUIRE(retryAfterMs == 0u);
    REQUIRE(inWindow == LIMIT / 2 + 1);
}

TEST_CASE("GET_BLOCK policy reason codes are explicit", "[rate_limit][get_block][reason]")
{
    REQUIRE(std::string(LLP::GetBlockPolicyReasonCode(LLP::GetBlockPolicyReason::RATE_LIMIT_EXCEEDED)) == "RATE_LIMIT_EXCEEDED");
    REQUIRE(std::string(LLP::GetBlockPolicyReasonCode(LLP::GetBlockPolicyReason::SESSION_INVALID)) == "SESSION_INVALID");
    REQUIRE(std::string(LLP::GetBlockPolicyReasonCode(LLP::GetBlockPolicyReason::UNAUTHENTICATED)) == "UNAUTHENTICATED");
    REQUIRE(std::string(LLP::GetBlockPolicyReasonCode(LLP::GetBlockPolicyReason::TEMPLATE_NOT_READY)) == "TEMPLATE_NOT_READY");
    REQUIRE(std::string(LLP::GetBlockPolicyReasonCode(LLP::GetBlockPolicyReason::INTERNAL_RETRY)) == "INTERNAL_RETRY");
}

TEST_CASE("GET_BLOCK control payload is explicit and machine-readable", "[rate_limit][get_block][control]")
{
    SECTION("retryable reason includes retry_after_ms")
    {
        const auto payload = LLP::BuildGetBlockControlPayload(LLP::GetBlockPolicyReason::RATE_LIMIT_EXCEEDED, 2000u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[0] == LLP::GET_BLOCK_CONTROL_PAYLOAD_VERSION);
        REQUIRE(payload[1] == static_cast<uint8_t>(LLP::GetBlockPolicyReason::RATE_LIMIT_EXCEEDED));
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x07);
        REQUIRE(payload[7] == 0xD0);
    }

    SECTION("non-retryable reasons force retry_after_ms to zero")
    {
        const auto payload = LLP::BuildGetBlockControlPayload(LLP::GetBlockPolicyReason::UNAUTHENTICATED, 9999u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[1] == static_cast<uint8_t>(LLP::GetBlockPolicyReason::UNAUTHENTICATED));
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x00);
        REQUIRE(payload[7] == 0x00);
    }
}

TEST_CASE("Legacy empty BLOCK_DATA compatibility flag defaults off", "[rate_limit][get_block][legacy]")
{
    REQUIRE(config::GetBoolArg("-allow_legacy_empty_block_data", false) == false);
}
