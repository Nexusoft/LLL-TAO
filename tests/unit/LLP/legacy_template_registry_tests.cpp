/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/types/miner.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/tritium.h>

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


TEST_CASE("Legacy template registry tracks same-height reorg anchors", "[llp][miner][template_registry]")
{
    ChainStateRestore restore;
    LLP::Miner miner;

    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
    stateBest.nHeight = 77;
    TAO::Ledger::ChainState::tStateBest.store(stateBest);
    TAO::Ledger::ChainState::nBestHeight.store(77);
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(111));

    REQUIRE(miner.CheckBestHeightForTests());

    auto* pBlock = new TAO::Ledger::TritiumBlock();
    pBlock->hashMerkleRoot = uint512_t(222);
    pBlock->nChannel = TAO::Ledger::CHANNEL::HASH;
    pBlock->nHeight = 78;

    REQUIRE(miner.RegisterTemplateForTests(pBlock) == pBlock);
    REQUIRE(miner.LookupTemplateForTests(pBlock->hashMerkleRoot) == pBlock);
    REQUIRE(miner.FindTemplateForTests(pBlock->hashMerkleRoot));

    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(333));

    REQUIRE(miner.CheckBestHeightForTests());
    REQUIRE_FALSE(miner.FindTemplateForTests(uint512_t(222)));
    REQUIRE(miner.LookupTemplateForTests(uint512_t(222)) == nullptr);
}


TEST_CASE("Legacy template lookups do not mutate cache on misses", "[llp][miner][template_registry]")
{
    LLP::Miner miner;
    const uint512_t hashMissing(444);

    REQUIRE_FALSE(miner.FindTemplateForTests(hashMissing));
    REQUIRE(miner.LookupTemplateForTests(hashMissing) == nullptr);

    REQUIRE_FALSE(miner.SignBlockForTests(123, hashMissing));
    REQUIRE_FALSE(miner.FindTemplateForTests(hashMissing));
    REQUIRE(miner.LookupTemplateForTests(hashMissing) == nullptr);

    REQUIRE_FALSE(miner.ValidateBlockForTests(hashMissing));
    REQUIRE_FALSE(miner.FindTemplateForTests(hashMissing));
    REQUIRE(miner.LookupTemplateForTests(hashMissing) == nullptr);
}
