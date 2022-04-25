/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/constants.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists all transactions for a given register. */
    encoding::json Templates::Transactions(const encoding::json& jParams, const bool fHelp)
    {
        /* The register address of the object to get the transactions for. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Get our verbosity levels. */
        const uint32_t nVerbose = ExtractVerbose(jParams, 2);

        /* Read the register from DB. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashRegister, tObject))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc", strColumn = "timestamp";

        /* Get the params to apply to the response. */
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setTransactions({}, CompareResults(strOrder, strColumn));

        /* Get the list of txid's that modified given register. */
        std::vector<uint512_t> vTransactions;
        if(LLD::Logical->ListTransactions(hashRegister, vTransactions))
        {
            /* Loop through all entries in list. */
            for(const auto& hashLast : vTransactions)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-108, "Failed to read transaction");

                /* Read the block state from the the ledger DB using the transaction hash index */
                TAO::Ledger::BlockState blockState;
                LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

                /* Get the transaction JSON. */
                encoding::json jTransaction =
                    TAO::API::TransactionToJSON(tx, blockState, nVerbose);

                /* Check for missing contracts to adjust. */
                if(jTransaction.find("contracts") == jTransaction.end())
                    continue;

                /* Erase based on having available address. */
                encoding::json& jContracts = jTransaction["contracts"];
                jContracts.erase
                (
                    /* Use custom lambda to sort out contracts that aren't for the address we are searching. */
                    std::remove_if
                    (
                        jContracts.begin(),
                        jContracts.end(),

                        /* Lambda function to remove based on address match. */
                        [&](const encoding::json& jValue)
                        {
                            /* Check for missing value key. */
                            std::string strAddress;
                            if(jValue.find("address") != jValue.end())
                                strAddress = jValue["address"].get<std::string>();

                            /* Check for from value key. */
                            if(jValue.find("from") != jValue.end())
                                strAddress = jValue["from"].get<std::string>();

                            /* Check for to value key. */
                            if(jValue.find("to") != jValue.end())
                                strAddress = jValue["to"].get<std::string>();

                            /* Check that we found parameters. */
                            if(strAddress.empty())
                                return true;

                            /* Check for mismatched address. */
                            if(TAO::Register::Address(strAddress) != hashRegister)
                                return true;

                            return false;
                        }
                    ),
                    jContracts.end()
                );

                /* Check to see whether the transaction has had all children filtered out */
                if(jContracts.empty())
                    continue;

                /* Apply our where filters now. */
                if(!FilterResults(jParams, jTransaction))
                    continue;

                /* Filter out our expected fieldnames if specified. */
                if(!FilterFieldname(jParams, jTransaction))
                    continue;

                /* Insert into set and automatically sort. */
                setTransactions.insert(jTransaction);
            }
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jTransaction : setTransactions)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jTransaction);
        }

        return jRet;
    }
}
