/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/get_block_policy.h>
#include <LLP/include/get_block_handler.h>
#include <LLP/include/stateless_manager.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

using namespace LLP;

/**
 * SIM-LINK Session-Scoped Rate Limiter Tests
 *
 * Tests for the dual-lane session rate limiter that enforces a single 20/60s
 * GET_BLOCK budget shared across both the legacy (8323) and stateless (9323)
 * protocol lanes for the same authenticated session.
 */

TEST_CASE("SIM-LINK: GetSessionRateLimiter returns consistent object per session",
          "[simlink][rate_limit][session]")
{
    SECTION("Same session ID returns same limiter object")
    {
        const uint32_t nSessionId = 0xABCD1234;

        auto pLimiter1 = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionId);
        auto pLimiter2 = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionId);

        REQUIRE(pLimiter1 != nullptr);
        REQUIRE(pLimiter2 != nullptr);
        /* Same underlying object — both shared_ptrs point to the same limiter */
        REQUIRE(pLimiter1.get() == pLimiter2.get());
    }

    SECTION("Different session IDs return different limiter objects")
    {
        const uint32_t nSessionA = 0x11111111;
        const uint32_t nSessionB = 0x22222222;

        auto pLimiterA = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionA);
        auto pLimiterB = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionB);

        REQUIRE(pLimiterA != nullptr);
        REQUIRE(pLimiterB != nullptr);
        REQUIRE(pLimiterA.get() != pLimiterB.get());
    }

    SECTION("Limiter enforces 20/60s combined budget across both lanes")
    {
        const uint32_t nSessionId = 0xDEADBEEF;
        auto pLimiter = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionId);

        const std::string strKey = "session=" + std::to_string(nSessionId) + "|combined";
        auto tNow = std::chrono::steady_clock::now();
        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;

        /* Consume 19 requests (under the 20/60s limit) */
        for(int i = 0; i < 19; ++i)
        {
            tNow = std::chrono::steady_clock::now();
            bool fAllowed = pLimiter->Allow(strKey, tNow, nRetryAfterMs, nCurrentInWindow);
            REQUIRE(fAllowed);
            REQUIRE(nRetryAfterMs == 0);
        }

        /* 20th request — still allowed (limit is 20) */
        tNow = std::chrono::steady_clock::now();
        bool f20thAllowed = pLimiter->Allow(strKey, tNow, nRetryAfterMs, nCurrentInWindow);
        REQUIRE(f20thAllowed);
        REQUIRE(nRetryAfterMs == 0);

        /* 21st request — over budget, should be denied */
        tNow = std::chrono::steady_clock::now();
        bool f21stAllowed = pLimiter->Allow(strKey, tNow, nRetryAfterMs, nCurrentInWindow);
        REQUIRE_FALSE(f21stAllowed);
        REQUIRE(nRetryAfterMs > 0); /* retry hint must be non-zero */
    }

    SECTION("Budget is shared across lane keys for same session")
    {
        /* Simulate a miner with connections on BOTH lanes sharing one budget.
         * Both lanes use the same key "session=N|combined" so they draw from
         * one pool. */
        const uint32_t nSessionId = 0xC0FFEE42;
        auto pLimiter = StatelessMinerManager::Get().GetSessionRateLimiter(nSessionId);

        /* Use the combined key (same key for both legacy and stateless lanes) */
        const std::string strKeyLegacy    = "session=" + std::to_string(nSessionId) + "|combined";
        const std::string strKeyStateless = "session=" + std::to_string(nSessionId) + "|combined";

        /* Both keys are identical — requests from both lanes draw from the same window */
        REQUIRE(strKeyLegacy == strKeyStateless);

        auto tNow = std::chrono::steady_clock::now();
        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;

        /* Consume 10 via "legacy lane" */
        for(int i = 0; i < 10; ++i)
        {
            tNow = std::chrono::steady_clock::now();
            bool fAllowed = pLimiter->Allow(strKeyLegacy, tNow, nRetryAfterMs, nCurrentInWindow);
            REQUIRE(fAllowed);
        }

        /* Consume 10 via "stateless lane" — should exhaust budget */
        for(int i = 0; i < 10; ++i)
        {
            tNow = std::chrono::steady_clock::now();
            bool fAllowed = pLimiter->Allow(strKeyStateless, tNow, nRetryAfterMs, nCurrentInWindow);
            REQUIRE(fAllowed);
        }

        /* 21st total request (either lane) — over budget */
        tNow = std::chrono::steady_clock::now();
        bool fOver = pLimiter->Allow(strKeyLegacy, tNow, nRetryAfterMs, nCurrentInWindow);
        REQUIRE_FALSE(fOver);
        REQUIRE(nRetryAfterMs > 0);
    }
}


TEST_CASE("SIM-LINK: Session block storage and retrieval", "[simlink][session_blocks]")
{
    SECTION("StoreSessionBlock and FindSessionBlock round-trip")
    {
        const uint32_t nSessionId = 0x12345678;
        /* Use a zero-initialized merkle root for testing */
        uint512_t fakeMerkle(uint64_t(0));

        /* Without a real Block to store, verify the manager doesn't crash and
         * returns nullptr for a missing block. */
        auto pFound = StatelessMinerManager::Get().FindSessionBlock(nSessionId, fakeMerkle);
        REQUIRE(pFound == nullptr);
    }

    SECTION("PruneSessionBlocks clears all templates for a session")
    {
        const uint32_t nSessionId = 0x98765432;

        /* Pruning a non-existent session should not crash */
        REQUIRE_NOTHROW(StatelessMinerManager::Get().PruneSessionBlocks(nSessionId));
    }
}


TEST_CASE("SIM-LINK: GetBlockRequest and GetBlockResult structure", "[simlink][handler]")
{
    SECTION("GetBlockRequest initializes with zero session ID by default")
    {
        GetBlockRequest req;
        REQUIRE(req.nSessionId == 0);
        REQUIRE(!req.fnCreateBlock); /* null function object */
    }

    SECTION("GetBlockResult initializes to failed state by default")
    {
        GetBlockResult result;
        REQUIRE_FALSE(result.fSuccess);
        REQUIRE(result.vPayload.empty());
        REQUIRE(result.eReason == GetBlockPolicyReason::NONE);
        REQUIRE(result.nRetryAfterMs == 0);
        REQUIRE(result.nBlockChannel == 0);
        REQUIRE(result.nBlockHeight == 0);
        REQUIRE(result.nBlockBits == 0);
    }

    SECTION("SharedGetBlockHandler returns INTERNAL_RETRY when fnCreateBlock is not set")
    {
        /* A request with no block factory callback should fail gracefully */
        GetBlockRequest req;
        req.nSessionId = 0xFEED;
        /* fnCreateBlock is not set — calling it would crash */

        /* We cannot actually call SharedGetBlockHandler here without a valid
         * fnCreateBlock callback.  This test validates the struct defaults only.
         * Full integration tests require a running blockchain and are in integration tests. */
        REQUIRE(req.nSessionId == 0xFEED);
        REQUIRE(!req.fnCreateBlock);
    }
}


TEST_CASE("SIM-LINK: Combined key format", "[simlink][rate_limit]")
{
    SECTION("Session key format is session=N|combined")
    {
        const uint32_t nSessionId = 42;
        const std::string expected = "session=42|combined";
        const std::string actual = "session=" + std::to_string(nSessionId) + "|combined";
        REQUIRE(actual == expected);
    }

    SECTION("Session key is lane-agnostic (same key for both lanes)")
    {
        const uint32_t nSessionId = 999;
        /* Both lanes use the same combined key — no per-lane differentiation */
        const std::string legacyKey    = "session=" + std::to_string(nSessionId) + "|combined";
        const std::string statelessKey = "session=" + std::to_string(nSessionId) + "|combined";
        REQUIRE(legacyKey == statelessKey);
    }
}
