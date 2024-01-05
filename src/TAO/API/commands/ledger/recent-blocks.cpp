/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the block data for a sequential range of blocks starting at a given hash or height. */
    encoding::json Ledger::RecentBlocks(const encoding::json& jParams, const bool fHelp)
    {
        /* Number of results to return. */
        const uint32_t nLimit =
            ExtractInteger<uint32_t>(jParams, "limit", 10, 1440);

        /* Default to verbosity 1 which includes only the hash */
        const uint32_t nVerbose =
            ExtractVerbose(jParams, 1);

        /* Check for height parameter */
        TAO::Ledger::BlockState tLastBlock =
            TAO::Ledger::ChainState::tStateBest.load();

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* List our blocks via a batch read for efficiency. */
        do
        {
            /* Build JSON object. */
            encoding::json jBlock =
                BlockToJSON(tLastBlock, nVerbose);

            /* Filter our results now if desired. */
            if(FilterResults(jParams, jBlock) && FilterFieldname(jParams, jBlock))
                jRet.push_back(jBlock);

            tLastBlock = tLastBlock.Prev();
        }
        while(jRet.size() < nLimit && runtime::unifiedtimestamp() - tLastBlock.nTime < (86400 * 28));

        return jRet;
    }
}
