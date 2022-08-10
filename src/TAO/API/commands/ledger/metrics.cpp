/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing mining-related information. */
    encoding::json Ledger::GetMetrics(const encoding::json& jParams, const bool fHelp)
    {
        /* Grab our best block. */
        const TAO::Ledger::BlockState tBestBlock =
            TAO::Ledger::ChainState::stateBest.load();

        /* Get the current time of height of blockchain. */
        const uint64_t nBestTime =
            ExtractInteger<uint64_t>(jParams, "timestamp", tBestBlock.GetBlockTime());

        /* Get total amount of transactions processed. */
        uint64_t nDaily = 0, nWeekly = 0, nMonthly = 0;

        /* Iterate backwards until we have reached one whole day. */
        TAO::Ledger::BlockState tPrevBlock = tBestBlock;
        while(!config::fShutdown.load())
        {
            /* Check our time for days. */
            if(tPrevBlock.GetBlockTime() + 86400 > nBestTime)
                nDaily += (tPrevBlock.vtx.size());

            /* Check our time for weeks. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 > nBestTime)
                nWeekly += (tPrevBlock.vtx.size());

            /* Check our time for months. */
            if(tPrevBlock.GetBlockTime() + 86400 * 7 * 4 > nBestTime)
                nMonthly += (tPrevBlock.vtx.size());
            else
                break;

            /* Iterate to previous block. */
            tPrevBlock = tPrevBlock.Prev();
        }

        /* Build our list of transactions. */
        const encoding::json jTransactions =
        {
            { "daily",   nDaily   },
            { "weekly",  nWeekly  },
            { "monthly", nMonthly }
        };

        /* Add chain-state data. */
        encoding::json jRet;
        jRet["transactions"] = jTransactions;

        /* Filter our fieldname. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
} /* End Ledger Namespace*/
