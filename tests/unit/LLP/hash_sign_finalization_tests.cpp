/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/types/tritium.h>

#include <cstdint>
#include <vector>


/** Test Suite: Hash (channel 2) Sign / Finalization
 *
 *  These tests validate the Hash-channel solved-candidate utility functions that
 *  mirror the Prime-focused utilities introduced in PR #392.
 *
 *  Specifically tested:
 *
 *    1. BuildSolvedHashCandidateFromTemplate – constructs a canonical solved
 *       Hash block by copying the immutable template and applying the miner's
 *       nNonce without touching nTime and clearing vOffsets (Hash invariant).
 *
 *    2. FinalizeWalletSignatureForSolvedBlock (interface) – structural coverage
 *       for the shared signing utility on the Hash channel.  Full signing tests
 *       are integration-level (require an active autologin session).
 *
 *  Regression coverage:
 *    - Hash channel must never carry vOffsets; any data in vOffsets from a prior
 *      channel switch or serialisation artefact must be cleared by the helper.
 *    - The original template must NOT be mutated (solved candidate is a copy).
 *    - All consensus-critical fields (nVersion, hashPrevBlock, hashMerkleRoot,
 *      nChannel, nHeight, nBits, nTime) must be preserved from the template.
 *    - vchBlockSig must be cleared so FinalizeWalletSignatureForSolvedBlock()
 *      can re-sign the solved candidate.
 *
 *  Architecture note (PR #392 + this PR):
 *    - PR #392 introduced BuildSolvedPrimeCandidateFromTemplate and
 *      FinalizeWalletSignatureForSolvedBlock for Prime / channel 1.
 *    - This PR adds BuildSolvedHashCandidateFromTemplate for Hash / channel 2,
 *      completing the symmetric solved-candidate flow across all mining channels.
 *    - Both helpers use FinalizeWalletSignatureForSolvedBlock as the shared
 *      canonical signing step, ensuring consistent behaviour.
 **/


/* ─── helpers ─────────────────────────────────────────────────────────────── */

/** Build a minimal TritiumBlock with the supplied channel, nNonce, and vOffsets
 *  for use as a test fixture.  Fields irrelevant to the tested utility
 *  functions are left at their zero-initialised defaults. */
static TAO::Ledger::TritiumBlock MakeHashTestBlock(
    uint32_t nChannel,
    uint64_t nNonce,
    const std::vector<uint8_t>& vOffsets)
{
    TAO::Ledger::TritiumBlock block;
    block.nVersion  = 8;
    block.nChannel  = nChannel;
    block.nNonce    = nNonce;
    block.nHeight   = 2000;
    block.nBits     = 0x04000000u;
    block.nTime     = 1700001000u;  // fixed timestamp
    block.vOffsets  = vOffsets;
    return block;
}

/** A minimal non-empty Prime vOffsets that is structurally valid (1 chain offset + 4 fractional). */
static std::vector<uint8_t> SomePrimeOffsets()
{
    return {2u, 0u, 0u, 0u, 0u};
}


/* ─── BuildSolvedHashCandidateFromTemplate: consensus fields preserved ──── */

TEST_CASE("BuildSolvedHashCandidateFromTemplate: consensus fields preserved", "[hash][sign_finalization]")
{
    const uint32_t nChannel  = 2u;  // Hash channel
    const uint64_t nNonce    = 0xFEEDFACE12345678ULL;
    const uint32_t nVersion  = 8u;
    const uint32_t nHeight   = 7000000u;
    const uint32_t nBits     = 0x04000002u;
    const uint32_t nTime     = 1700001000u;

    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(nChannel, 1u /*template nNonce*/, {});
    tmpl.nVersion = nVersion;
    tmpl.nHeight  = nHeight;
    tmpl.nBits    = nBits;
    tmpl.nTime    = nTime;
    /* Simulate a prior template signature (should be cleared by the helper). */
    tmpl.vchBlockSig = {0x11, 0x22, 0x33};

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, nNonce);

    SECTION("nVersion is preserved from template")
    {
        REQUIRE(solved.nVersion == nVersion);
    }

    SECTION("nChannel is preserved from template (must be 2)")
    {
        REQUIRE(solved.nChannel == 2u);
    }

    SECTION("nHeight is preserved from template")
    {
        REQUIRE(solved.nHeight == nHeight);
    }

    SECTION("nBits is preserved from template")
    {
        REQUIRE(solved.nBits == nBits);
    }

    SECTION("nTime is preserved from template (not refreshed for Hash)")
    {
        /* For Hash: ProofHash = SK1024(nVersion..nNonce) excludes nTime.
         * The miner's solved proof is independent of nTime, so we preserve
         * the template's nTime to avoid mutating anchor fields after issuance. */
        REQUIRE(solved.nTime == nTime);
    }

    SECTION("nNonce is set to the miner-submitted value")
    {
        REQUIRE(solved.nNonce == nNonce);
    }

    SECTION("vOffsets is empty (Hash channel invariant)")
    {
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


/* ─── Hash channel vOffsets invariant ───────────────────────────────────── */

TEST_CASE("BuildSolvedHashCandidateFromTemplate: vOffsets cleared regardless of template content",
          "[hash][sign_finalization]")
{
    /* Even if the template carries residual Prime offsets (e.g., channel switch
     * or serialisation artefact), the Hash solved candidate must have empty vOffsets. */
    const std::vector<uint8_t> residualOffsets = SomePrimeOffsets();
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, 1u, residualOffsets);
    REQUIRE(!tmpl.vOffsets.empty());  // template has offsets

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 42u);

    REQUIRE(solved.vOffsets.empty());
}


TEST_CASE("BuildSolvedHashCandidateFromTemplate: vOffsets cleared even for large offset payloads",
          "[hash][sign_finalization]")
{
    /* Large residual offset payload — should still be cleared. */
    std::vector<uint8_t> largeOffsets(20, 2u);
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, 1u, largeOffsets);
    REQUIRE(tmpl.vOffsets.size() == 20u);

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 99u);

    REQUIRE(solved.vOffsets.empty());
}


/* ─── Template immutability ─────────────────────────────────────────────── */

TEST_CASE("BuildSolvedHashCandidateFromTemplate: template is not mutated", "[hash][sign_finalization]")
{
    /* Confirm the function returns a copy and does not modify the original template. */
    const uint64_t tmplNonce = 1u;
    const uint32_t tmplTime  = 1700001000u;
    const std::vector<uint8_t> tmplOffsets = {0xAA, 0xBB};

    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, tmplNonce, tmplOffsets);
    tmpl.nTime = tmplTime;
    tmpl.vchBlockSig = {0xDE, 0xAD};

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 0xCAFEBABEULL);

    /* Template must be unchanged. */
    REQUIRE(tmpl.nNonce == tmplNonce);
    REQUIRE(tmpl.nTime  == tmplTime);
    REQUIRE(tmpl.vOffsets == tmplOffsets);
    REQUIRE(!tmpl.vchBlockSig.empty());

    /* Solved block must differ in the expected fields. */
    REQUIRE(solved.nNonce != tmplNonce);
    REQUIRE(solved.vOffsets.empty());
    REQUIRE(solved.vchBlockSig.empty());
}


/* ─── nNonce application ─────────────────────────────────────────────────── */

TEST_CASE("BuildSolvedHashCandidateFromTemplate: nNonce is applied correctly",
          "[hash][sign_finalization]")
{
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, 1u, {});

    SECTION("Typical miner nonce is applied")
    {
        const uint64_t nMinerNonce = 0x123456789ABCDEF0ULL;
        const TAO::Ledger::TritiumBlock solved =
            TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, nMinerNonce);
        REQUIRE(solved.nNonce == nMinerNonce);
    }

    SECTION("Zero nonce is applied (edge case)")
    {
        const TAO::Ledger::TritiumBlock solved =
            TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 0u);
        REQUIRE(solved.nNonce == 0u);
    }

    SECTION("Maximum nonce value is applied (edge case)")
    {
        const uint64_t nMaxNonce = std::numeric_limits<uint64_t>::max();
        const TAO::Ledger::TritiumBlock solved =
            TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, nMaxNonce);
        REQUIRE(solved.nNonce == nMaxNonce);
    }
}


/* ─── Symmetric channel comparison ──────────────────────────────────────── */

TEST_CASE("Hash and Prime solved candidates share common invariants", "[hash][sign_finalization]")
{
    /* BuildSolvedHashCandidateFromTemplate and BuildSolvedPrimeCandidateFromTemplate
     * are symmetric in that both:
     *   - Copy all consensus-critical fields from the template.
     *   - Apply the miner's nNonce.
     *   - Clear vchBlockSig.
     *   - Do NOT mutate the original template.
     * They differ in vOffsets handling:
     *   - Hash: always clears vOffsets.
     *   - Prime: copies the supplied vOffsets. */

    const std::vector<uint8_t> vOffsets = {2u, 4u, 0u, 0u, 0u};  // structurally valid Prime offsets
    const uint64_t nNonce = 0x1122334455667788ULL;

    TAO::Ledger::TritiumBlock tmplHash  = MakeHashTestBlock(2u, 1u, vOffsets);
    TAO::Ledger::TritiumBlock tmplPrime = MakeHashTestBlock(1u, 1u, {});
    /* Simulate prior signatures. */
    tmplHash.vchBlockSig  = {0xFF};
    tmplPrime.vchBlockSig = {0xFF};

    const TAO::Ledger::TritiumBlock solvedHash =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmplHash, nNonce);

    const TAO::Ledger::TritiumBlock solvedPrime =
        TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(tmplPrime, nNonce, vOffsets);

    SECTION("Both helpers produce a solved nNonce equal to the supplied nonce")
    {
        REQUIRE(solvedHash.nNonce  == nNonce);
        REQUIRE(solvedPrime.nNonce == nNonce);
    }

    SECTION("Both helpers clear vchBlockSig")
    {
        REQUIRE(solvedHash.vchBlockSig.empty());
        REQUIRE(solvedPrime.vchBlockSig.empty());
    }

    SECTION("Hash candidate always has empty vOffsets")
    {
        REQUIRE(solvedHash.vOffsets.empty());
    }

    SECTION("Prime candidate has the supplied vOffsets applied")
    {
        REQUIRE(solvedPrime.vOffsets == vOffsets);
    }

    SECTION("Both helpers preserve nTime from the template")
    {
        REQUIRE(solvedHash.nTime  == tmplHash.nTime);
        REQUIRE(solvedPrime.nTime == tmplPrime.nTime);
    }
}


/* ─── FinalizeWalletSignatureForSolvedBlock interface (Hash channel) ───── */

TEST_CASE("FinalizeWalletSignatureForSolvedBlock: Hash solved candidate has empty vchBlockSig before signing",
          "[hash][sign_finalization]")
{
    /* Confirm that a solved candidate produced by BuildSolvedHashCandidateFromTemplate
     * has an empty vchBlockSig, confirming that FinalizeWalletSignatureForSolvedBlock
     * must be called to produce a valid signature before ValidateMinedBlock(). */
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, 1u, {});
    tmpl.vchBlockSig = {0xAB, 0xCD};  // simulate template signature

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 0xDEADBEEFULL);

    /* vchBlockSig must be cleared before the caller invokes FinalizeWalletSignatureForSolvedBlock. */
    REQUIRE(solved.vchBlockSig.empty());

    /* Note: FinalizeWalletSignatureForSolvedBlock() requires an active autologin
     * session and is therefore not directly invoked in this unit test.
     * The full signing + Check() → VerifySignature() path is covered by
     * integration tests that run against a live node with -autologin configured. */
}


/* ─── Stale template regression ─────────────────────────────────────────── */

TEST_CASE("Hash channel solved candidate preserves hashPrevBlock for stale detection",
          "[hash][sign_finalization][stale]")
{
    /* hashPrevBlock is the primary staleness anchor baked into the template.
     * BuildSolvedHashCandidateFromTemplate must NOT modify this field. */
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u, 1u, {});
    /* Simulate a non-zero hashPrevBlock (the real anchor would be a chain hash). */
    tmpl.hashPrevBlock.SetHex(
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, 0xFEEDULL);

    REQUIRE(solved.hashPrevBlock == tmpl.hashPrevBlock);
}


/* ─── Integration-style: submit → canonical block → Check() assumptions ─ */

TEST_CASE("Hash submit flow: BuildSolvedHashCandidateFromTemplate produces a structurally valid solved block",
          "[hash][sign_finalization][integration]")
{
    /* This test simulates the node-side Hash submit flow at the structural level:
     *
     *   1. Start from an immutable wallet-signed template (simulated as a TritiumBlock
     *      with a prior vchBlockSig and fixed consensus fields).
     *   2. Call BuildSolvedHashCandidateFromTemplate with the miner's nNonce.
     *   3. Verify the solved candidate satisfies all structural invariants required
     *      before FinalizeWalletSignatureForSolvedBlock() is called.
     *
     * The full Check() / ValidateMinedBlock() / AcceptMinedBlock() path is
     * integration-level and requires a live chain; this test covers the
     * pre-signing structural guarantees only. */

    /* Step 1: Simulate a wallet-signed template. */
    TAO::Ledger::TritiumBlock tmpl = MakeHashTestBlock(2u /*Hash channel*/, 1u /*template nNonce*/, {});
    tmpl.nHeight   = 8000000u;
    tmpl.nBits     = 0x04000003u;
    tmpl.vchBlockSig = {0xDE, 0xAD, 0xBE, 0xEF};  // prior template signature

    /* Step 2: Apply miner's solved nonce. */
    const uint64_t nMinerNonce = 0xABCDEF1234567890ULL;
    const TAO::Ledger::TritiumBlock solved =
        TAO::Ledger::BuildSolvedHashCandidateFromTemplate(tmpl, nMinerNonce);

    /* Step 3: Verify structural invariants before signing. */

    /* Channel must remain Hash (2). */
    REQUIRE(solved.nChannel == 2u);

    /* The miner's nonce must be applied. */
    REQUIRE(solved.nNonce == nMinerNonce);

    /* vOffsets must be empty (Hash channel invariant). */
    REQUIRE(solved.vOffsets.empty());

    /* vchBlockSig must be cleared (FinalizeWalletSignatureForSolvedBlock must re-sign). */
    REQUIRE(solved.vchBlockSig.empty());

    /* Consensus-critical fields from the template must be unchanged. */
    REQUIRE(solved.nVersion        == tmpl.nVersion);
    REQUIRE(solved.nHeight         == tmpl.nHeight);
    REQUIRE(solved.nBits           == tmpl.nBits);
    REQUIRE(solved.nTime           == tmpl.nTime);
    REQUIRE(solved.hashPrevBlock   == tmpl.hashPrevBlock);
    REQUIRE(solved.hashMerkleRoot  == tmpl.hashMerkleRoot);

    /* Original template must be unchanged. */
    REQUIRE(tmpl.nNonce == 1u);
    REQUIRE(!tmpl.vchBlockSig.empty());
}
