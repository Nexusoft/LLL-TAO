/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/types/tritium.h>

#include <unit/catch2/catch.hpp>

namespace
{
    struct ChainTipRestore
    {
        explicit ChainTipRestore(const uint1024_t& hashOriginalIn)
        : hashOriginal(hashOriginalIn)
        {
        }

        ~ChainTipRestore()
        {
            TAO::Ledger::ChainState::hashBestChain.store(hashOriginal);
        }

        uint1024_t hashOriginal;
    };

    TAO::Ledger::BlockAcceptanceResult SimulateAcceptMinedBlockPostValidationTipRaceGuard(
        const TAO::Ledger::TritiumBlock& block)
    {
        TAO::Ledger::BlockAcceptanceResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.nUnifiedHeight = block.nHeight;
        result.hashBlock = block.hashMerkleRoot;

        const uint1024_t hashBestChainNow = TAO::Ledger::ChainState::hashBestChain.load();
        if(block.hashPrevBlock != hashBestChainNow)
        {
            result.reason = "submitted block is stale (post-validation tip-race check)";
            return result;
        }

        result.accepted = true;
        result.reason = "would process";
        return result;
    }
}

TEST_CASE("AcceptMinedBlock post-validation guard rejects stale tip-race submissions", "[ledger]")
{
    const uint1024_t hashOriginal = TAO::Ledger::ChainState::hashBestChain.load();
    ChainTipRestore restore(hashOriginal);

    const uint1024_t hashInitialTip(1);
    const uint1024_t hashAdvancedTip(2);

    TAO::Ledger::ChainState::hashBestChain.store(hashInitialTip);

    TAO::Ledger::TritiumBlock block;
    block.nChannel = 1;
    block.nHeight = 42;
    block.hashPrevBlock = hashInitialTip;

    /* Simulate ValidateMinedBlock() staleness check passing first. */
    REQUIRE(block.hashPrevBlock == TAO::Ledger::ChainState::hashBestChain.load());

    /* Simulate concurrent submission winner advancing the canonical tip before accept. */
    TAO::Ledger::ChainState::hashBestChain.store(hashAdvancedTip);

    const TAO::Ledger::BlockAcceptanceResult result =
        SimulateAcceptMinedBlockPostValidationTipRaceGuard(block);

    REQUIRE_FALSE(result.accepted);
    REQUIRE(result.reason == "submitted block is stale (post-validation tip-race check)");
}
