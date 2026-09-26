/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/legacy_get_block_handler.h>
#include <LLP/include/mining_constants.h>
#include <LLP/include/mining_template_payload.h>


namespace LLP
{
    /* Legacy-lane-only GET_BLOCK implementation. */
    LegacyGetBlockResult LegacyGetBlockHandler(const LegacyGetBlockRequest& req)
    {
        /* ── Step 1: Per-connection rate limit check ──────────────────────────────
         * The legacy lane uses a per-connection rolling limiter (25/60s) rather
         * than the former session-scoped SIM-LINK shared budget.  Each Miner
         * connection has its own independent rate limit. */
        if(req.pRateLimiter)
        {
            const std::string strRateKey = "legacy|session=" + std::to_string(req.nSessionId);
            const auto tNow = GetBlockRollingLimiter::clock::now();
            uint32_t nRetryAfterMs = 0;
            std::size_t nCurrentInWindow = 0;

            if(!req.pRateLimiter->Allow(strRateKey, tNow, nRetryAfterMs, nCurrentInWindow))
            {
                debug::log(1, FUNCTION,
                    "Legacy lane GET_BLOCK rate limit exceeded: session=", req.nSessionId,
                    " count=", nCurrentInWindow, "/", GET_BLOCK_ROLLING_LIMIT_PER_MINUTE,
                    " retry_after_ms=", nRetryAfterMs);

                LegacyGetBlockResult result;
                result.fSuccess      = false;
                result.eReason       = GetBlockPolicyReason::RATE_LIMIT_EXCEEDED;
                result.nRetryAfterMs = nRetryAfterMs;
                return result;
            }
        }

        /* ── Step 2: Create / serialize template via shared helper ─────────────── */
        const SharedTemplatePayloadResult shared =
            BuildSharedTemplatePayloadWithRetry(req.fnCreateBlock, "Legacy lane");

        LegacyGetBlockResult result;
        result.fSuccess      = shared.fSuccess;
        result.vPayload      = shared.vPayload;
        result.eReason       = shared.eReason;
        result.nRetryAfterMs = shared.nRetryAfterMs;
        result.nUnifiedHeight = shared.nUnifiedHeight;
        result.nChannelHeight = shared.nChannelHeight;
        result.hashBestChain = shared.hashBestChain;
        result.nBlockChannel = shared.nBlockChannel;
        result.nBlockHeight  = shared.nBlockHeight;
        result.nBlockBits    = shared.nBlockBits;
        return result;
    }

} // namespace LLP
