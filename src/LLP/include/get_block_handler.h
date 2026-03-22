/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GET_BLOCK_HANDLER_H
#define NEXUS_LLP_INCLUDE_GET_BLOCK_HANDLER_H

/* SIM-LINK Dual-Lane Architecture — Shared GET_BLOCK Handler
 *
 * This header defines the lane-agnostic GET_BLOCK implementation shared by both
 * the legacy (port 8323, Miner class) and stateless (port 9323,
 * StatelessMinerConnection class) mining protocol lanes.
 *
 * ARCHITECTURE INVARIANTS:
 *   1. "Never serve authenticated in-budget miners 0 block data" — if fSuccess is
 *      true, vPayload MUST be non-empty (228 bytes).
 *   2. "20/60s combined budget" — the rate limit is session-scoped (shared across
 *      both lanes) via StatelessMinerManager::GetSessionRateLimiter().
 *   3. "Thread safety" — the session-scoped limiter and block store are protected
 *      by their own mutexes inside StatelessMinerManager.
 *
 * USAGE:
 *   // Legacy lane (miner.cpp) after auth + reward-bound check:
 *   GetBlockRequest req;
 *   req.context   = context;
 *   req.nSessionId = context.nSessionId;
 *   req.fnCreateBlock = [this]() -> TAO::Ledger::Block* { return new_block(); };
 *   GetBlockResult result = SharedGetBlockHandler(req);
 *
 *   // Stateless lane (stateless_miner_connection.cpp) after auth + rate-limit checks:
 *   GetBlockRequest req;
 *   req.context   = context;
 *   req.nSessionId = context.nSessionId;
 *   req.fnCreateBlock = [this]() -> TAO::Ledger::Block* { return new_block(); };
 *   GetBlockResult result = SharedGetBlockHandler(req);
 */

#include <LLP/include/get_block_policy.h>
#include <LLP/include/stateless_miner.h>    /* MiningContext */

#include <functional>
#include <vector>
#include <cstdint>

/* Forward declaration */
namespace TAO { namespace Ledger { class Block; } }

namespace LLP
{
    /** GetBlockRequest
     *
     *  Input descriptor for SharedGetBlockHandler.
     *  Callers supply the session context and a factory callback for block creation,
     *  allowing each lane to use its own new_block() implementation while sharing the
     *  rate-limit, session storage, and serialization logic.
     *
     **/
    struct GetBlockRequest
    {
        /** Full session context (has nSessionId, ProtocolLane, hashRewardAddress, nChannel) **/
        MiningContext context;

        /** Session ID used for rate-limiter and session block map lookup **/
        uint32_t nSessionId = 0;

        /** Block creation callback — wraps the per-connection new_block() call.
         *  Returns a raw pointer owned by the per-connection mapBlocks, or nullptr on
         *  failure (chain advancing mid-creation).  SharedGetBlockHandler will retry
         *  once on nullptr before returning INTERNAL_RETRY. **/
        std::function<TAO::Ledger::Block*()> fnCreateBlock;
    };


    /** GetBlockResult
     *
     *  Output from SharedGetBlockHandler.
     *
     *  If fSuccess == true:
     *    - vPayload contains the 228-byte BLOCK_DATA wire payload
     *      (12-byte header prefix + 216-byte serialized block)
     *    - eReason == GetBlockPolicyReason::NONE
     *    - nRetryAfterMs == 0
     *    - nBlockChannel / nBlockHeight / nBlockBits are populated from the created block
     *
     *  If fSuccess == false:
     *    - vPayload is empty
     *    - eReason describes the failure cause
     *    - nRetryAfterMs is the retry hint (0 for non-retryable reasons)
     *    - nBlockChannel / nBlockHeight / nBlockBits are 0
     *
     **/
    struct GetBlockResult
    {
        bool fSuccess = false;
        std::vector<uint8_t> vPayload;
        GetBlockPolicyReason eReason = GetBlockPolicyReason::NONE;
        uint32_t nRetryAfterMs  = 0;

        /* Block metadata — populated on success for connection-specific post-processing */
        uint32_t nBlockChannel  = 0;  // pBlock->nChannel from the created block template
        uint32_t nBlockHeight   = 0;  // pBlock->nHeight  from the created block template
        uint32_t nBlockBits     = 0;  // pBlock->nBits    from the created block template
    };


    /** SharedGetBlockHandler
     *
     *  Lane-agnostic GET_BLOCK implementation.
     *  Called by both Miner (legacy lane) and StatelessMinerConnection (stateless lane).
     *
     *  Performs:
     *    1. Session-scoped rate limit check (shared 20/60s budget across both lanes)
     *    2. req.fnCreateBlock() call — retried once on nullptr return
     *    3. Session-scoped block template storage for cross-lane SUBMIT_BLOCK resolution
     *    4. Serialization of the 228-byte BLOCK_DATA payload
     *
     *  Does NOT perform: authentication check, channel check, session validity check.
     *  Callers MUST gate on those before calling this handler.
     *
     *  Thread-safe: may be called concurrently from legacy and stateless connection threads
     *  for the same session.  Rate limiter and session block store are internally locked.
     *
     *  @param[in] req  GetBlockRequest with session context and block factory callback
     *
     *  @return GetBlockResult with success flag, payload, and failure metadata
     *
     **/
    GetBlockResult SharedGetBlockHandler(const GetBlockRequest& req);

} // namespace LLP

#endif
