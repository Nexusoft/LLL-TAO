/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>
#include <Util/include/json.h>

namespace Legacy         { class Transaction; }

/* Forward declarations. */
namespace TAO::Operation { class Contract;    }
namespace TAO::Register
{
    class Object;
    class Address;
}
namespace TAO::Ledger
{
    class Transaction;
    class BlockState;
}

/* Global TAO namespace. */
namespace TAO::API
{
    /** BlockToJSON
     *
     *  Converts the block to formatted JSON
     *
     *  @param[in] block The block to convert
     *  @param[in] nVerbose determines the amount of transaction data to include in the response
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json BlockToJSON(const TAO::Ledger::BlockState& block, const uint32_t nVerbose);


    /** TransactionToJSON
     *
     *  Converts the transaction to formatted JSON
     *
     *  @param[in] tx The transaction to convert to JSON
     *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
     *  @param[in] nVerbose determines the amount of transaction data to include in the response
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json TransactionToJSON(const TAO::Ledger::Transaction& tx,
                                                           const TAO::Ledger::BlockState& block, const uint32_t nVerbose = 0);


    /** TransactionToJSON
     *
     *  Converts the transaction to formatted JSON
     *
     *  @param[in] tx The transaction to convert to JSON
     *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
     *  @param[in] nVerbose determines the amount of transaction data to include in the response
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block,
                                     const uint32_t nVerbose = 0);


    /** ContractToJSON
     *
     *  Converts a serialized contract stream to formattted JSON
     *
     *  @param[in] contract The contract to de-serialize
     *  @param[in] nContract the id of the contract within the transaction
     *  @param[in] nVerbose The verbose output level.
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json ContractToJSON(const TAO::Operation::Contract& contract,
                                                        const uint32_t nContract, const uint32_t nVerbose = 0);

    /** ObjectToJSON
     *
     *  Converts an Object Register to formattted JSON with no external lookups
     *
     *  @param[in] object The Object Register to convert
     *  @param[in] hashRegister The address of the object.
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json ObjectToJSON(const TAO::Register::Object& object, const uint256_t& hashRegister = 0);


    /** ObjectToJSON
     *
     *  Converts an Object Register to formattted JSON
     *
     *  @param[in] params The paramets passed in the request
     *  @param[in] object The Object Register to convert
     *  @param[in] hashRegister The register address of the object
     *  @param[in] fLookupName True if the Name object record should be looked up
     *
     *  @return the formatted JSON object
     *
     **/
    encoding::json ObjectToJSON(const encoding::json& params,
                            const TAO::Register::Object& object,
                            const TAO::Register::Address& hashRegister,
                            bool fLookupName = true);


    /** StatementToJSON
     *
     *  Recursive helper function for QueryToJSON to recursively generate JSON statements for use with filters.
     *
     *  @param[out] vWhere The statements to parse into query.
     *  @param[out] nIndex The current statement being processed.
     *  @param[out] jStatement The statement reference to pass through recursion.
     *
     *  @return The JSON statement generated with query.
     *
     **/
    __attribute__((pure)) encoding::json StatementToJSON(std::vector<std::string> &vWhere, uint32_t &nIndex, encoding::json &jStatement);


    /** QueryToJSON
     *
     *  Turns a where query string in url encoding into a formatted JSON object.
     *
     *  @param[in] strWhere The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json QueryToJSON(const std::string& strWhere);


    /** ClauseToJSON
     *
     *  Turns a where clause string in url encoding into a formatted JSON object.
     *
     *  @param[in] strClause The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json ClauseToJSON(const std::string& strClause);


    /** ParamsToJSON
     *
     *  Turns parameters from url encoding into a formatted JSON object.
     *
     *  @param[in] vParams The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json ParamsToJSON(const std::vector<std::string>& vParams);
}
