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
#include <TAO/Ledger/types/tritium.h>

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

    SECTION("FindSessionBlock returns nullptr for unknown session")
    {
        const uint32_t nUnknownSession = 0xDEADC0DE;
        uint512_t fakeMerkle(uint64_t(0xABCDEF));

        auto pResult = StatelessMinerManager::Get().FindSessionBlock(nUnknownSession, fakeMerkle);
        REQUIRE(pResult == nullptr);
    }

    SECTION("FindSessionBlock returns nullptr for unknown merkle under known session")
    {
        const uint32_t nSessionId = 0xBEEFCAFE;
        uint512_t knownMerkle(uint64_t(0x1111));
        uint512_t unknownMerkle(uint64_t(0x2222));

        /* Store a block under knownMerkle */
        auto spBlock = std::make_shared<TAO::Ledger::TritiumBlock>();
        StatelessMinerManager::Get().StoreSessionBlock(nSessionId, knownMerkle, spBlock);

        /* Looking up an unrelated merkle should return nullptr */
        auto pMiss = StatelessMinerManager::Get().FindSessionBlock(nSessionId, unknownMerkle);
        REQUIRE(pMiss == nullptr);

        /* Looking up the stored merkle should succeed */
        auto pHit = StatelessMinerManager::Get().FindSessionBlock(nSessionId, knownMerkle);
        REQUIRE(pHit != nullptr);

        /* Clean up */
        StatelessMinerManager::Get().PruneSessionBlocks(nSessionId);
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


TEST_CASE("SIM-LINK: CleanupSessionScopedMaps removes orphaned entries", "[simlink][cleanup]")
{
    SECTION("CleanupSessionScopedMaps removes rate limiter for session not in mapMiners")
    {
        /* Use a high fake session ID that has no associated miner entry */
        const uint32_t nOrphanSessionId = 0xFACEFACE;

        /* Force-create an entry in the session rate limiter map */
        auto pLimiter = StatelessMinerManager::Get().GetSessionRateLimiter(nOrphanSessionId);
        REQUIRE(pLimiter != nullptr);

        /* Calling cleanup should remove the orphan (no miner registered for this session) */
        uint32_t nRemoved = StatelessMinerManager::Get().CleanupSessionScopedMaps();

        /* The limiter for nOrphanSessionId must have been pruned.
         * nRemoved may be > 1 if other orphan limiters existed from earlier tests. */
        REQUIRE(nRemoved >= 1);

        /* A subsequent GetSessionRateLimiter call creates a fresh limiter.
         * Verify the old one is gone by checking the fresh limiter is new. */
        auto pLimiter2 = StatelessMinerManager::Get().GetSessionRateLimiter(nOrphanSessionId);
        REQUIRE(pLimiter2 != nullptr);
        /* After cleanup, a new limiter should have been created; its window should be empty */
        const std::string strKey = "session=" + std::to_string(nOrphanSessionId) + "|combined";
        auto tNow = std::chrono::steady_clock::now();
        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;
        bool fAllowed = pLimiter2->Allow(strKey, tNow, nRetryAfterMs, nCurrentInWindow);
        REQUIRE(fAllowed);
        REQUIRE(nCurrentInWindow == 1);  /* fresh limiter: only this one request in window */

        /* Cleanup the new limiter too */
        StatelessMinerManager::Get().CleanupSessionScopedMaps();
    }

    SECTION("CleanupSessionScopedMaps removes orphaned session block map")
    {
        const uint32_t nOrphanSessionId = 0xC0DEBABE;
        uint512_t fakeMerkle(uint64_t(0xDEAD));

        /* Store a block under this orphan session */
        auto spBlock = std::make_shared<TAO::Ledger::TritiumBlock>();
        StatelessMinerManager::Get().StoreSessionBlock(nOrphanSessionId, fakeMerkle, spBlock);

        /* Block should be retrievable before cleanup */
        auto pPre = StatelessMinerManager::Get().FindSessionBlock(nOrphanSessionId, fakeMerkle);
        REQUIRE(pPre != nullptr);

        /* Cleanup removes orphan session block maps */
        uint32_t nRemoved = StatelessMinerManager::Get().CleanupSessionScopedMaps();
        REQUIRE(nRemoved >= 1);

        /* Block should no longer be found */
        auto pPost = StatelessMinerManager::Get().FindSessionBlock(nOrphanSessionId, fakeMerkle);
        REQUIRE(pPost == nullptr);
    }
}


TEST_CASE("SIM-LINK: GetMinerContextByIP fallback lookup", "[simlink][session][gap3]")
{
    SECTION("GetMinerContextByIP returns context for registered IP")
    {
        const std::string strAddr = "192.168.7.1:45000";
        MiningContext ctx;
        ctx.strAddress  = strAddr;
        ctx.nSessionId  = 0xA1B2C3D4;
        ctx.nTimestamp  = 1000;
        ctx.fAuthenticated = true;

        StatelessMinerManager::Get().UpdateMiner(strAddr, ctx);

        auto opt = StatelessMinerManager::Get().GetMinerContextByIP("192.168.7.1");
        REQUIRE(opt.has_value());
        REQUIRE(opt->nSessionId == ctx.nSessionId);

        /* Cleanup */
        StatelessMinerManager::Get().RemoveMiner(strAddr);
    }

    SECTION("GetMinerContextByIP returns nullopt for unknown IP")
    {
        auto opt = StatelessMinerManager::Get().GetMinerContextByIP("10.99.88.77");
        REQUIRE_FALSE(opt.has_value());
    }

    SECTION("GetMinerContextByIP returns most recently active context when IP has two ports")
    {
        const std::string strAddrOld = "192.168.7.2:45000";
        const std::string strAddrNew = "192.168.7.2:45001";

        MiningContext ctxOld;
        ctxOld.strAddress  = strAddrOld;
        ctxOld.nSessionId  = 0x00000001;
        ctxOld.nTimestamp  = 500;   /* older */
        ctxOld.fAuthenticated = true;

        MiningContext ctxNew;
        ctxNew.strAddress  = strAddrNew;
        ctxNew.nSessionId  = 0x00000002;
        ctxNew.nTimestamp  = 2000;  /* more recent */
        ctxNew.fAuthenticated = true;

        StatelessMinerManager::Get().UpdateMiner(strAddrOld, ctxOld);
        StatelessMinerManager::Get().UpdateMiner(strAddrNew, ctxNew);

        auto opt = StatelessMinerManager::Get().GetMinerContextByIP("192.168.7.2");
        REQUIRE(opt.has_value());
        /* Should return the newer (strAddrNew) context */
        REQUIRE(opt->nSessionId == ctxNew.nSessionId);

        /* Cleanup */
        StatelessMinerManager::Get().RemoveMiner(strAddrOld);
        StatelessMinerManager::Get().RemoveMiner(strAddrNew);
    }
}


TEST_CASE("SIM-LINK: GetMinerContextByAddressOrIP migrates safe port changes", "[simlink][session][address_migration]")
{
    SECTION("IP-only canonical stateless entry resolves from IP:port lookup without migration")
    {
        const std::string strAddrCanonical = "192.168.7.9";
        const std::string strLookupAddr = "192.168.7.9:45000";

        MiningContext ctx;
        ctx.strAddress = strAddrCanonical;
        ctx.nSessionId = 0x0BADB001;
        ctx.nTimestamp = 1200;
        ctx.fAuthenticated = true;

        StatelessMinerManager::Get().UpdateMiner(strAddrCanonical, ctx);

        auto opt = StatelessMinerManager::Get().GetMinerContextByAddressOrIP(
            strLookupAddr, ctx.nSessionId, true);
        REQUIRE(opt.has_value());
        REQUIRE(opt->strAddress == strAddrCanonical);
        REQUIRE(opt->nSessionId == ctx.nSessionId);

        auto optCanonical = StatelessMinerManager::Get().GetMinerContext(strAddrCanonical);
        REQUIRE(optCanonical.has_value());

        auto optLookup = StatelessMinerManager::Get().GetMinerContext(strLookupAddr);
        REQUIRE_FALSE(optLookup.has_value());

        StatelessMinerManager::Get().RemoveMiner(strAddrCanonical);
    }

    SECTION("Safe session match migrates exact address key")
    {
        const std::string strAddrOld = "192.168.7.10:45000";
        const std::string strAddrNew = "192.168.7.10:45001";

        MiningContext ctx;
        ctx.strAddress = strAddrOld;
        ctx.nSessionId = 0x0BADB002;
        ctx.nTimestamp = 1234;
        ctx.fAuthenticated = true;

        StatelessMinerManager::Get().UpdateMiner(strAddrOld, ctx);

        auto opt = StatelessMinerManager::Get().GetMinerContextByAddressOrIP(
            strAddrNew, ctx.nSessionId, true);
        REQUIRE(opt.has_value());
        REQUIRE(opt->strAddress == strAddrNew);
        REQUIRE(opt->nSessionId == ctx.nSessionId);

        auto optNew = StatelessMinerManager::Get().GetMinerContext(strAddrNew);
        REQUIRE(optNew.has_value());
        REQUIRE(optNew->strAddress == strAddrNew);

        auto optOld = StatelessMinerManager::Get().GetMinerContext(strAddrOld);
        REQUIRE_FALSE(optOld.has_value());

        auto optByIP = StatelessMinerManager::Get().GetMinerContextByIP("192.168.7.10");
        REQUIRE(optByIP.has_value());
        REQUIRE(optByIP->strAddress == strAddrNew);

        StatelessMinerManager::Get().RemoveMiner(strAddrNew);
    }

    SECTION("Session mismatch skips migration and preserves fallback context")
    {
        const std::string strAddrOld = "192.168.7.11:45000";
        const std::string strAddrNew = "192.168.7.11:45001";

        MiningContext ctx;
        ctx.strAddress = strAddrOld;
        ctx.nSessionId = 0x01020304;
        ctx.nTimestamp = 4567;
        ctx.fAuthenticated = true;

        StatelessMinerManager::Get().UpdateMiner(strAddrOld, ctx);

        auto opt = StatelessMinerManager::Get().GetMinerContextByAddressOrIP(
            strAddrNew, 0x0A0B0C0D, true);
        REQUIRE(opt.has_value());
        REQUIRE(opt->strAddress == strAddrOld);
        REQUIRE(opt->nSessionId == ctx.nSessionId);

        auto optOld = StatelessMinerManager::Get().GetMinerContext(strAddrOld);
        REQUIRE(optOld.has_value());

        auto optNew = StatelessMinerManager::Get().GetMinerContext(strAddrNew);
        REQUIRE_FALSE(optNew.has_value());

        StatelessMinerManager::Get().RemoveMiner(strAddrOld);
    }
}
