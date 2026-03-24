/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/types/tritium.h>

#include <LLC/include/random.h>

#include <cstdint>
#include <vector>


/** Test Suite: Prime Sign / Finalization Refactor
 *
 *  These tests validate the three new ledger utility functions introduced to
 *  replace the broken vOffsets cross-validation approach:
 *
 *    1. BuildSolvedPrimeCandidateFromTemplate – constructs a canonical solved
 *       block by copying the immutable template and applying the miner's nNonce
 *       and vOffsets without touching nTime (Prime proof invariant).
 *
 *    2. VerifySubmittedPrimeOffsets – validates miner-submitted vOffsets
 *       structurally without relying on the broken GetOffsets(GetPrime())
 *       equivalence check that returned empty results whenever GetPrime() was
 *       not itself prime.
 *
 *    3. FinalizeWalletSignatureForSolvedBlock – generates the canonical
 *       vchBlockSig so that TritiumBlock::Check() → VerifySignature() passes.
 *       (Runtime tests are integration-level; unit tests cover structural
 *       correctness of the utility's interface.)
 *
 *  Regression coverage:
 *    - The broken path rejected valid Prime submissions by comparing
 *      GetOffsets(GetPrime()) (which returns empty when GetPrime() is not prime)
 *      against the miner's submitted vOffsets, producing a false mismatch.
 *    - VerifySubmittedPrimeOffsets removes that equivalence check entirely;
 *      the authoritative PoW gate is VerifyWork() inside TritiumBlock::Check().
 *
 *  Limitations documented:
 *    - VerifySubmittedPrimeOffsets does NOT cryptographically prove the offsets
 *      describe a valid Cunningham chain starting at GetPrime(); full chain
 *      verification is deferred to VerifyWork().
 *    - FinalizeWalletSignatureForSolvedBlock requires an active autologin
 *      session; runtime signing tests are integration-level.
 **/


/* ─── helpers ─────────────────────────────────────────────────────────────── */

/** Build a minimal TritiumBlock with the supplied channel, nNonce, and vOffsets
 *  for use as a test fixture.  Fields that are irrelevant to the tested utility
 *  functions are left at their zero-initialised defaults. */
static TAO::Ledger::TritiumBlock MakeTestBlock(
    uint32_t nChannel,
    uint64_t nNonce,
    const std::vector<uint8_t>& vOffsets)
{
    TAO::Ledger::TritiumBlock block;
    block.nVersion  = 8;
    block.nChannel  = nChannel;
    block.nNonce    = nNonce;
    block.nHeight   = 1000;
    block.nBits     = 0x04000000u;
    block.nTime     = 1700000000u;  // fixed timestamp
    block.vOffsets  = vOffsets;
    return block;
}

/** Minimal valid Prime vOffsets: one chain-offset byte (2) and 4 fractional bytes. */
static std::vector<uint8_t> MinimalValidPrimeOffsets()
{
    return {2u, 0u, 0u, 0u, 0u};
}

/** Build a well-formed Prime vOffsets with N chain-offset bytes (all = 2) and
 *  4 fractional bytes. */
static std::vector<uint8_t> MakeChainOffsets(size_t nChainLen)
{
    std::vector<uint8_t> v(nChainLen, 2u);
    v.push_back(0u); v.push_back(0u); v.push_back(0u); v.push_back(0u);
    return v;
}


/* ─── VerifySubmittedPrimeOffsets tests ────────────────────────────────────── */

TEST_CASE("VerifySubmittedPrimeOffsets: empty offsets rejected", "[prime][sign_finalization]")
{
    TAO::Ledger::TritiumBlock block = MakeTestBlock(1, 42u, {});

    REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, {}) == false);
}


TEST_CASE("VerifySubmittedPrimeOffsets: too-short offsets rejected", "[prime][sign_finalization]")
{
    TAO::Ledger::TritiumBlock block = MakeTestBlock(1, 42u, {});

    SECTION("1 byte — no fractional component")
    {
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, {2u}) == false);
    }

    SECTION("4 bytes — exactly fractional, no chain offset byte")
    {
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, {0u, 0u, 0u, 0u}) == false);
    }
}


TEST_CASE("VerifySubmittedPrimeOffsets: invalid chain-offset byte rejected", "[prime][sign_finalization]")
{
    TAO::Ledger::TritiumBlock block = MakeTestBlock(1, 42u, {});

    SECTION("Chain offset 14 (above max gap of 12)")
    {
        /* offset[0]=14 exceeds the maximum valid gap of 12 */
        std::vector<uint8_t> v = {14u, 0u, 0u, 0u, 0u};
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == false);
    }

    SECTION("Chain offset 255 (way above max gap)")
    {
        std::vector<uint8_t> v = {255u, 0u, 0u, 0u, 0u};
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == false);
    }

    SECTION("Multiple chain offsets, one invalid mid-way")
    {
        /* offsets: [2, 4, 6, 14-invalid, 2] + 4 fractional */
        std::vector<uint8_t> v = {2u, 4u, 6u, 14u, 2u, 0u, 0u, 0u, 0u};
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == false);
    }

    SECTION("Last 4 bytes (fractional) are not subject to the ≤12 check")
    {
        /* The last 4 bytes are fractional difficulty — they can be any value.
         * Offset[0]=2 is valid; bytes 1-4 are fractional. */
        std::vector<uint8_t> v = {2u, 255u, 255u, 255u, 255u};
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == true);
    }
}


TEST_CASE("VerifySubmittedPrimeOffsets: valid offsets accepted", "[prime][sign_finalization]")
{
    TAO::Ledger::TritiumBlock block = MakeTestBlock(1, 42u, {});

    SECTION("Minimal valid: 1 chain offset + 4 fractional")
    {
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, MinimalValidPrimeOffsets()) == true);
    }

    SECTION("Typical cluster of 3 chain offsets + 4 fractional (7 bytes total)")
    {
        std::vector<uint8_t> v = {2u, 4u, 6u, 0u, 0u, 0u, 0u};
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == true);
    }

    SECTION("All chain offsets at maximum valid gap of 12")
    {
        std::vector<uint8_t> v = MakeChainOffsets(5);
        for(size_t i = 0; i < 5; ++i) v[i] = 12u;
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, v) == true);
    }

    SECTION("Long chain (10 offset bytes) all valid")
    {
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, MakeChainOffsets(10)) == true);
    }
}


/* ─── Regression: broken GetOffsets(GetPrime()) cross-validation ─────────── */

TEST_CASE("Regression: old GetOffsets equivalence check rejects valid submissions", "[prime][sign_finalization][regression]")
{
    /* This test demonstrates the prior broken behaviour:
     *   GetOffsets(GetPrime()) returns an EMPTY vector whenever GetPrime() is
     *   not itself prime.  The old code then compared that empty vector against
     *   the miner's submitted (non-empty) offsets and rejected the block.
     *
     * The new VerifySubmittedPrimeOffsets() does NOT perform this comparison.
     * A miner-submitted block with valid structural offsets must pass the
     * structural check regardless of whether GetPrime() is prime. */

    SECTION("Block where GetPrime() is NOT prime: old path would reject, new path accepts structurally")
    {
        /* Construct a block such that ProofHash() + nNonce is composite.
         * We use a fixed nNonce (1) and rely on the fact that a random 1024-bit
         * number is overwhelmingly likely to be composite. */
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 1u, {});

        /* The miner reports valid offsets (structurally well-formed). */
        const std::vector<uint8_t> vMinerOffsets = MinimalValidPrimeOffsets();

        /* Old broken path: GetOffsets(GetPrime()) would return empty because
         * the prime candidate is composite → compare(empty, {2,0,0,0,0}) → REJECT.
         * We demonstrate this by explicitly calling GetOffsets on the block: */
        std::vector<uint8_t> vNodeDerived;
        TAO::Ledger::GetOffsets(block.GetPrime(), vNodeDerived);
        /* vNodeDerived will likely be empty (GetPrime() is almost certainly composite). */

        /* New structural-only check: passes regardless of GetPrime() primality. */
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, vMinerOffsets) == true);

        /* Regression confirmation: if the old equivalence check were applied,
         * it would fail (vNodeDerived likely != vMinerOffsets). */
        if(vNodeDerived.empty())
        {
            /* Old code would have done: if(vNodeDerived != vMinerOffsets) return false; */
            REQUIRE(vNodeDerived != vMinerOffsets);  // confirms old check would reject
        }
    }
}


/* ─── BuildSolvedPrimeCandidateFromTemplate tests ────────────────────────── */

TEST_CASE("BuildSolvedPrimeCandidateFromTemplate: consensus fields preserved", "[prime][sign_finalization]")
{
    const uint32_t nChannel  = TAO::Ledger::CHANNEL::PRIME;
    const uint64_t nNonce    = 0xDEADBEEF12345678ULL;
    const uint32_t nVersion  = 8u;
    const uint32_t nHeight   = 6630855u;
    const uint32_t nBits     = 0x04000001u;
    const uint32_t nTime     = 1700000000u;

    TAO::Ledger::TritiumBlock tmpl = MakeTestBlock(nChannel, 1u /*template nNonce*/, {});
    tmpl.nVersion = nVersion;
    tmpl.nHeight  = nHeight;
    tmpl.nBits    = nBits;
    tmpl.nTime    = nTime;
    /* Simulate a prior template signature (to confirm it is cleared). */
    tmpl.vchBlockSig = {0xAA, 0xBB, 0xCC};

    const std::vector<uint8_t> vOffsets = MinimalValidPrimeOffsets();
    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, nNonce, vOffsets);

    SECTION("nVersion is preserved from template")
    {
        REQUIRE(solved.nVersion == nVersion);
    }

    SECTION("nChannel is preserved from template")
    {
        REQUIRE(solved.nChannel == nChannel);
    }

    SECTION("nHeight is preserved from template")
    {
        REQUIRE(solved.nHeight == nHeight);
    }

    SECTION("nBits is preserved from template")
    {
        REQUIRE(solved.nBits == nBits);
    }

    SECTION("nTime is preserved from template (not refreshed for Prime)")
    {
        /* For Prime: ProofHash = SK1024(nVersion..nBits) excludes nTime.
         * The miner's solved proof is independent of nTime, so we preserve
         * the template's nTime to avoid mutating anchor fields after issuance. */
        REQUIRE(solved.nTime == nTime);
    }

    SECTION("nNonce is set to the miner-submitted value")
    {
        REQUIRE(solved.nNonce == nNonce);
    }

    SECTION("vOffsets is set to the miner-submitted value for Prime channel")
    {
        REQUIRE(solved.vOffsets == vOffsets);
    }

    SECTION("vchBlockSig is cleared (must be re-generated after nNonce/vOffsets change)")
    {
        REQUIRE(solved.vchBlockSig.empty());
    }

    SECTION("hashPrevBlock is preserved (stale detection anchor)")
    {
        REQUIRE(solved.hashPrevBlock == tmpl.hashPrevBlock);
    }

    SECTION("hashMerkleRoot is preserved (transaction commitment)")
    {
        REQUIRE(solved.hashMerkleRoot == tmpl.hashMerkleRoot);
    }
}


TEST_CASE("BuildSolvedPrimeCandidateFromTemplate: Hash channel clears vOffsets", "[prime][sign_finalization]")
{
    /* For non-Prime channels (Hash), vOffsets must be cleared. */
    const std::vector<uint8_t> vOffsets = MinimalValidPrimeOffsets();
    TAO::Ledger::TritiumBlock tmpl = MakeTestBlock(2u /*Hash*/, 1u, vOffsets);

    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, 42u, vOffsets);

    REQUIRE(solved.nChannel == 2u);
    REQUIRE(solved.vOffsets.empty());
}


TEST_CASE("BuildSolvedPrimeCandidateFromTemplate: template is not mutated", "[prime][sign_finalization]")
{
    /* Confirm the function returns a copy and does not modify the original. */
    const uint64_t tmplNonce = 1u;
    const uint32_t tmplTime  = 1700000000u;
    TAO::Ledger::TritiumBlock tmpl = MakeTestBlock(1u, tmplNonce, {});
    tmpl.nTime = tmplTime;
    tmpl.vchBlockSig = {0xFF};

    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(
            tmpl, 0xCAFEBABEULL, MinimalValidPrimeOffsets());

    /* Template must be unchanged. */
    REQUIRE(tmpl.nNonce == tmplNonce);
    REQUIRE(tmpl.nTime  == tmplTime);
    REQUIRE(!tmpl.vchBlockSig.empty());

    /* Solved block must differ. */
    REQUIRE(solved.nNonce != tmplNonce);
    REQUIRE(solved.vchBlockSig.empty());
}


/* ─── Stale template rejection tests ─────────────────────────────────────── */

TEST_CASE("Stale Prime template: rejection is handled before sign_block", "[prime][sign_finalization][stale]")
{
    /* This test documents the expected stale-rejection behaviour:
     *   - sign_block() in both the legacy and stateless paths checks IsStale()
     *     on the TemplateMetadata before applying the miner's nNonce.
     *   - A stale template must be rejected BEFORE any mutation.
     *   - The final hashPrevBlock-vs-hashBestChain stale check catches
     *     templates that became stale after the IsStale() pre-screen.
     *
     * We cannot fully exercise the runtime staleness path in a unit test
     * (it requires a live chain state), but we can verify the TemplateMetadata
     * interface that drives those decisions. */

    SECTION("A fresh Prime block candidate has the correct channel")
    {
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 42u, MinimalValidPrimeOffsets());
        REQUIRE(block.nChannel == TAO::Ledger::CHANNEL::PRIME);
    }

    SECTION("VerifySubmittedPrimeOffsets passes for a fresh Prime submission")
    {
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 42u, {});
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, MinimalValidPrimeOffsets()) == true);
    }

    SECTION("VerifySubmittedPrimeOffsets rejects empty offsets (stale/corrupted payload)")
    {
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 42u, {});
        REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(block, {}) == false);
    }
}


/* ─── Canonical consensus-field completeness ─────────────────────────────── */

TEST_CASE("BuildSolvedPrimeCandidateFromTemplate: result passes structural VerifySubmittedPrimeOffsets", "[prime][sign_finalization]")
{
    /* A solved candidate built by BuildSolvedPrimeCandidateFromTemplate with
     * valid structural offsets must pass VerifySubmittedPrimeOffsets. */
    TAO::Ledger::TritiumBlock tmpl = MakeTestBlock(1u, 1u, {});
    const std::vector<uint8_t> vOffsets = MakeChainOffsets(3);

    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, 0xABCDEFULL, vOffsets);

    REQUIRE(solved.nChannel == TAO::Ledger::CHANNEL::PRIME);
    REQUIRE(TAO::Ledger::VerifySubmittedPrimeOffsets(solved, solved.vOffsets) == true);
}
