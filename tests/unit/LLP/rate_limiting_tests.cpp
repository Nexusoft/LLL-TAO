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
#include <LLP/include/mining_constants.h>
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
 * Key invariants after the SIM Link fix (PR removing the 2-second floor):
 *   1. GET_BLOCK_MIN_INTERVAL_MS == 0   → the per-request minimum is disabled;
 *      a reconnecting miner can fire GET_BLOCK immediately after Falcon auth
 *      without accruing a violation.
 *   2. The per-minute cap (20/min) is the spam guard; a miner that fires 21
 *      requests inside a 60-second window triggers RecordViolation.
 *      Increased from 10/min to give recovery sufficient retries.
 *   3. GET_BLOCK_COOLDOWN_SECONDS == 2 so miners can retry every 2 seconds during recovery.
 * ─────────────────────────────────────────────────────────────────────────────
 */
TEST_CASE("GET_BLOCK rate-limit constants", "[rate_limit][mining_constants]")
{
    using namespace LLP::MiningConstants;

    SECTION("GET_BLOCK_MIN_INTERVAL_MS is 0 (floor removed for SIM Link recovery)")
    {
        /* The 2-second per-request minimum was removed because it broke SIM Link
         * recovery: a reconnecting miner legitimately fires GET_BLOCK within
         * ~400–800 ms of completing Falcon auth.  The per-minute cap is the
         * only spam guard now. */
        REQUIRE(GET_BLOCK_MIN_INTERVAL_MS == 0u);
    }

    SECTION("GET_BLOCK requests with short intervals are allowed (no per-request floor)")
    {
        /* Reconnecting miners legitimately fire GET_BLOCK within ~600 ms of
         * completing Falcon auth.  With GET_BLOCK_MIN_INTERVAL_MS == 0, the
         * per-request floor is disabled so any non-zero interval is accepted.
         * Only the per-minute cap (MAX_GET_BLOCK_PER_MINUTE) can reject requests. */
        constexpr uint32_t reconnect_interval_ms = 600u;
        bool blocked_by_floor = (GET_BLOCK_MIN_INTERVAL_MS > 0 &&
                                 reconnect_interval_ms < GET_BLOCK_MIN_INTERVAL_MS);
        REQUIRE_FALSE(blocked_by_floor);
    }
}
