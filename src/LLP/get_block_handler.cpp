/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/get_block_handler.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/mining_constants.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/debug.h>

/* TAO::Ledger::GetLastState lives in chainstate.h / state.h */
namespace TAO { namespace Ledger { bool GetLastState(BlockState& state, uint32_t nChannel); } }

namespace LLP
{
    /* Lane-agnostic GET_BLOCK implementation shared by both protocol lanes. */
    GetBlockResult SharedGetBlockHandler(const GetBlockRequest& req)
    {
        /* ── Step 1: Session-scoped rate limit check ──────────────────────────────
         * We use a single GetBlockRollingLimiter keyed by session ID so that a
         * miner with simultaneous legacy + stateless connections shares one 20/60s
         * budget across both lanes (SIM-LINK combined budget).
         * The limiter is retrieved (or created) from the global manager, ensuring
         * the same object is returned for both lane threads of the same session. */
        auto pLimiter = StatelessMinerManager::Get().GetSessionRateLimiter(req.nSessionId);

        const std::string strRateKey = "session=" + std::to_string(req.nSessionId) + "|combined";
        const auto tNow = GetBlockRollingLimiter::clock::now();
        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;

        if(!pLimiter->Allow(strRateKey, tNow, nRetryAfterMs, nCurrentInWindow))
        {
            debug::log(1, FUNCTION,
                "SIM-LINK GET_BLOCK session rate limit exceeded: session=", req.nSessionId,
                " count=", nCurrentInWindow, "/", GET_BLOCK_ROLLING_LIMIT_PER_MINUTE,
                " (combined across all lanes) retry_after_ms=", nRetryAfterMs,
                " lane=", (req.context.nProtocolLane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            GetBlockResult result;
            result.fSuccess      = false;
            result.eReason       = GetBlockPolicyReason::RATE_LIMIT_EXCEEDED;
            result.nRetryAfterMs = nRetryAfterMs;
            return result;
        }

        /* ── Step 2: Create block template ────────────────────────────────────────
         * Call the per-connection block creation callback.  The callback wraps the
         * connection's own new_block() method, which handles coinbase reward address
         * binding and block validation for its lane.
         *
         * Retry once if the first call returns nullptr — this handles the common case
         * where the chain tip advanced between the function entry and the actual
         * CreateBlock() call inside new_block(). */
        TAO::Ledger::Block* pBlock = req.fnCreateBlock();
        if(!pBlock)
        {
            debug::log(2, FUNCTION, "SIM-LINK: new_block() returned nullptr, retrying once (chain may have advanced)");
            pBlock = req.fnCreateBlock();
        }

        if(!pBlock)
        {
            debug::log(2, FUNCTION, "SIM-LINK: new_block() failed after retry — returning INTERNAL_RETRY");

            GetBlockResult result;
            result.fSuccess      = false;
            result.eReason       = GetBlockPolicyReason::INTERNAL_RETRY;
            /* Invariant: INTERNAL_RETRY MUST always carry a non-zero retry_after_ms
             * so miners do not poll blind. */
            result.nRetryAfterMs = MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS;
            return result;
        }

        /* ── Step 3: Session-scoped block template storage ───────────────────────
         * In addition to the per-connection mapBlocks (legacy raw-ptr or stateless
         * TemplateMetadata), we store a shared_ptr copy in the session-scoped map.
         * This enables cross-lane SUBMIT_BLOCK resolution: a solution submitted on
         * port 8323 can resolve a template issued on port 9323, and vice versa.
         *
         * Ownership: the per-connection mapBlocks remains the primary owner.
         * The session block store holds an independent copy (via copy constructor)
         * with its own lifetime managed by shared_ptr. */
        try
        {
            auto spBlockCopy = std::make_shared<TAO::Ledger::Block>(*pBlock);
            StatelessMinerManager::Get().StoreSessionBlock(
                req.nSessionId, pBlock->hashMerkleRoot, std::move(spBlockCopy));
        }
        catch(const std::exception& e)
        {
            /* Non-fatal: log and continue.  The primary per-connection map still holds
             * the template; cross-lane resolution is a best-effort SIM-LINK feature. */
            debug::error(FUNCTION, "SIM-LINK: failed to copy block for session store: ", e.what());
        }

        /* ── Step 4: Serialize the 228-byte BLOCK_DATA payload ───────────────────
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
            debug::error(FUNCTION, "SIM-LINK: Block::Serialize() returned empty vector — TEMPLATE_NOT_READY");

            GetBlockResult result;
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
            "SIM-LINK: GET_BLOCK success session=", req.nSessionId,
            " payload=", vPayload.size(), "B",
            " channel=", pBlock->nChannel,
            " height=", pBlock->nHeight,
            " nBits=", pBlock->nBits,
            " lane=", (req.context.nProtocolLane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

        GetBlockResult result;
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
