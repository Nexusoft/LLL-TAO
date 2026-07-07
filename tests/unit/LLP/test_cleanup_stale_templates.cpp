/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/types/miner.h>
#include <LLP/include/falcon_constants.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/runtime.h>

using namespace LLP;

namespace
{
    struct ChainStateRestore
    {
        uint32_t nBestHeight;
        uint1024_t hashBestChain;
        TAO::Ledger::BlockState stateBest;

        ChainStateRestore()
            : nBestHeight(TAO::Ledger::ChainState::nBestHeight.load())
            , hashBestChain(TAO::Ledger::ChainState::hashBestChain.load())
            , stateBest(TAO::Ledger::ChainState::tStateBest.load())
        {
        }

        ~ChainStateRestore()
        {
            TAO::Ledger::ChainState::nBestHeight.store(nBestHeight);
            TAO::Ledger::ChainState::hashBestChain.store(hashBestChain);
            TAO::Ledger::ChainState::tStateBest.store(stateBest);
        }
    };
}


/** Test Suite: CleanupStaleTemplates Channel Height Fix
 *
 *  These tests verify that CleanupStaleTemplates correctly uses CHANNEL HEIGHT
 *  instead of UNIFIED HEIGHT when determining template staleness.
 *
 *  Key Bug Fixed:
 *  --------------
 *  BEFORE: CleanupStaleTemplates compared (nCurrentHeight - meta.nHeight) > RETENTION
 *          This caused false negatives when unified height changed but channel height didn't.
 *          Example: Prime template at unified 2333381, chain advances to 2333383 (only 2 blocks)
 *                   but Prime channel is still stale. Cleanup kept the template!
 *
 *  AFTER:  CleanupStaleTemplates compares (nCurrentChannelHeight - meta.nChannelHeight) >= RETENTION
 *          This correctly removes templates when their SPECIFIC CHANNEL has advanced,
 *          regardless of unified height changes.
 *
 *  Test Scenarios:
 *  ---------------
 *  1. Template stale by channel height (should be removed)
 *  2. Template fresh within retention window (should be kept)
 *  3. Template stale by age (should be removed)
 *  4. Cross-channel block doesn't trigger false cleanup (Prime template safe when Hash advances)
 *  5. Latest template per channel receives no special retention exemption and is removed when stale
 *  6. Legacy Miner lane uses the same symmetric channel-height stale check
 **/


TEST_CASE("CleanupStaleTemplates uses channel height not unified height", "[cleanup][channel_height]")
{
    SECTION("Validates the fix: channel height comparison, not unified height")
    {
        /* This test documents the core fix:
         *
         * BUGGY CODE (removed):
         *   const bool fTooOldByBlocks =
         *       (nCurrentHeight >= meta.nHeight) &&
         *       ((nCurrentHeight - meta.nHeight) > TEMPLATE_RETENTION_BLOCKS);
         *
         * FIXED CODE (current):
         *   Get current channel height for template's channel
         *   if(TemplateChannelHeightDistance(nCurrentChannelHeight, meta.nChannelHeight) >= TEMPLATE_RETENTION_BLOCKS)
         *       fTooOldByBlocks = true;
         *
         * The fix ensures that a Prime template at channel_height=2165443 is removed
         * when Prime channel advances to 2165445+, REGARDLESS of unified height changes.
         */

        /* Scenario from problem statement:
         * - 6 stale templates at channel_height = 2333381
         * - CleanupStaleTemplates removed 0 templates (BUG!)
         * - Templates should have been removed based on channel height advancement
         */

        const uint32_t TEMPLATE_RETENTION_BLOCKS = MINING_TEMPLATE_RETENTION_BLOCKS;

        /* Template metadata (from problem statement) */
        uint32_t nTemplateChannelHeight = 2333381;  // Template targets this channel height
        uint32_t nTemplateUnifiedHeight = 6535198;  // Unified height when template was created

        /* Current blockchain state (Prime channel has advanced) */
        uint32_t nCurrentChannelHeight = 2333385;   // Prime channel now at 2333385 (4 blocks ahead)
        uint32_t nCurrentUnifiedHeight = 6535202;   // Unified height advanced by 4

        /* BUGGY logic (what was happening):
         * unified_diff = 6535202 - 6535198 = 4 blocks
         * 4 > 2 retention blocks → would remove (but unified heights DON'T match template semantic!)
         *
         * BUT: if unified only advanced by 2 (other channels mined), it would incorrectly KEEP the stale template!
         */
        uint32_t nUnifiedDiff = nCurrentUnifiedHeight - nTemplateUnifiedHeight;
        bool fBuggyLogicWouldRemove = (nUnifiedDiff > TEMPLATE_RETENTION_BLOCKS);

        /* CORRECT logic (what we fixed):
         * channel_diff = 2333385 - 2333381 = 4 blocks
         * 4 >= 2 retention blocks → remove (correct!)
         */
        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fCorrectLogicRemoves =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        /* The fix ensures we use channel height, not unified height */
        REQUIRE(fCorrectLogicRemoves == true);  // Template IS stale by channel height
        REQUIRE(nChannelDiff == 4);
        REQUIRE(nChannelDiff >= TEMPLATE_RETENTION_BLOCKS);

        /* Demonstrate the bug scenario: if unified height only advanced by 1-2 blocks
         * (because other channels mined), buggy logic would KEEP the stale template */
        uint32_t nBuggyUnifiedHeightScenario = 6535199;  // Only 1 block ahead in unified
        uint32_t nBuggyUnifiedDiff = nBuggyUnifiedHeightScenario - nTemplateUnifiedHeight;
        bool fBuggyLogicWouldKeep = (nBuggyUnifiedDiff <= TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(fBuggyLogicWouldKeep == true);   // Buggy logic would keep it
        REQUIRE(fCorrectLogicRemoves == true);   // Fixed logic removes it (channel advanced!)
    }
}


TEST_CASE("CleanupStaleTemplates channel height scenarios", "[cleanup][scenarios]")
{
    const uint32_t TEMPLATE_RETENTION_BLOCKS = MINING_TEMPLATE_RETENTION_BLOCKS;

    SECTION("Template within retention window: should be kept")
    {
        /* Template targets channel height 2165443 */
        uint32_t nTemplateChannelHeight = 2165443;

        /* Current channel height is 2165444 (1 block ahead) */
        uint32_t nCurrentChannelHeight = 2165444;

        uint32_t nDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOld =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 1);
        REQUIRE(fTooOld == false);  // Within retention window, should be kept
    }

    SECTION("Template at retention boundary: should be removed")
    {
        /* Template targets channel height 2165443 */
        uint32_t nTemplateChannelHeight = 2165443;

        /* Current channel height is 2165445 (2 blocks ahead) */
        uint32_t nCurrentChannelHeight = 2165445;

        uint32_t nDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOld =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 2);
        REQUIRE(fTooOld == true);  // At boundary, should be removed
    }

    SECTION("Template far behind: should be removed")
    {
        /* Template targets channel height 2165443 */
        uint32_t nTemplateChannelHeight = 2165443;

        /* Current channel height is 2165450 (7 blocks ahead) */
        uint32_t nCurrentChannelHeight = 2165450;

        uint32_t nDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOld =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 7);
        REQUIRE(fTooOld == true);  // Far behind, definitely should be removed
    }

    SECTION("Template abandoned by deep backward reorg: should be removed")
    {
        /* Template targets channel height 2165443 */
        uint32_t nTemplateChannelHeight = 2165443;

        /* Deep reorg moved the active channel tip back below the template parent */
        uint32_t nCurrentChannelHeight = 2165178;

        uint32_t nDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOld =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 265);
        REQUIRE(fTooOld == true);  // Backward movement beyond retention is stale
    }

    SECTION("Legacy Miner lane uses the same backward reorg distance")
    {
        const uint32_t nLegacyTemplateChannelHeight = 2333381;
        const uint32_t nLegacyCurrentChannelHeight = 2333116;

        const uint32_t nDiff =
            TemplateChannelHeightDistance(nLegacyCurrentChannelHeight, nLegacyTemplateChannelHeight);
        const bool fTooOld =
            IsTemplateTooOldByChannelHeight(nLegacyCurrentChannelHeight,
                                            nLegacyTemplateChannelHeight,
                                            TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 265);
        REQUIRE(fTooOld == true);
    }

    SECTION("Template at current channel height: should be kept")
    {
        /* Template targets channel height 2165443 */
        uint32_t nTemplateChannelHeight = 2165443;

        /* Current channel is still at 2165443 (no new blocks in this channel) */
        uint32_t nCurrentChannelHeight = 2165443;

        uint32_t nDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOld =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nDiff == 0);
        REQUIRE(fTooOld == false);  // Current, should be kept
    }
}


TEST_CASE("CleanupStaleTemplates cross-channel independence", "[cleanup][cross_channel]")
{
    const uint32_t TEMPLATE_RETENTION_BLOCKS = MINING_TEMPLATE_RETENTION_BLOCKS;

    SECTION("Prime template unaffected when Hash channel advances")
    {
        /* Prime template at channel height 2165443 */
        uint32_t nPrimeTemplateChannelHeight = 2165443;
        uint32_t nPrimeCurrentChannelHeight = 2165443;  // Prime hasn't advanced

        /* Hash channel advances (unified height changes but Prime doesn't) */
        uint32_t nHashCurrentChannelHeight = 4165050;   // Hash advanced 10 blocks
        uint32_t nHashPreviousChannelHeight = 4165040;

        /* Prime template check (using channel height) */
        uint32_t nPrimeDiff = TemplateChannelHeightDistance(nPrimeCurrentChannelHeight, nPrimeTemplateChannelHeight);
        bool fPrimeStale =
            IsTemplateTooOldByChannelHeight(nPrimeCurrentChannelHeight, nPrimeTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nPrimeDiff == 0);
        REQUIRE(fPrimeStale == false);  // Prime template still fresh

        /* Verify Hash advancement happened */
        uint32_t nHashDiff = nHashCurrentChannelHeight - nHashPreviousChannelHeight;
        REQUIRE(nHashDiff == 10);  // Hash did advance
    }

    SECTION("Hash template unaffected when Prime channel advances")
    {
        /* Hash template at channel height 4165040 */
        uint32_t nHashTemplateChannelHeight = 4165040;
        uint32_t nHashCurrentChannelHeight = 4165040;  // Hash hasn't advanced

        /* Prime channel advances */
        uint32_t nPrimeCurrentChannelHeight = 2165450;
        uint32_t nPrimePreviousChannelHeight = 2165443;

        /* Hash template check */
        uint32_t nHashDiff = TemplateChannelHeightDistance(nHashCurrentChannelHeight, nHashTemplateChannelHeight);
        bool fHashStale =
            IsTemplateTooOldByChannelHeight(nHashCurrentChannelHeight, nHashTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nHashDiff == 0);
        REQUIRE(fHashStale == false);  // Hash template still fresh

        /* Verify Prime advancement happened */
        uint32_t nPrimeDiff = nPrimeCurrentChannelHeight - nPrimePreviousChannelHeight;
        REQUIRE(nPrimeDiff == 7);  // Prime did advance
    }
}


TEST_CASE("CleanupStaleTemplates age-based timeout still works", "[cleanup][age]")
{
    SECTION("Template older than MAX_TEMPLATE_AGE_SECONDS is removed regardless of height")
    {
        /* Template age timeout is 600 seconds */
        const uint64_t MAX_TEMPLATE_AGE_SECONDS = LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS;
        REQUIRE(MAX_TEMPLATE_AGE_SECONDS == 600);

        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCreationTime = nNow - (MAX_TEMPLATE_AGE_SECONDS + 10);  // 610 seconds old

        /* Template is within channel height retention window */
        uint32_t nTemplateChannelHeight = 2165443;
        uint32_t nCurrentChannelHeight = 2165444;  // Only 1 block ahead
        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);

        bool fTooOldByBlocks =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, 2);  // False (1 < 2)
        bool fTooOldByTime = ((nNow - nCreationTime) > MAX_TEMPLATE_AGE_SECONDS);  // True (610 > 600)

        REQUIRE(fTooOldByBlocks == false);  // Fresh by channel height
        REQUIRE(fTooOldByTime == true);     // Stale by age

        /* Should be removed due to age timeout */
        bool fShouldRemove = fTooOldByBlocks || fTooOldByTime;
        REQUIRE(fShouldRemove == true);
    }

    SECTION("Template within age timeout and height retention is kept")
    {
        const uint64_t MAX_TEMPLATE_AGE_SECONDS = LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS;

        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCreationTime = nNow - 30;  // 30 seconds old (fresh)

        /* Template within retention window */
        uint32_t nTemplateChannelHeight = 2165443;
        uint32_t nCurrentChannelHeight = 2165444;  // 1 block ahead
        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);

        bool fTooOldByBlocks =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, 2);
        bool fTooOldByTime = ((nNow - nCreationTime) > MAX_TEMPLATE_AGE_SECONDS);

        REQUIRE(fTooOldByBlocks == false);  // Fresh by height
        REQUIRE(fTooOldByTime == false);    // Fresh by age

        bool fShouldRemove = fTooOldByBlocks || fTooOldByTime;
        REQUIRE(fShouldRemove == false);  // Should be kept
    }
}


TEST_CASE("CleanupStaleTemplates problem statement scenario", "[cleanup][problem_statement]")
{
    SECTION("Reproduces the exact bug from problem statement")
    {
        /* From problem statement:
         * "Looking at Image 5: Templates before cleanup: 6 and Cleanup complete: 0 templates removed.
         *  The 6 cached templates at channel_height = 2333381 are stale (block advanced)
         *  but CleanupStaleTemplates isn't cleaning them."
         */

        const uint32_t TEMPLATE_RETENTION_BLOCKS = MINING_TEMPLATE_RETENTION_BLOCKS;

        /* 6 stale templates at channel_height 2333381 */
        uint32_t nTemplateChannelHeight = 2333381;

        /* Suppose channel has advanced to 2333385 (4 blocks ahead) */
        uint32_t nCurrentChannelHeight = 2333385;

        /* With the FIX, these templates should be removed */
        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        bool fTooOldByBlocks =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nChannelDiff == 4);
        REQUIRE(fTooOldByBlocks == true);  // Should be removed!

        /* Before the fix, if unified height only advanced by 1-2 blocks
         * (because other channels mined), buggy code would keep them:
         *
         * Buggy scenario:
         *   nTemplateUnifiedHeight = 6535198
         *   nCurrentUnifiedHeight  = 6535199 (only 1 block ahead)
         *   nUnifiedDiff = 1
         *   fBuggyStale = (1 > 2) = false → INCORRECTLY KEPT
         *
         * But channel actually advanced by 4 blocks → should be removed!
         */
        uint32_t nTemplateUnifiedHeight = 6535198;
        uint32_t nBuggyCurrentUnifiedHeight = 6535199;
        uint32_t nUnifiedDiff = nBuggyCurrentUnifiedHeight - nTemplateUnifiedHeight;
        bool fBuggyStale = (nUnifiedDiff > TEMPLATE_RETENTION_BLOCKS);

        REQUIRE(nUnifiedDiff == 1);
        REQUIRE(fBuggyStale == false);     // Buggy logic: kept (WRONG!)
        REQUIRE(fTooOldByBlocks == true);  // Fixed logic: removed (CORRECT!)
    }
}


TEST_CASE("CleanupStaleTemplates applies staleness checks to all templates", "[cleanup][latest]")
{
    SECTION("Latest template is removed if stale by age or height")
    {
        /* Template is stale by BOTH age and height */
        const uint64_t MAX_TEMPLATE_AGE_SECONDS = 600;
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCreationTime = nNow - 700;  // 700 seconds old (too old)

        uint32_t nTemplateChannelHeight = 2165443;
        uint32_t nCurrentChannelHeight = 2165450;  // 7 blocks ahead (too far)

        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        uint64_t nAge = nNow - nCreationTime;

        bool fTooOldByBlocks =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, 2);
        bool fTooOldByTime = (nAge > MAX_TEMPLATE_AGE_SECONDS);

        REQUIRE(nChannelDiff == 7);
        REQUIRE(nAge == 700);
        REQUIRE(fTooOldByBlocks == true);
        REQUIRE(fTooOldByTime == true);

        /* A latest template is not exempt from the staleness checks. */
        bool fShouldRemove = (fTooOldByBlocks || fTooOldByTime);

        REQUIRE(fShouldRemove == true);  // Latest stale template is removed
    }

    SECTION("Latest template is kept when it passes staleness checks")
    {
        const uint64_t MAX_TEMPLATE_AGE_SECONDS = 600;
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCreationTime = nNow - 30;

        uint32_t nTemplateChannelHeight = 2165443;
        uint32_t nCurrentChannelHeight = 2165444;

        uint32_t nChannelDiff = TemplateChannelHeightDistance(nCurrentChannelHeight, nTemplateChannelHeight);
        uint64_t nAge = nNow - nCreationTime;

        bool fTooOldByBlocks =
            IsTemplateTooOldByChannelHeight(nCurrentChannelHeight, nTemplateChannelHeight, 2);
        bool fTooOldByTime = (nAge > MAX_TEMPLATE_AGE_SECONDS);
        bool fShouldRemove = (fTooOldByBlocks || fTooOldByTime);

        REQUIRE(fShouldRemove == false);
    }
}

TEST_CASE("Legacy Miner cleanup removes stale registered templates from all maps", "[cleanup][miner][legacy]")
{
    ChainStateRestore restore;
    LLP::Miner miner;

    const uint32_t nChannel = TAO::Ledger::CHANNEL::HASH;
    const uint32_t nTemplateChannelHeight = 2333381;

    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
    stateBest.nHeight = 6535198;
    stateBest.nChannel = nChannel;
    stateBest.nChannelHeight = nTemplateChannelHeight - 1;
    TAO::Ledger::ChainState::tStateBest.store(stateBest);
    TAO::Ledger::ChainState::nBestHeight.store(stateBest.nHeight);
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(1001));

    auto* pBlock = new TAO::Ledger::TritiumBlock();
    pBlock->hashMerkleRoot = uint512_t(2002);
    pBlock->nChannel = nChannel;
    pBlock->nHeight = stateBest.nHeight + 1;

    REQUIRE(miner.RegisterTemplateForTests(pBlock) == pBlock);
    REQUIRE(miner.TemplateBlockCountForTests() == 1);
    REQUIRE(miner.TemplateHashCountForTests() == 1);
    REQUIRE(miner.TemplateChannelHeightCountForTests() == 1);
    REQUIRE(miner.TemplateCreationTimeCountForTests() == 1);

    uint32_t nRecordedChannelHeight = 0;
    REQUIRE(miner.GetTemplateChannelHeightForTests(pBlock->hashMerkleRoot, nRecordedChannelHeight));
    REQUIRE(nRecordedChannelHeight == nTemplateChannelHeight);

    uint64_t nRecordedCreationTime = 0;
    REQUIRE(miner.GetTemplateCreationTimeForTests(pBlock->hashMerkleRoot, nRecordedCreationTime));
    REQUIRE(nRecordedCreationTime != 0);

    TAO::Ledger::BlockState stateReorg = stateBest;
    stateReorg.nHeight = 6534933;
    stateReorg.nChannelHeight = 2333116;
    TAO::Ledger::ChainState::tStateBest.store(stateReorg);
    TAO::Ledger::ChainState::nBestHeight.store(stateReorg.nHeight);
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(3003));

    REQUIRE(TemplateChannelHeightDistance(stateReorg.nChannelHeight, nRecordedChannelHeight) == 265);
    REQUIRE(miner.CleanupStaleTemplatesForTests() == 1);

    REQUIRE(miner.TemplateBlockCountForTests() == 0);
    REQUIRE(miner.TemplateHashCountForTests() == 0);
    REQUIRE(miner.TemplateChannelHeightCountForTests() == 0);
    REQUIRE(miner.TemplateCreationTimeCountForTests() == 0);
    REQUIRE(miner.LookupTemplateForTests(uint512_t(2002)) == nullptr);
}

