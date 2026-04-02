/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_GET_BLOCK_HANDLER_H
#define NEXUS_LLP_INCLUDE_STATELESS_GET_BLOCK_HANDLER_H

/* Stateless Lane GET_BLOCK Handler
 *
 * Dedicated handler for the stateless mining lane (port 9323).
 * No cross-lane state sharing — rate limiting and template storage are
 * handled per-connection via the StatelessMinerConnection's own mapBlocks
 * and connection-level cooldown timer.
 *
 * This replaces the former SharedGetBlockHandler which shared session-scoped
 * rate limiters and block template stores across both lanes.
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
    /** StatelessGetBlockRequest
     *
     *  Input descriptor for StatelessGetBlockHandler.
     *  The fnCreateBlock callback wraps StatelessMinerConnection::new_block()
     *  which handles coinbase reward binding, TEMPLATE_CREATE_MUTEX coordination,
     *  staleness detection, and mapBlocks storage.
     *
     **/
    struct StatelessGetBlockRequest
    {
        /** Full session context snapshot **/
        MiningContext context;

        /** Block creation callback — wraps per-connection new_block().
         *  Returns a raw pointer owned by the per-connection mapBlocks,
         *  or nullptr on failure. Handler will retry once on nullptr. **/
        std::function<TAO::Ledger::Block*()> fnCreateBlock;
    };


    /** StatelessGetBlockResult
     *
     *  Output from StatelessGetBlockHandler.
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
    struct StatelessGetBlockResult
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


    /** StatelessGetBlockHandler
     *
     *  Stateless-lane-only GET_BLOCK implementation.
     *  Called by StatelessMinerConnection (port 9323).
     *
     *  Performs:
     *    1. req.fnCreateBlock() call — retried once on nullptr return
     *    2. Serialization of the 228-byte BLOCK_DATA payload
     *
     *  No session-scoped rate limiter — the connection-level cooldown
     *  timer is the sole backstop.
     *  No cross-lane block storage — per-connection mapBlocks is the sole store.
     *
     *  @param[in] req  StatelessGetBlockRequest with context and block factory
     *
     *  @return StatelessGetBlockResult with success flag, payload, and metadata
     *
     **/
    StatelessGetBlockResult StatelessGetBlockHandler(const StatelessGetBlockRequest& req);

} // namespace LLP

#endif
