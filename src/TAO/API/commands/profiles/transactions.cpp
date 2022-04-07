/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands/profiles.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* List the transactions from a particular sigchain. */
    encoding::json Profiles::Transactions(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract input parameters. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Extract our verbose parameter. */
        const uint32_t  nVerbose =
            ExtractVerbose(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* JSON return value. */
        encoding::json jRet =
            encoding::json::array();

        /* Check for ascending order */
        uint32_t nTotal = 0;
        if(strOrder == "asc")
        {
            /* Get the last transaction. */
            uint512_t hashFirst = 0;
            if(!LLD::Logical->ReadFirst(hashGenesis, hashFirst))
                throw Exception(-144, "No transactions found");

            /* Loop until genesis, storing all tx into a vector (these will be in descending order). */
            while(hashFirst != 0)
            {
                /* Get the transaction from disk. */
                TAO::API::Transaction tx;
                if(!LLD::Logical->ReadTx(hashFirst, tx))
                    throw Exception(-108, "Failed to read transaction");

                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState bState;
                LLD::Ledger->ReadBlock(hashFirst, bState);

                /* Get the transaction JSON. */
                encoding::json jResult =
                    TAO::API::TransactionToJSON(tx, bState, nVerbose);

                /* Set the next last. */
                hashFirst = tx.hashNextTx;

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
        }
        else
        {
            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Logical->ReadLast(hashGenesis, hashLast))
                throw Exception(-144, "No transactions found");

            /* Loop until genesis, storing all tx into a vector (these will be in descending order). */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::API::Transaction tx;
                if(!LLD::Logical->ReadTx(hashLast, tx))
                    throw Exception(-108, "Failed to read transaction");

                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState bState;
                LLD::Ledger->ReadBlock(hashLast, bState);

                /* Get the transaction JSON. */
                encoding::json jResult =
                    TAO::API::TransactionToJSON(tx, bState, nVerbose);

                /* Set the next last. */
                hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

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
        }

        return jRet;
    }
}
