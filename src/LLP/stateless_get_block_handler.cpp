/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_get_block_handler.h>
#include <LLP/include/mining_constants.h>
#include <LLP/include/mining_template_payload.h>

namespace LLP
{
    /* Stateless-lane-only GET_BLOCK implementation. */
    StatelessGetBlockResult StatelessGetBlockHandler(const StatelessGetBlockRequest& req)
    {
        const SharedTemplatePayloadResult shared =
            BuildSharedTemplatePayloadWithRetry(req.fnCreateBlock, "Stateless lane");

        StatelessGetBlockResult result;
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
