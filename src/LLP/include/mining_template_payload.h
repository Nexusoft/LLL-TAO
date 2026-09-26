/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_TEMPLATE_PAYLOAD_H
#define NEXUS_LLP_INCLUDE_MINING_TEMPLATE_PAYLOAD_H

#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_constants.h>
#include <LLP/include/round_state_utility.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/debug.h>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace LLP
{
    struct SharedTemplatePayloadResult
    {
        bool fSuccess = false;
        std::vector<uint8_t> vPayload;
        GetBlockPolicyReason eReason = GetBlockPolicyReason::NONE;
        uint32_t nRetryAfterMs = 0;
        uint32_t nUnifiedHeight = 0;
        uint32_t nChannelHeight = 0;
        uint1024_t hashBestChain = 0;
        uint32_t nBlockChannel = 0;
        uint32_t nBlockHeight = 0;
        uint32_t nBlockBits = 0;
    };


    /** BuildSharedTemplatePayload
     *
     *  Build the canonical 12-byte metadata + serialized block payload used by
     *  both mining lanes. A nullptr block is treated as a transient template
     *  creation failure and returns INTERNAL_RETRY with the standard retry hint.
     */
    inline SharedTemplatePayloadResult BuildSharedTemplatePayload(
        TAO::Ledger::Block* pBlock, std::string_view strLaneLabel)
    {
        SharedTemplatePayloadResult result;

        if(pBlock == nullptr)
        {
            result.eReason = GetBlockPolicyReason::INTERNAL_RETRY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        /* Consistent-snapshot rule: derive every metadata field — height,
         * channel-height, and hash — from a SINGLE load of stateBest.  The
         * previous code did three independent atomic reads
         * (tStateBest.load(), hashBestChain.load(), GetLastState() over the
         * current chain), which during a fork-resolution storm could
         * straddle several distinct tips and produce a packet whose
         * 12-byte metadata header advertised a different height than the
         * contained 216-byte block body.
         *
         * stateBest.GetHash() is computed from the snapshot itself, so
         * (height, hash) cannot disagree by construction. */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        TAO::Ledger::BlockState stateChannel = stateBest;
        const uint1024_t hashBestChain =
            stateBest.IsNull() ? TAO::Ledger::ChainState::hashBestChain.load()
                               : stateBest.GetHash();
        uint32_t nChannelHeight = 0;
        if(TAO::Ledger::GetLastState(stateChannel, pBlock->nChannel))
            nChannelHeight = stateChannel.nChannelHeight;

        const std::vector<uint8_t> vBlockData = pBlock->Serialize();
        if(vBlockData.empty())
        {
            debug::error(FUNCTION, strLaneLabel, ": Block::Serialize() returned empty — TEMPLATE_NOT_READY");
            result.eReason = GetBlockPolicyReason::TEMPLATE_NOT_READY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        /* Guard: a zero nBits means the difficulty is unresolved — any miner submission
         * built from this template would be rejected.  Treat it as a transient failure
         * and let the caller retry rather than propagating poison nBits to the miner. */
        if(pBlock->nBits == 0)
        {
            debug::error(FUNCTION, strLaneLabel, ": block has nBits == 0 — INTERNAL_RETRY");
            result.eReason = GetBlockPolicyReason::INTERNAL_RETRY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        std::vector<uint8_t> vMetadata = RoundStateUtility::SerializeTemplateMetadata(
            static_cast<uint32_t>(stateBest.nHeight), nChannelHeight, pBlock->nBits);

        result.vPayload.reserve(vMetadata.size() + vBlockData.size());
        result.vPayload.insert(result.vPayload.end(), vMetadata.begin(), vMetadata.end());
        result.vPayload.insert(result.vPayload.end(), vBlockData.begin(), vBlockData.end());

        result.fSuccess = true;
        result.eReason = GetBlockPolicyReason::NONE;
        result.nRetryAfterMs = 0;
        result.nUnifiedHeight = static_cast<uint32_t>(stateBest.nHeight);
        result.nChannelHeight = nChannelHeight;
        result.hashBestChain = hashBestChain;
        result.nBlockChannel = pBlock->nChannel;
        result.nBlockHeight = pBlock->nHeight;
        result.nBlockBits = pBlock->nBits;

        return result;
    }


    /** BuildSharedTemplatePayloadWithRetry
     *
     *  Invoke the supplied block factory, retry once on nullptr, then build the
     *  canonical shared BLOCK_DATA payload from the resulting template. The
     *  callable must return TAO::Ledger::Block* and leave ownership semantics to
     *  the lane-local creator (e.g. per-connection mapBlocks storage).
     */
    template <typename CreateBlockFn>
    inline SharedTemplatePayloadResult BuildSharedTemplatePayloadWithRetry(
        CreateBlockFn&& fnCreateBlock, std::string_view strLaneLabel)
    {
        TAO::Ledger::Block* pBlock = fnCreateBlock();
        if(!pBlock)
        {
            debug::log(3, FUNCTION, strLaneLabel, ": new_block() returned nullptr, retrying once");
            pBlock = fnCreateBlock();
        }

        if(!pBlock)
        {
            debug::log(3, FUNCTION, strLaneLabel, ": new_block() failed after retry — INTERNAL_RETRY");

            SharedTemplatePayloadResult result;
            result.eReason = GetBlockPolicyReason::INTERNAL_RETRY;
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        return BuildSharedTemplatePayload(pBlock, strLaneLabel);
    }
}

#endif
