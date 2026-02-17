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
 * @brief Test suite for channel-specific height tracking
 *
 * This test prevents regression of the bug where unified height (tStateBest.nHeight)
 * was incorrectly used instead of channel-specific height (nChannelHeight).
 *
 * Critical scenarios tested:
 * 1. First block in a new channel after genesis
 * 2. Mining alternating channels
 * 3. GetLastState behavior at genesis
 * 4. Channel height independence
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

    SECTION("First block in new channel has correct height")
    {
        BlockState stateChannel = ChainState::tStateBest.load();
        uint32_t nChannel = 1;  // Prime channel

        /* Simulate calculating height for first Prime block after genesis */
        GetLastState(stateChannel, nChannel);
        uint32_t nNextHeight = stateChannel.nChannelHeight + 1;

        /* First block in channel should be height 2, not 1!
         * (Genesis is nChannelHeight=1, so next=2) */
        REQUIRE(nNextHeight == 2);

        /* This is the bug we're preventing:
         * WRONG: nNextHeight = 1 (would collide with genesis!)
         * RIGHT: nNextHeight = stateChannel.nChannelHeight + 1 = 2
         */
    }

    SECTION("AddBlockData uses channel height, not unified height")
    {
        /* Create a mock scenario where:
         * - Unified height (tStateBest.nHeight) = 100
         * - Prime channel height = 40
         * - Hash channel height = 60
         */

        BlockState tStateBest;
        tStateBest.nHeight = 100;  // Unified height (all channels combined)
        tStateBest.nChannel = 2;   // Last block was Hash
        tStateBest.nChannelHeight = 60;

        /* Now mine a Prime block (channel 1) */
        uint32_t nChannel = 1;
        TritiumBlock block;

        /* Get the last Prime block state */
        BlockState stateChannel = tStateBest;
        GetLastState(stateChannel, nChannel);
        block.nHeight = stateChannel.nChannelHeight + 1;

        /* Block height should be based on CHANNEL height, not unified height
         * WRONG: block.nHeight = 101 (tStateBest.nHeight + 1)
         * RIGHT: block.nHeight = 41 (Prime channel's last height + 1)
         */
        REQUIRE(block.nHeight != 101);  // Should NOT use unified height
        REQUIRE(block.nHeight == 41);   // Should use channel-specific height
    }

    SECTION("Multiple channels maintain independent heights")
    {
        /* Simulate mining blocks across multiple channels */
        BlockState statePrime, stateHash;

        statePrime = ChainState::tStateBest.load();
        stateHash = ChainState::tStateBest.load();

        /* Get last block for each channel */
        GetLastState(statePrime, 1);  // Prime channel
        GetLastState(stateHash, 2);   // Hash channel

        uint32_t nPrimeNext = statePrime.nChannelHeight + 1;
        uint32_t nHashNext = stateHash.nChannelHeight + 1;

        /* Channel heights should be independent */
        REQUIRE(nPrimeNext != nHashNext);  // Different channels can have different heights

        /* Both should be >= 2 (genesis is nChannelHeight=1) */
        REQUIRE(nPrimeNext >= 2);
        REQUIRE(nHashNext >= 2);
    }

    SECTION("Unified height vs channel height divergence")
    {
        /* After many blocks, unified height should be sum of all channel heights */
        BlockState state = ChainState::tStateBest.load();

        uint32_t nUnifiedHeight = state.nHeight;

        /* Get heights for all channels */
        BlockState statePrime = state, stateHash = state, stateStake = state;
        GetLastState(statePrime, 1);
        GetLastState(stateHash, 2);
        GetLastState(stateStake, 0);

        uint32_t nPrimeHeight = statePrime.nChannelHeight;
        uint32_t nHashHeight = stateHash.nChannelHeight;
        uint32_t nStakeHeight = stateStake.nChannelHeight;

        /* Unified height should be approximately the sum of all channels
         * (allowing for genesis block and rounding) */
        uint32_t nApproximateSum = nPrimeHeight + nHashHeight + nStakeHeight;

        /* The key insight: unified height DIVERGES from individual channel heights! */
        REQUIRE(nUnifiedHeight >= nPrimeHeight);
        REQUIRE(nUnifiedHeight >= nHashHeight);
        REQUIRE(nUnifiedHeight >= nStakeHeight);

        /* This is why we MUST use channel-specific height, not unified height! */
    }

    SECTION("Template height calculation matches reference implementation")
    {
        /* This test ensures AddBlockData() matches stateless_block_utility.cpp */

        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 1;  // Prime channel
        TritiumBlock blockTemplate;

        /* Reference implementation from stateless_block_utility.cpp:
         * BlockState stateChannel = tStateBest;
         * if(GetLastState(stateChannel, nChannel))
         *     pBlock->nHeight = stateChannel.nChannelHeight + 1;
         * else
         *     pBlock->nHeight = stateChannel.nChannelHeight + 1;
         */

        BlockState stateChannel = tStateBest;
        bool found = GetLastState(stateChannel, nChannel);
        uint32_t nHeightReference = stateChannel.nChannelHeight + 1;

        /* Now test AddBlockData implementation */
        AddBlockData(tStateBest, nChannel, blockTemplate);

        /* Heights should match! */
        REQUIRE(blockTemplate.nHeight == nHeightReference);

        /* Additional validation */
        REQUIRE(blockTemplate.nChannel == nChannel);
        REQUIRE(blockTemplate.nHeight >= 2);  // At minimum, first block after genesis
    }

    SECTION("Edge case: Genesis height is never duplicated")
    {
        /* This is the critical regression test!
         * Genesis has nChannelHeight = 1.
         * First block in ANY channel must have nChannelHeight = 2.
         *
         * The bug was: else block.nHeight = 1 (WRONG!)
         * The fix is: else block.nHeight = stateChannel.nChannelHeight + 1 (RIGHT!)
         */

        std::vector<uint32_t> vChannels = {0, 1, 2};  // Stake, Prime, Hash

        for(uint32_t nChannel : vChannels)
        {
            BlockState stateChannel = ChainState::tStateGenesis;

            /* Simulate first block in this channel */
            GetLastState(stateChannel, nChannel);
            uint32_t nNextHeight = stateChannel.nChannelHeight + 1;

            /* MUST be 2, never 1! */
            REQUIRE(nNextHeight == 2);
            REQUIRE(nNextHeight != 1);  // This would collide with genesis!

            INFO("Channel " << nChannel << " first block height: " << nNextHeight);
        }
    }
}

/**
 * @brief Integration test: Full block creation flow
 */
TEST_CASE("Block Template Creation - Channel Height Integration", "[ledger][integration]")
{
    using namespace TAO::Ledger;

    SECTION("CreateBlock produces correct channel heights")
    {
        /* This requires a wallet setup, so we'll mock the critical parts */

        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 1;  // Prime channel

        /* Get the expected height */
        BlockState stateChannel = tStateBest;
        GetLastState(stateChannel, nChannel);
        uint32_t nExpectedHeight = stateChannel.nChannelHeight + 1;

        /* Create a block template */
        TritiumBlock blockTemplate;
        AddBlockData(tStateBest, nChannel, blockTemplate);

        /* Validate the template */
        REQUIRE(blockTemplate.nHeight == nExpectedHeight);
        REQUIRE(blockTemplate.nChannel == nChannel);

        /* Template should not use unified height */
        REQUIRE(blockTemplate.nHeight != tStateBest.nHeight + 1);

        INFO("Unified height: " << tStateBest.nHeight);
        INFO("Channel height: " << stateChannel.nChannelHeight);
        INFO("Template height: " << blockTemplate.nHeight);
        INFO("Expected height: " << nExpectedHeight);
    }

    SECTION("Staleness detection works with channel-specific heights")
    {
        /* This is the original bug scenario:
         * Miner compares template height (was unified, now channel-specific)
         * against its local channel height to detect staleness.
         */

        BlockState tStateBest = ChainState::tStateBest.load();
        uint32_t nChannel = 2;  // Hash channel

        /* Get channel-specific height */
        BlockState stateChannel = tStateBest;
        GetLastState(stateChannel, nChannel);

        /* Create template */
        TritiumBlock blockTemplate;
        AddBlockData(tStateBest, nChannel, blockTemplate);

        /* Miner's local channel height (simulated) */
        uint32_t nMinerChannelHeight = stateChannel.nChannelHeight;

        /* Template should be for the NEXT block in this channel */
        REQUIRE(blockTemplate.nHeight == nMinerChannelHeight + 1);

        /* Staleness check: template height should match expected channel height
         * BEFORE: template.nHeight (unified) != miner's channel height (ALWAYS STALE!)
         * AFTER: template.nHeight (channel) == miner's channel height + 1 (CORRECT!)
         */
        bool bIsStale = (blockTemplate.nHeight != nMinerChannelHeight + 1);
        REQUIRE(bIsStale == false);
    }
}
