/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_get_block_handler.h>
#include <LLP/include/mining_constants.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/debug.h>

/* TAO::Ledger::GetLastState lives in chainstate.h / state.h */
namespace TAO { namespace Ledger { bool GetLastState(BlockState& state, uint32_t nChannel); } }

namespace LLP
{
    /* Stateless-lane-only GET_BLOCK implementation. */
    StatelessGetBlockResult StatelessGetBlockHandler(const StatelessGetBlockRequest& req)
    {
        /* ── Step 1: Create block template ────────────────────────────────────────
         * Call the per-connection block creation callback.  The callback wraps
         * StatelessMinerConnection::new_block(), which handles coinbase reward
         * address binding, TEMPLATE_CREATE_MUTEX coordination, staleness detection,
         * fork handling, and mapBlocks storage.
         *
         * Retry once if the first call returns nullptr — this handles the common
         * case where the chain tip advanced between function entry and the actual
         * CreateBlock() call inside new_block(). */
        TAO::Ledger::Block* pBlock = req.fnCreateBlock();
        if(!pBlock)
        {
            debug::log(2, FUNCTION, "Stateless lane: new_block() returned nullptr, retrying once");
            pBlock = req.fnCreateBlock();
        }

        if(!pBlock)
        {
            debug::log(2, FUNCTION, "Stateless lane: new_block() failed after retry — INTERNAL_RETRY");

            StatelessGetBlockResult result;
            result.fSuccess      = false;
            result.eReason       = GetBlockPolicyReason::INTERNAL_RETRY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        /* ── Step 2: Serialize the 228-byte BLOCK_DATA payload ───────────────────
         * NexusMiner wire protocol:
         *   [0-3]    nUnifiedHeight  (big-endian uint32)
         *   [4-7]    nChannelHeight  (big-endian uint32)
         *   [8-11]   nBits           (big-endian uint32)
         *   [12-227] Block::Serialize() (216 bytes) */
        const uint32_t nUnifiedHeight =
            static_cast<uint32_t>(TAO::Ledger::ChainState::nBestHeight.load());

        uint32_t nChannelHeight = 0;
        {
            TAO::Ledger::BlockState stateChannel = TAO::Ledger::ChainState::tStateBest.load();
            if(TAO::Ledger::GetLastState(stateChannel, pBlock->nChannel))
                nChannelHeight = stateChannel.nChannelHeight;
        }

        const std::vector<uint8_t> vData = pBlock->Serialize();
        if(vData.empty())
        {
            debug::error(FUNCTION, "Stateless lane: Block::Serialize() returned empty — TEMPLATE_NOT_READY");

            StatelessGetBlockResult result;
            result.fSuccess      = false;
            result.eReason       = GetBlockPolicyReason::TEMPLATE_NOT_READY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        std::vector<uint8_t> vPayload;
        vPayload.reserve(12 + vData.size());
        vPayload.push_back(static_cast<uint8_t>((nUnifiedHeight >> 24) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nUnifiedHeight >> 16) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nUnifiedHeight >>  8) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nUnifiedHeight      ) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nChannelHeight >> 24) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nChannelHeight >> 16) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nChannelHeight >>  8) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((nChannelHeight      ) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((pBlock->nBits >> 24) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((pBlock->nBits >> 16) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((pBlock->nBits >>  8) & 0xFF));
        vPayload.push_back(static_cast<uint8_t>((pBlock->nBits      ) & 0xFF));
        vPayload.insert(vPayload.end(), vData.begin(), vData.end());

        debug::log(2, FUNCTION,
            "Stateless lane: GET_BLOCK success",
            " payload=", vPayload.size(), "B",
            " channel=", pBlock->nChannel,
            " height=", pBlock->nHeight,
            " nBits=", pBlock->nBits);

        StatelessGetBlockResult result;
        result.fSuccess      = true;
        result.vPayload      = std::move(vPayload);
        result.eReason       = GetBlockPolicyReason::NONE;
        result.nRetryAfterMs = 0;
        result.nBlockChannel = pBlock->nChannel;
        result.nBlockHeight  = pBlock->nHeight;
        result.nBlockBits    = pBlock->nBits;
        return result;
    }

} // namespace LLP
