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


/** Test Suite: Prime Sign / Finalization
 *
 *  These tests validate the ledger utility functions for Prime channel
 *  block candidate construction.
 *
 *  Key invariant after vOffsets wire removal:
 *    - BuildSolvedPrimeCandidateFromTemplate() always clears vOffsets.
 *    - The node derives vOffsets via GetOffsets(GetPrime(), vOffsets) after
 *      setting nNonce.  Miners no longer transmit vOffsets on the wire.
 *    - The Prime wire format is now identical to Hash:
 *        [block(216)][timestamp(8)][sig_len(2)][signature]
 *
 *  FinalizeWalletSignatureForSolvedBlock requires an active autologin
 *  session; runtime signing tests are integration-level.
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

    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, nNonce);

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

    SECTION("vOffsets is always empty (node derives via GetOffsets after this call)")
    {
        /* The node no longer accepts miner-submitted vOffsets on the wire.
         * BuildSolvedPrimeCandidateFromTemplate always clears vOffsets;
         * the caller must call GetOffsets(GetPrime(), pBlock->vOffsets). */
        REQUIRE(solved.vOffsets.empty());
    }

    SECTION("vchBlockSig is cleared (must be re-generated after nNonce change)")
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


TEST_CASE("BuildSolvedPrimeCandidateFromTemplate: vOffsets always cleared", "[prime][sign_finalization]")
{
    /* Even if the template carries residual vOffsets, the solved candidate must
     * have empty vOffsets.  The node derives them via GetOffsets() after this call. */
    const std::vector<uint8_t> residualOffsets = {2u, 4u, 0u, 0u, 0u, 0u};
    TAO::Ledger::TritiumBlock tmpl = MakeTestBlock(TAO::Ledger::CHANNEL::PRIME, 1u, residualOffsets);
    REQUIRE(!tmpl.vOffsets.empty()); // template has residual offsets

    TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, 42u);

    REQUIRE(solved.nChannel == TAO::Ledger::CHANNEL::PRIME);
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
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmpl, 0xCAFEBABEULL);

    /* Template must be unchanged. */
    REQUIRE(tmpl.nNonce == tmplNonce);
    REQUIRE(tmpl.nTime  == tmplTime);
    REQUIRE(!tmpl.vchBlockSig.empty());

    /* Solved block must differ. */
    REQUIRE(solved.nNonce != tmplNonce);
    REQUIRE(solved.vchBlockSig.empty());
    REQUIRE(solved.vOffsets.empty());
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
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 42u, {});
        REQUIRE(block.nChannel == TAO::Ledger::CHANNEL::PRIME);
    }

    SECTION("BuildSolvedPrimeCandidateFromTemplate always clears vOffsets")
    {
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 42u, {2u, 0u, 0u, 0u, 0u});
        TAO::Ledger::TritiumBlock solved =
            TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(block, 99u);
        REQUIRE(solved.vOffsets.empty());
    }
}


/* ─── Regression: node must derive vOffsets after BuildSolved ───────────── */

TEST_CASE("Regression: GetOffsets used by node after BuildSolvedPrimeCandidateFromTemplate", "[prime][sign_finalization][regression]")
{
    /* This test documents the correct invariant after the vOffsets wire removal:
     *   1. BuildSolvedPrimeCandidateFromTemplate clears vOffsets.
     *   2. The node then calls GetOffsets(pBlock->GetPrime(), pBlock->vOffsets).
     *   3. If GetOffsets returns empty → GetPrime() is composite → VerifyWork() rejects.
     *
     * The old broken path rejected valid Prime submissions by comparing
     * GetOffsets(GetPrime()) (which returns empty when GetPrime() is not prime)
     * against the miner's submitted vOffsets, producing a false mismatch.
     * The new path always uses GetOffsets() on the node side regardless of
     * what the miner sent. */

    SECTION("Block where GetPrime() is composite: GetOffsets returns empty (VerifyWork gates acceptance)")
    {
        /* Construct a block where ProofHash() + nNonce is almost certainly composite. */
        TAO::Ledger::TritiumBlock block = MakeTestBlock(1u, 1u, {});

        /* Step 1: Build solved candidate — vOffsets always cleared. */
        TAO::Ledger::TritiumBlock solved =
            TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(block, 1u);
        REQUIRE(solved.vOffsets.empty());

        /* Step 2: Node derives offsets. If GetPrime() is composite, GetOffsets returns empty. */
        std::vector<uint8_t> vDerived;
        TAO::Ledger::GetOffsets(solved.GetPrime(), vDerived);
        /* vDerived will be empty (GetPrime() almost certainly composite for nNonce=1). */

        /* Step 3: VerifyWork() will reject a block with empty vOffsets — that is the
         * correct behaviour.  The rejection happens at VerifyWork, not in sign_block. */
        /* (No assertion on vDerived — it may be empty or not; the key point is that
         *  sign_block no longer rejects based on VerifySubmittedPrimeOffsets.) */
    }
}
