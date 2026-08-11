#include <unit/catch2/catch.hpp>

#include <LLP/include/legacy_get_block_handler.h>
#include <LLP/include/mining_template_payload.h>
#include <LLP/include/stateless_get_block_handler.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/convert.h>

#include <memory>

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


    TAO::Ledger::TritiumBlock* MakeTemplateBlock()
    {
        auto* pBlock = new TAO::Ledger::TritiumBlock();
        pBlock->nVersion = 9;
        pBlock->nChannel = TAO::Ledger::CHANNEL::HASH;
        pBlock->nHeight = 123;
        pBlock->nBits = 0x1a2b3c4d;
        pBlock->hashMerkleRoot = uint512_t(456);
        return pBlock;
    }
}


TEST_CASE("Shared template payload exposes the snapshot used for metadata", "[llp][template][anchors]")
{
    ChainStateRestore restore;

    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
    stateBest.nHeight = 321;
    TAO::Ledger::ChainState::tStateBest.store(stateBest);
    TAO::Ledger::ChainState::nBestHeight.store(321);
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(654));

    std::unique_ptr<TAO::Ledger::TritiumBlock> pBlock(MakeTemplateBlock());

    const LLP::SharedTemplatePayloadResult result =
        LLP::BuildSharedTemplatePayload(pBlock.get(), "unit test");

    REQUIRE(result.fSuccess);
    REQUIRE(result.nUnifiedHeight == 321u);
    REQUIRE(result.hashBestChain == uint1024_t(654));
    REQUIRE(result.nBlockBits == pBlock->nBits);
    REQUIRE(result.vPayload.size() >= 12u);
    REQUIRE(convert::bytes2uint(result.vPayload, 0) == result.nUnifiedHeight);
    REQUIRE(convert::bytes2uint(result.vPayload, 4) == result.nChannelHeight);
    REQUIRE(convert::bytes2uint(result.vPayload, 8) == result.nBlockBits);
}


TEST_CASE("Lane GET_BLOCK helpers propagate shared template anchors", "[llp][template][anchors][handlers]")
{
    ChainStateRestore restore;

    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
    stateBest.nHeight = 777;
    TAO::Ledger::ChainState::tStateBest.store(stateBest);
    TAO::Ledger::ChainState::nBestHeight.store(777);
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(888));

    SECTION("Legacy lane")
    {
        std::unique_ptr<TAO::Ledger::TritiumBlock> pBlock(MakeTemplateBlock());

        LLP::LegacyGetBlockRequest req;
        req.fnCreateBlock = [&pBlock]() -> TAO::Ledger::Block*
        {
            return pBlock.get();
        };

        const LLP::LegacyGetBlockResult result = LLP::LegacyGetBlockHandler(req);
        REQUIRE(result.fSuccess);
        REQUIRE(result.nUnifiedHeight == 777u);
        REQUIRE(result.hashBestChain == uint1024_t(888));
        REQUIRE(convert::bytes2uint(result.vPayload, 0) == result.nUnifiedHeight);
        REQUIRE(convert::bytes2uint(result.vPayload, 4) == result.nChannelHeight);
        REQUIRE(convert::bytes2uint(result.vPayload, 8) == result.nBlockBits);
    }

    SECTION("Stateless lane")
    {
        std::unique_ptr<TAO::Ledger::TritiumBlock> pBlock(MakeTemplateBlock());

        LLP::StatelessGetBlockRequest req;
        req.fnCreateBlock = [&pBlock]() -> TAO::Ledger::Block*
        {
            return pBlock.get();
        };

        const LLP::StatelessGetBlockResult result = LLP::StatelessGetBlockHandler(req);
        REQUIRE(result.fSuccess);
        REQUIRE(result.nUnifiedHeight == 777u);
        REQUIRE(result.hashBestChain == uint1024_t(888));
        REQUIRE(convert::bytes2uint(result.vPayload, 0) == result.nUnifiedHeight);
        REQUIRE(convert::bytes2uint(result.vPayload, 4) == result.nChannelHeight);
        REQUIRE(convert::bytes2uint(result.vPayload, 8) == result.nBlockBits);
    }
}
