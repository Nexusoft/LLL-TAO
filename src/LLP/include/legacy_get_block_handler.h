/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_LEGACY_GET_BLOCK_HANDLER_H
#define NEXUS_LLP_INCLUDE_LEGACY_GET_BLOCK_HANDLER_H

/* Legacy Lane GET_BLOCK Handler
 *
 * Dedicated handler for the legacy mining lane (port 8323).
 * No cross-lane state sharing — rate limiting is per-connection via the
 * Miner's own GetBlockRollingLimiter member, and template storage uses
 * the Miner's per-connection mapBlocks.
 *
 * This replaces the former SharedGetBlockHandler which shared session-scoped
 * rate limiters and block template stores across both lanes.
 */

#include <LLP/include/get_block_policy.h>

#include <functional>
#include <vector>
#include <cstdint>

/* Forward declaration */
namespace TAO { namespace Ledger { class Block; } }

namespace LLP
{
    /** LegacyGetBlockRequest
     *
     *  Input descriptor for LegacyGetBlockHandler.
     *  The fnCreateBlock callback wraps Miner::new_block() which handles
     *  coinbase reward binding and prime-mod iteration.
     *
     **/
    struct LegacyGetBlockRequest
    {
        /** Session ID for logging **/
        uint32_t nSessionId = 0;

        /** Per-connection rate limiter — owned by the Miner instance.
         *  If null, rate limiting is skipped (e.g. unit tests). **/
        GetBlockRollingLimiter* pRateLimiter = nullptr;

        /** Block creation callback — wraps per-connection new_block().
         *  Returns a raw pointer owned by the per-connection mapBlocks,
         *  or nullptr on failure. Handler will retry once on nullptr. **/
        std::function<TAO::Ledger::Block*()> fnCreateBlock;
    };


    /** LegacyGetBlockResult
     *
     *  Output from LegacyGetBlockHandler.
     *
     *  If fSuccess == true:
     *    - vPayload contains the 228-byte BLOCK_DATA wire payload
     *    - eReason == GetBlockPolicyReason::NONE
     *
     *  If fSuccess == false:
     *    - vPayload is empty
     *    - eReason describes the failure cause
     *    - nRetryAfterMs is the retry hint
     *
     **/
    struct LegacyGetBlockResult
    {
        bool fSuccess = false;
        std::vector<uint8_t> vPayload;
        GetBlockPolicyReason eReason = GetBlockPolicyReason::NONE;
        uint32_t nRetryAfterMs  = 0;

        /* Block metadata — populated on success */
        uint32_t nBlockChannel  = 0;
        uint32_t nBlockHeight   = 0;
        uint32_t nBlockBits     = 0;
    };


    /** LegacyGetBlockHandler
     *
     *  Legacy-lane-only GET_BLOCK implementation.
     *  Called by Miner (port 8323).
     *
     *  Performs:
     *    1. Per-connection rate limit check (25/60s rolling window)
     *    2. req.fnCreateBlock() call — retried once on nullptr return
     *    3. Serialization of the 228-byte BLOCK_DATA payload
     *
     *  No session-scoped rate limiter — per-connection limiter is sole gate.
     *  No cross-lane block storage — per-connection mapBlocks is the sole store.
     *
     *  @param[in] req  LegacyGetBlockRequest with session info and block factory
     *
     *  @return LegacyGetBlockResult with success flag, payload, and metadata
     *
     **/
    LegacyGetBlockResult LegacyGetBlockHandler(const LegacyGetBlockRequest& req);

} // namespace LLP

#endif
