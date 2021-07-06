/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands/register.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/constants.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists all transactions for a given register. */
    encoding::json Register::Transactions(const encoding::json& jParams, const bool fHelp)
    {
        /* The register address of the object to get the transactions for. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Get our verbosity levels. */
        const uint32_t nVerbose = ExtractVerbose(jParams, 2);

        /* Read the register from DB. */
        TAO::Register::State objCheck;
        if(!LLD::Register->ReadState(hashRegister, objCheck))
            throw Exception(-13, "Object not found");

        /* Use this asset to get our genesis-id adjusting if it is in system state. */
        uint256_t hashGenesis = objCheck.hashOwner;
        if(hashGenesis.GetType() == TAO::Ledger::GENESIS::SYSTEM)
            hashGenesis.SetType(TAO::Ledger::GENESIS::UserType());

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc";

        /* Get the params to apply to the response. */
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Get the last transaction. */
        uint512_t hashLast = 0;
        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
            throw Exception(-144, "No transactions found");

        /* JSON return value. */
        encoding::json jRet = encoding::json::array();

        /* Loop until genesis. */
        uint32_t nTotal = 0;
        while(hashLast != 0)
        {
            /* Get the transaction from disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-108, "Failed to read transaction");

            /* Set the next last. */
            hashLast = !tx.IsFirst() ? tx.hashPrevTx : 0;

            /* Read the block state from the the ledger DB using the transaction hash index */
            TAO::Ledger::BlockState blockState;
            LLD::Ledger->ReadBlock(tx.GetHash(), blockState);

            /* Get the transaction JSON. */
            encoding::json jResult =
                TAO::API::TransactionToJSON(tx, blockState, nVerbose);

            /* Check for missing contracts to adjust. */
            if(jResult.find("contracts") == jResult.end())
                continue;

            /* Erase based on having available address. */
            encoding::json& jContracts = jResult["contracts"];
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

            /* Check for a claim that would iterate to another sigchain. */
            for(const auto& jContract : jContracts)
            {
                /* Check for claim OP. */
                if(jContract["OP"] == "CLAIM")
                {
                    hashLast = uint512_t(jContract["txid"].get<std::string>());
                    break;
                }

                /* Check for create OP to break from loop. */
                if(jContract["OP"] == "CREATE")
                {
                    hashLast = 0;
                    break;
                }
            }

            /* Check to see whether the transaction has had all children filtered out */
            if(jContracts.empty())
                continue;

            /* Apply our where filters now. */
            if(!FilterResults(jParams, jResult))
                continue;

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
