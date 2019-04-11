/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>
#include <TAO/Ledger/include/chainstate.h>
#include <Legacy/types/transaction.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** BlockToJSON
        *
        *  Converts the block to formatted JSON
        *
        *  @param[in] block The block to convert
        *  @param[in] nTransactionVerbosity determines the amount of transaction data to include in the response
        *
        *  @return the formatted JSON object
        *
        **/
        json::json BlockToJSON(const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity);


        /** TransactionToJSON
        *
        *  Converts the transaction to formatted JSON
        *
        *  @param[in] tx The transaction to convert to JSON
        *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
        *  @param[in] nTransactionVerbosity determines the amount of transaction data to include in the response
        *
        *  @return the formatted JSON object
        *
        **/
        json::json TransactionToJSON(TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity);


        /** TransactionToJSON
        *
        *  Converts the transaction to formatted JSON
        *
        *  @param[in] tx The transaction to convert to JSON
        *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
        *  @param[in] nTransactionVerbosity determines the amount of transaction data to include in the response
        *
        *  @return the formatted JSON object
        *
        **/
        json::json TransactionToJSON(Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nTransactionVerbosity);


        /** OperationToJSON
        *
        *  Converts a serialized operation stream to formattted JSON
        *
        *  @param[in] ssOperation The serialized Operation stream to convert
        *
        *  @return the formatted JSON object
        *
        **/
        json::json OperationToJSON(const TAO::Operation::Stream& ssOperation);

    }
}
