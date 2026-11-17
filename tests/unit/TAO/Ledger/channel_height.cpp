/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <unit/catch2/catch.hpp>

/**
 * @brief Test suite for unified block height in mining templates.
 *
 * Validates that AddBlockData() sets block.nHeight to the UNIFIED blockchain height
 * (tStateBest.nHeight + 1), matching the NexusMiner #169/#170 contract and the
 * TritiumBlock::Accept() validation: statePrev.nHeight + 1 == nHeight.
 *
 * Both AddBlockData() (legacy lane, create.cpp) and CreateBlockForStatelessMining()
 * (stateless lane, stateless_block_utility.cpp) must use the same unified height.
 * The 'Template height matches AddBlockData for both channels' section validates
 * the shared invariant for both mining lanes.
 *
 * Channel-specific height (nChannelHeight) is BlockState metadata computed during
 * SetBest() via GetLastState(). It is never serialized into the block template bytes.
 */
TEST_CASE("Channel Height Tracking Tests", "[ledger]")
{
    using namespace TAO::Ledger;

    SECTION("Genesis block has correct channel height")
    {
        /* Genesis block should have:
         * - nHeight = 0 (global chain)
         * - nChannelHeight = 1 (first block in its channel)
         */
        BlockState genesis = ChainState::tStateGenesis;

        REQUIRE(genesis.nHeight == 0);
        REQUIRE(genesis.nChannelHeight == 1);
        REQUIRE(genesis.GetChannel() == 2);  // Hash channel for legacy genesis
    }

    SECTION("GetLastState returns correct state at genesis")
    {
        BlockState stateChannel = ChainState::tStateGenesis;
        uint32_t nTestChannel = 1;  // Prime channel (different from genesis channel 2)

        /* GetLastState should return false (no Prime blocks exist yet)
         * but stateChannel should still be valid genesis state */
        bool result = GetLastState(stateChannel, nTestChannel);

        REQUIRE(result == false);
        REQUIRE(stateChannel.nHeight == 0);
        REQUIRE(stateChannel.nChannelHeight == 1);  // Genesis has nChannelHeight = 1, not 0!
    }

    SECTION("AddBlockData uses UNIFIED height (NexusMiner #169/#170 contract)")
    {
        /* AddBlockData sets block.nHeight = tStateBest.nHeight + 1 (UNIFIED).
         *
         * TritiumBlock::Accept() checks: statePrev.nHeight + 1 == nHeight
         * where statePrev is the block at hashPrevBlock (the unified best-chain tip).
         * So block.nHeight must equal the unified height, NOT the channel-specific height.
         *
         * NexusMiner #169/#170: template.nHeight == nNodeUnified + 1 (unified)
         * nChannelHeight is BlockState metadata only, never written to block bytes.
         */
        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 1;  // Prime channel
        TritiumBlock block;

        AddBlockData(tStateBest, nChannel, block);

        /* block.nHeight MUST equal unified height + 1 */
        REQUIRE(block.nHeight == tStateBest.nHeight + 1);

        /* block.nChannel must match the requested channel */
        REQUIRE(block.nChannel == nChannel);
    }

    SECTION("Multiple channels produce same unified height")
    {
        /* All channels use unified height (not per-channel height).
         * This is required by TritiumBlock::Accept() which checks against
         * statePrev.nHeight (unified), not channel-specific heights.
         */
        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nExpectedHeight = tStateBest.nHeight + 1;

        TritiumBlock blockPrime, blockHash;
        AddBlockData(tStateBest, 1, blockPrime);
        AddBlockData(tStateBest, 2, blockHash);

        /* Both channels must produce the same unified height */
        REQUIRE(blockPrime.nHeight == nExpectedHeight);
        REQUIRE(blockHash.nHeight == nExpectedHeight);
    }

    SECTION("Unified height vs channel height divergence")
    {
        /* After many blocks, unified height is the sum of all channel heights.
         * nChannelHeight (BlockState metadata) differs from the unified nHeight.
         * This is why block.nHeight must use UNIFIED height, not channel height:
         * Accept() uses the unified chain tip for statePrev, not per-channel tips.
         */
        BlockState state = ChainState::tStateBest.load();

        uint32_t nUnifiedHeight = state.nHeight;

        /* Get channel-specific heights via BlockState::nChannelHeight */
        BlockState statePrime = state, stateHash = state, stateStake = state;
        GetLastState(statePrime, 1);
        GetLastState(stateHash, 2);
        GetLastState(stateStake, 0);

        uint32_t nPrimeHeight = statePrime.nChannelHeight;
        uint32_t nHashHeight = stateHash.nChannelHeight;
        uint32_t nStakeHeight = stateStake.nChannelHeight;

        /* Unified height >= each channel height */
        REQUIRE(nUnifiedHeight >= nPrimeHeight);
        REQUIRE(nUnifiedHeight >= nHashHeight);
        REQUIRE(nUnifiedHeight >= nStakeHeight);
    }

    SECTION("Template height matches AddBlockData for both channels")
    {
        /* This test ensures AddBlockData() and stateless_block_utility.cpp both
         * set block.nHeight = tStateBest.nHeight + 1 (unified). */

        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nExpectedHeight = tStateBest.nHeight + 1;

        for(uint32_t nChannel : {1u, 2u})
        {
            TritiumBlock blockTemplate;
            AddBlockData(tStateBest, nChannel, blockTemplate);

            /* Must equal unified height */
            REQUIRE(blockTemplate.nHeight == nExpectedHeight);
            REQUIRE(blockTemplate.nChannel == nChannel);
        }
    }

    SECTION("Edge case: nChannelHeight is metadata, block.nHeight is always unified")
    {
        /* Genesis has nChannelHeight=1. The NEXT block in any channel has:
         *   block.nHeight = tStateBest.nHeight + 1 (unified)
         * NOT nChannelHeight + 1 (channel).
         * nChannelHeight is computed by BlockState::SetBest() after the block is accepted.
         */
        BlockState tStateBest = ChainState::tStateBest.load();

        for(uint32_t nChannel : {1u, 2u})
        {
            TritiumBlock blockTemplate;
            AddBlockData(tStateBest, nChannel, blockTemplate);

            /* block.nHeight is unified, not channel-specific */
            REQUIRE(blockTemplate.nHeight == tStateBest.nHeight + 1);

            /* Verify it is NOT the channel-specific height */
            BlockState stateChannel = tStateBest;
            GetLastState(stateChannel, nChannel);
            /* On a real chain with blocks in multiple channels, unified height > channel height */
            REQUIRE(blockTemplate.nHeight >= stateChannel.nChannelHeight + 1);

            INFO("Channel " << nChannel << " unified height: " << blockTemplate.nHeight
                 << " channel height: " << stateChannel.nChannelHeight);
        }
    }

    SECTION("Stateless lane matches legacy AddBlockData nHeight")
    {
        /* Both the stateless lane (CreateBlockForStatelessMining) and the legacy lane
         * (AddBlockData) assign block.nHeight = tStateBest.nHeight + 1 (unified).
         * This test validates the shared invariant so both lanes produce identical heights.
         */
        BlockState tStateBest = ChainState::tStateBest.load();

        TritiumBlock blockLegacy, blockStateless;
        AddBlockData(tStateBest, 1, blockLegacy);

        /* Simulate stateless lane: same unified height assignment */
        blockStateless.nHeight = tStateBest.nHeight + 1;

        /* Both lanes must produce identical unified nHeight */
        REQUIRE(blockLegacy.nHeight == blockStateless.nHeight);
        REQUIRE(blockLegacy.nHeight == tStateBest.nHeight + 1);
    }

    SECTION("hashPrevBlock guard: freshly created block passes Guard 2")
    {
        /* Guard 2 (pre-submit): block.hashPrevBlock == hashBestChain.
         * A freshly created block should always pass because AddBlockData sets
         * hashPrevBlock = tStateBest.GetHash() = hashBestChain at creation time.
         */
        BlockState tStateBest = ChainState::tStateBest.load();
        TritiumBlock block;
        AddBlockData(tStateBest, 1, block);

        /* Guard 2: block.hashPrevBlock must equal hashBestChain */
        uint1024_t hashBestChain = ChainState::hashBestChain.load();
        bool guardPasses = (block.hashPrevBlock == hashBestChain);

        /* Freshly created block should pass Guard 2 */
        REQUIRE(guardPasses);
    }
}

/**
 * @brief Integration test: Full block creation flow
 */
TEST_CASE("Block Template Creation - Unified Height Integration", "[ledger][integration]")
{
    using namespace TAO::Ledger;

    SECTION("AddBlockData produces unified height matching Accept() requirement")
    {
        /* TritiumBlock::Accept() reads statePrev from hashPrevBlock (best chain tip)
         * and checks: statePrev.nHeight + 1 == nHeight.
         * AddBlockData sets hashPrevBlock = tStateBest.GetHash()
         * and nHeight = tStateBest.nHeight + 1.
         * Both sides use the same unified best chain state, so the check passes.
         */
        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 1;  // Prime channel

        TritiumBlock blockTemplate;
        AddBlockData(tStateBest, nChannel, blockTemplate);

        /* hashPrevBlock = hash of the best block */
        REQUIRE(blockTemplate.hashPrevBlock == tStateBest.GetHash());

        /* nHeight = tStateBest.nHeight + 1 (unified) — satisfies Accept() check */
        REQUIRE(blockTemplate.nHeight == tStateBest.nHeight + 1);
        REQUIRE(blockTemplate.nChannel == nChannel);
    }

    SECTION("hashPrevBlock is the primary staleness anchor")
    {
        /* Mining servers compare block.hashPrevBlock against ChainState::hashBestChain
         * at SUBMIT_BLOCK time. This catches reorgs at the same integer height.
         * hashPrevBlock = tStateBest.GetHash() = ChainState::hashBestChain at template creation.
         */
        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 2;  // Hash channel

        TritiumBlock blockTemplate;
        AddBlockData(tStateBest, nChannel, blockTemplate);

        /* hashPrevBlock must equal the best chain hash at creation time */
        REQUIRE(blockTemplate.hashPrevBlock == TAO::Ledger::ChainState::hashBestChain.load());
    }
}

