/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* List the transactions from a particular sigchain. */
    encoding::json Ledger::ListTransactions(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract input parameters. */
        const uint256_t hashGenesis = ExtractGenesis(jParams);
        const uint32_t  nVerbose    = ExtractVerbose(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Get the last transaction. */
        uint512_t hashLast = 0;
        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-144, "No transactions found");

        /* Loop until genesis, storing all tx into a vector (these will be in descending order). */
        std::vector<TAO::Ledger::Transaction> vtx;
        while(hashLast != 0)
        {
            /* Get the transaction from disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-108, "Failed to read transaction");

            /* Set the next last. */
            hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

            vtx.push_back(tx);
        }

        /* Transactions sorted in descending order. Reverse the order if ascending requested. */
        if(strOrder == "asc")
            std::reverse(vtx.begin(), vtx.end());

        /* JSON return value. */
        encoding::json jRet = encoding::json::array();

        /* Loop through our transactions now and apply filters. */
        uint32_t nTotal = 0;
        for(auto tx : vtx)
        {
            /* Read the block state from the the ledger DB using the transaction hash index */
            TAO::Ledger::BlockState blockState;
            LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

            /* Get the transaction JSON. */
            encoding::json jResult =
                TAO::API::TransactionToJSON(tx, blockState, nVerbose);

            /* Check to see whether the transaction has had all children filtered out */
            if(jResult.empty())
                continue;

            /* Apply our where filters now. */
            if(!FilterResults(jParams, jResult))
                continue;

            /* Filter out our expected fieldnames if specified. */
            FilterFieldname(jParams, jResult);

            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(nTotal - nOffset > nLimit)
                break;

            jRet.push_back(jResult);
        }

        return jRet;
    }
}
