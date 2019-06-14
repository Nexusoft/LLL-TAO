/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>
#include <TAO/Register/types/object.h>
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
         *  @param[in] nVerbosity determines the amount of transaction data to include in the response
         *
         *  @return the formatted JSON object
         *
         **/
        json::json BlockToJSON(const TAO::Ledger::BlockState& block, uint32_t nVerbosity);


        /** TransactionToJSON
         *
         *  Converts the transaction to formatted JSON
         *
         *  @param[in] tx The transaction to convert to JSON
         *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
         *  @param[in] nVerbosity determines the amount of transaction data to include in the response
         *
         *  @return the formatted JSON object
         *
         **/
        json::json TransactionToJSON(const TAO::Ledger::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nVerbosity);


        /** TransactionToJSON
         *
         *  Converts the transaction to formatted JSON
         *
         *  @param[in] tx The transaction to convert to JSON
         *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
         *  @param[in] nVerbosity determines the amount of transaction data to include in the response
         *
         *  @return the formatted JSON object
         *
         **/
        json::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, uint32_t nVerbosity);


        /** ContractsToJSON
         *
         *  Converts a transaction object into a formatted JSON list of contracts bound to the transaction.
         *
         *  @param[in] tx The transaction with contracts to convert to JSON
         *  @param[in] nVerbosity The verbose output level.
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ContractsToJSON(const TAO::Ledger::Transaction& tx, uint32_t nVerbosity = 0);


        /** ContractToJSON
         *
         *  Converts a serialized contract stream to formattted JSON
         *
         *  @param[in] contract The contract to de-serialize
         *  @param[in] nVerbosity The verbose output level.
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ContractToJSON(const TAO::Operation::Contract& contract, uint32_t nVerbosity = 0);


        /** ObjectToJSON
         *
         *  Converts an Object Register to formattted JSON
         *
         *  @param[in] params The paramets passed in the request
         *  @param[in] object The Object Register to convert
         *  @param[in] hashRegister The register address of the object
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ObjectToJSON(const json::json& params, const TAO::Register::Object& object, const uint256_t& hashRegister);

    }
}
