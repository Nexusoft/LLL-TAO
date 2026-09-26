/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/* Unit tests for the four independent safeguards re-applied from PR #563:
 *
 *   1. Per-channel difficulty cache (PaddedDifficultyCache with per-channel nCacheTime)
 *   2. Framing gate: !INCOMING.fLengthRead instead of INCOMING.LENGTH == 0
 *   3. Zero-difficulty rejection in cache, push, and template-build paths
 *   4. stateBest null guard before GetNextTargetRequired
 *
 * NOTE: The canonical TritiumBlock parser path that caused the PR #563 rejection
 * (block 6683025, parse failure at byte 225) is intentionally NOT included here.
 * The legacy wrapper-format SUBMIT_BLOCK parse path remains the primary path.
 */

#include <unit/catch2/catch.hpp>

/* Framing tests */
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>

/* Difficulty cache & null-guard tests */
#include <LLP/types/stateless_miner_connection.h>
#include <LLP/include/mining_constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <Util/include/runtime.h>

/* Zero-diff template path test */
#include <LLP/include/mining_template_payload.h>
#include <TAO/Ledger/types/tritium.h>

#include <atomic>
#include <thread>
#include <vector>


/* ══════════════════════════════════════════════════════════════════════════
 * TEST 1 — Per-channel difficulty cache independence
 *
 * Verifies that the PaddedDifficultyCache struct carries its own nCacheTime
 * per channel, so a cache write for channel N does not affect the freshness
 * sentinel for channel M (N != M).
 *
 * Before the fix: a single shared nDiffCacheTime controlled ALL channels.
 * A write to channel 2 would bump the timestamp and make channel 1's
 * stale-or-zero cached difficulty appear fresh — "fresh timestamp on stale
 * data" bug.
 * ══════════════════════════════════════════════════════════════════════════ */
TEST_CASE("Per-channel difficulty cache entries are independent", "[llp][mining][difficulty_cache]")
{
    using Cache = LLP::StatelessMinerConnection::PaddedDifficultyCache;

    /* Obtain a stable "now" for the test.  TTL is 1 second. */
    const uint64_t nNow = runtime::unifiedtimestamp();
    const uint64_t nFutureFresh = nNow;   /* within the TTL window */
    const uint64_t nStale       = 0;      /* never written — always a cache miss */

    /* Helper that builds a standalone cache entry (not using the class static)
     * to verify the struct semantics in isolation. */
    SECTION("Two cache slots start independent and stay independent after writes")
    {
        Cache slot1;
        Cache slot2;

        /* Both slots start empty */
        REQUIRE(slot1.nDifficulty.load() == 0u);
        REQUIRE(slot1.nCacheTime.load()  == 0u);
        REQUIRE(slot2.nDifficulty.load() == 0u);
        REQUIRE(slot2.nCacheTime.load()  == 0u);

        /* Write to slot 1 */
        slot1.nDifficulty.store(0xAAAA1111u);
        slot1.nCacheTime.store(nFutureFresh);

        /* slot 2 must remain untouched */
        REQUIRE(slot2.nDifficulty.load() == 0u);
        REQUIRE(slot2.nCacheTime.load()  == 0u);

        /* Write to slot 2 */
        slot2.nDifficulty.store(0xBBBB2222u);
        slot2.nCacheTime.store(nFutureFresh);

        /* slot 1 must retain its own values, unaffected by slot 2 write */
        REQUIRE(slot1.nDifficulty.load() == 0xAAAA1111u);
        REQUIRE(slot1.nCacheTime.load()  == nFutureFresh);
        REQUIRE(slot2.nDifficulty.load() == 0xBBBB2222u);
        REQUIRE(slot2.nCacheTime.load()  == nFutureFresh);
    }

    SECTION("Staling one channel leaves all others unaffected")
    {
        Cache slots[3];

        /* Pre-populate all three channels with distinct values */
        for(uint32_t ch = 0; ch < 3; ++ch)
        {
            slots[ch].nDifficulty.store(0x10000000u + ch);
            slots[ch].nCacheTime.store(nFutureFresh);
        }

        /* Expire channel 1 by zeroing its timestamp */
        slots[1].nCacheTime.store(nStale);

        /* Channel 0 and 2 must remain fresh */
        REQUIRE(slots[0].nCacheTime.load() == nFutureFresh);
        REQUIRE(slots[0].nDifficulty.load() == 0x10000000u);

        REQUIRE(slots[1].nCacheTime.load() == nStale);   /* expired */

        REQUIRE(slots[2].nCacheTime.load() == nFutureFresh);
        REQUIRE(slots[2].nDifficulty.load() == 0x10000002u);
    }

    SECTION("Concurrent writes to different channels do not corrupt each other")
    {
        /* Use the global static array directly to exercise the real storage. */
        auto& cache = LLP::StatelessMinerConnection::nDiffCacheValue;

        /* Reset all three channels */
        for(uint32_t ch = 0; ch < 3; ++ch)
        {
            cache[ch].nDifficulty.store(0u);
            cache[ch].nCacheTime.store(0u);
        }

        /* Launch one writer thread per channel, each writing a distinct sentinel */
        constexpr int kIterations = 500;
        auto writer = [&](uint32_t ch, uint32_t base)
        {
            for(int i = 0; i < kIterations; ++i)
            {
                cache[ch].nDifficulty.store(base + static_cast<uint32_t>(i));
                cache[ch].nCacheTime.store(nFutureFresh);
            }
        };

        std::thread t0(writer, 0, 0x00010000u);
        std::thread t1(writer, 1, 0x00020000u);
        std::thread t2(writer, 2, 0x00030000u);
        t0.join();
        t1.join();
        t2.join();

        /* Each channel's difficulty must belong to the right base range */
        const uint32_t d0 = cache[0].nDifficulty.load();
        const uint32_t d1 = cache[1].nDifficulty.load();
        const uint32_t d2 = cache[2].nDifficulty.load();

        REQUIRE(d0 >= 0x00010000u);
        REQUIRE(d0 <  0x00020000u);
        REQUIRE(d1 >= 0x00020000u);
        REQUIRE(d1 <  0x00030000u);
        REQUIRE(d2 >= 0x00030000u);
        REQUIRE(d2 <  0x00040000u);

        /* Timestamps must all be within the expected range */
        REQUIRE(cache[0].nCacheTime.load() > 0u);
        REQUIRE(cache[1].nCacheTime.load() > 0u);
        REQUIRE(cache[2].nCacheTime.load() > 0u);

        /* Clean up */
        for(uint32_t ch = 0; ch < 3; ++ch)
        {
            cache[ch].nDifficulty.store(0u);
            cache[ch].nCacheTime.store(0u);
        }
    }
}


/* ══════════════════════════════════════════════════════════════════════════
 * TEST 2 — Framing gate: fLengthRead correctly distinguishes "haven't read
 *           length yet" from "legitimately read a zero-length payload"
 *
 * The bug: the old gate was `INCOMING.LENGTH == 0`, which treated a genuine
 * zero-length packet (e.g. a header-only GET_BLOCK) the same as an unread
 * length field and would try to re-read 4 bytes from the NEXT packet's header.
 *
 * The fix: use `!INCOMING.fLengthRead` — SetLength() sets this flag
 * unconditionally when the 4-byte length field is physically consumed from
 * the wire, regardless of the numeric value it decodes to.
 * ══════════════════════════════════════════════════════════════════════════ */
TEST_CASE("fLengthRead gate correctly handles zero-length packets", "[llp][framing][packet]")
{
    using namespace LLP;

    SECTION("Legacy Packet: fLengthRead is false before SetLength, true after")
    {
        Packet pkt(0x81);  /* GET_BLOCK — header-only, LENGTH should be 0 */

        /* Before SetLength: length field has not been physically read */
        REQUIRE(pkt.LENGTH       == 0u);
        REQUIRE(pkt.fLengthRead  == false);
        /* Header() must be false until the length field is consumed */
        REQUIRE(pkt.Header()     == false);
        REQUIRE(pkt.Complete()   == false);

        /* Simulate the ReadPacket() path consuming the 4-byte length field */
        pkt.SetLength({0x00, 0x00, 0x00, 0x00});

        /* Now LENGTH is still 0, but fLengthRead is true */
        REQUIRE(pkt.LENGTH       == 0u);
        REQUIRE(pkt.fLengthRead  == true);
        /* With fLengthRead set, Header() transitions to true */
        REQUIRE(pkt.Header()     == true);
        /* A zero-length packet with no DATA requirement is complete */
        REQUIRE(pkt.Complete()   == true);
    }

    SECTION("StatelessPacket: fLengthRead is false before SetLength, true after zero-length")
    {
        StatelessPacket pkt(0xD081u);  /* STATELESS_GET_BLOCK */

        REQUIRE(pkt.LENGTH       == 0u);
        REQUIRE(pkt.fLengthRead  == false);
        REQUIRE(pkt.Header()     == false);
        REQUIRE(pkt.Complete()   == false);

        pkt.SetLength({0x00, 0x00, 0x00, 0x00});

        REQUIRE(pkt.LENGTH       == 0u);
        REQUIRE(pkt.fLengthRead  == true);
        REQUIRE(pkt.Header()     == true);
        REQUIRE(pkt.Complete()   == true);
    }

    SECTION("fLengthRead gate: LENGTH==0 without SetLength must NOT appear complete")
    {
        /* Directly verify that the old LENGTH==0 gate would have been wrong:
         * a packet can have LENGTH==0 at default construction without SetLength
         * ever being called, meaning the 4-byte length field hasn't been read. */
        Packet fresh;
        fresh.HEADER = 0x81;  /* valid header */

        /* The stale check (LENGTH == 0) would incorrectly say "length is read" */
        REQUIRE(fresh.LENGTH      == 0u);

        /* But fLengthRead correctly says "not yet read" */
        REQUIRE(fresh.fLengthRead == false);

        /* Therefore the packet is NOT complete (correct behaviour) */
        REQUIRE(fresh.Header()    == false);
        REQUIRE(fresh.Complete()  == false);
    }

    SECTION("StatelessPacket: same fLengthRead invariant holds for fresh-constructed packet")
    {
        StatelessPacket fresh(0xD009u);  /* some stateless opcode */

        REQUIRE(fresh.LENGTH      == 0u);
        REQUIRE(fresh.fLengthRead == false);
        REQUIRE(fresh.Header()    == false);
        REQUIRE(fresh.Complete()  == false);

        /* Only after SetLength does it become ready */
        fresh.SetLength({0x00, 0x00, 0x00, 0x05});
        REQUIRE(fresh.fLengthRead == true);
        REQUIRE(fresh.LENGTH      == 5u);
        REQUIRE(fresh.Header()    == true);
        REQUIRE(fresh.Complete()  == false);  /* still needs 5 data bytes */
    }
}


/* ══════════════════════════════════════════════════════════════════════════
 * TEST 3 — Zero-difficulty rejection
 *
 * Verifies that a zero nBits value is rejected before it can be cached,
 * pushed to a miner, or embedded in a block template payload.
 * ══════════════════════════════════════════════════════════════════════════ */
TEST_CASE("Zero-difficulty values are rejected before caching or payload creation",
          "[llp][mining][zero_diff]")
{
    SECTION("GetCachedDifficulty: with null stateBest, returns 0 and does not cache it")
    {
        /* Snapshot chain state so we can restore it */
        TAO::Ledger::BlockState savedState = TAO::Ledger::ChainState::tStateBest.load();

        /* Force a default (null) chain state — IsNull() == true because nBits == 0 */
        TAO::Ledger::BlockState nullState;
        TAO::Ledger::ChainState::tStateBest.store(nullState);

        /* Invalidate channel 1's cache entry */
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nDifficulty.store(0u);
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nCacheTime.store(0u);

        /* With null stateBest, GetCachedDifficulty must return 0 … */
        const uint32_t nDiff = LLP::StatelessMinerConnection::GetCachedDifficulty(1);
        REQUIRE(nDiff == 0u);

        /* … and must NOT have written anything to the cache (zero is poison) */
        REQUIRE(LLP::StatelessMinerConnection::nDiffCacheValue[1].nDifficulty.load() == 0u);
        /* nCacheTime must also remain 0 — a zero-diff should never be cached */
        REQUIRE(LLP::StatelessMinerConnection::nDiffCacheValue[1].nCacheTime.load() == 0u);

        /* Restore */
        TAO::Ledger::ChainState::tStateBest.store(savedState);
    }

    SECTION("BuildSharedTemplatePayload: block with nBits==0 returns INTERNAL_RETRY")
    {
        auto* pBlock = new TAO::Ledger::TritiumBlock();
        pBlock->nVersion = 9;
        pBlock->nChannel = TAO::Ledger::CHANNEL::PRIME;
        pBlock->nHeight  = 500;
        pBlock->nBits    = 0;  /* deliberately zero */
        pBlock->hashMerkleRoot = uint512_t(0x12345678u);

        LLP::SharedTemplatePayloadResult result =
            LLP::BuildSharedTemplatePayload(pBlock, "test");

        REQUIRE(result.fSuccess == false);
        REQUIRE(result.eReason == LLP::GetBlockPolicyReason::INTERNAL_RETRY);
        REQUIRE(result.nRetryAfterMs > 0u);
        REQUIRE(result.vPayload.empty());

        delete pBlock;
    }

    SECTION("BuildSharedTemplatePayload: block with non-zero nBits succeeds (after serialize)")
    {
        /* Note: this section only checks that a non-zero nBits passes the guard.
         * Full payload construction also requires a valid chain state + Serialize();
         * an empty Serialize() from a default block triggers TEMPLATE_NOT_READY,
         * which is a different (expected) failure mode. */
        auto* pBlock = new TAO::Ledger::TritiumBlock();
        pBlock->nVersion = 9;
        pBlock->nChannel = TAO::Ledger::CHANNEL::PRIME;
        pBlock->nHeight  = 500;
        pBlock->nBits    = 0x1a2b3c4du;  /* non-zero */
        pBlock->hashMerkleRoot = uint512_t(0x12345678u);

        LLP::SharedTemplatePayloadResult result =
            LLP::BuildSharedTemplatePayload(pBlock, "test");

        /* Must NOT be an INTERNAL_RETRY due to zero-nBits */
        REQUIRE(result.eReason != LLP::GetBlockPolicyReason::INTERNAL_RETRY);

        delete pBlock;
    }
}


/* ══════════════════════════════════════════════════════════════════════════
 * TEST 4 — stateBest null guard before GetNextTargetRequired
 *
 * A default-constructed BlockState has nBits == 0 (IsNull() == true).
 * Before the fix, GetCachedDifficulty would pass this into
 * GetNextTargetRequired() which could return 0 or produce undefined
 * behaviour depending on what RetargetPrime/RetargetHash do with a
 * bare default state.
 *
 * After the fix, GetCachedDifficulty returns 0 immediately (explicit
 * sentinel) without ever calling GetNextTargetRequired.
 * ══════════════════════════════════════════════════════════════════════════ */
TEST_CASE("stateBest null guard prevents call to GetNextTargetRequired with uninitialized state",
          "[llp][mining][state_guard]")
{
    SECTION("Default-constructed BlockState is null (IsNull() == true)")
    {
        TAO::Ledger::BlockState nullState;
        REQUIRE(nullState.IsNull() == true);   /* nBits == 0 */
        REQUIRE(nullState.nHeight == 0u);
    }

    SECTION("GetCachedDifficulty returns 0 sentinel for null chain state")
    {
        TAO::Ledger::BlockState savedState = TAO::Ledger::ChainState::tStateBest.load();

        /* Install a null/default chain state */
        TAO::Ledger::BlockState nullState;
        TAO::Ledger::ChainState::tStateBest.store(nullState);

        /* Invalidate the cache for all channels so we force a recalc attempt */
        for(uint32_t ch = 0; ch < 3; ++ch)
        {
            LLP::StatelessMinerConnection::nDiffCacheValue[ch].nDifficulty.store(0u);
            LLP::StatelessMinerConnection::nDiffCacheValue[ch].nCacheTime.store(0u);
        }

        /* All three channels must return 0 without crashing */
        REQUIRE(LLP::StatelessMinerConnection::GetCachedDifficulty(0) == 0u);
        REQUIRE(LLP::StatelessMinerConnection::GetCachedDifficulty(1) == 0u);
        REQUIRE(LLP::StatelessMinerConnection::GetCachedDifficulty(2) == 0u);

        TAO::Ledger::ChainState::tStateBest.store(savedState);
    }

    SECTION("GetCachedDifficulty returns cached value when state IS valid (cache hit)")
    {
        TAO::Ledger::BlockState savedState = TAO::Ledger::ChainState::tStateBest.load();

        /* Populate the cache for channel 1 with a known non-zero difficulty */
        const uint64_t nNow = runtime::unifiedtimestamp();
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nDifficulty.store(0xDEADBEEFu);
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nCacheTime.store(nNow);

        /* Even with null stateBest, a warm cache hit must return the cached value
         * without reaching the null-guard code path. */
        TAO::Ledger::BlockState nullState;
        TAO::Ledger::ChainState::tStateBest.store(nullState);

        const uint32_t nDiff = LLP::StatelessMinerConnection::GetCachedDifficulty(1);
        REQUIRE(nDiff == 0xDEADBEEFu);

        /* Clean up */
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nDifficulty.store(0u);
        LLP::StatelessMinerConnection::nDiffCacheValue[1].nCacheTime.store(0u);
        TAO::Ledger::ChainState::tStateBest.store(savedState);
    }
}
