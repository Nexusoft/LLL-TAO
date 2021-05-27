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
#include <TAO/API/types/clause.h>

namespace Legacy { class Transaction; }

/* Global TAO namespace. */
namespace TAO
{
    namespace Operation { class Contract; }
    namespace Register
    {
        class Object;
        class Address;
    }
    namespace Ledger
    {
        class Transaction;
        class BlockState;
    }

    /* API Layer namespace. */
    namespace API
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
        json::json BlockToJSON(const TAO::Ledger::BlockState& block, const uint32_t nVerbose,
                               const std::map<std::string, std::vector<Clause>>& vWhere = std::map<std::string, std::vector<Clause>>());


        /** TransactionToJSON
         *
         *  Converts the transaction to formatted JSON
         *
         *  @param[in] hashCaller Genesis hash of the callers sig chain (0 if not logged in)
         *  @param[in] tx The transaction to convert to JSON
         *  @param[in] block The block that the transaction exists in.  If null this will be loaded witin the method
         *  @param[in] nVerbose determines the amount of transaction data to include in the response
         *  @param[in] hashCoinbase Used to filter out coinbase transactions to only those belonging to hashCoinbase
         *
         *  @return the formatted JSON object
         *
         **/
        json::json TransactionToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction& tx,
                                     const TAO::Ledger::BlockState& block, const uint32_t nVerbose,
                                     const uint256_t& hashCoinbase = 0,
                                     const std::map<std::string, std::vector<Clause>>& vWhere = std::map<std::string, std::vector<Clause>>());


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
        json::json TransactionToJSON(const Legacy::Transaction& tx, const TAO::Ledger::BlockState& block, const uint32_t nVerbose,
                                     const std::map<std::string, std::vector<Clause>>& vWhere = std::map<std::string, std::vector<Clause>>());


        /** ContractsToJSON
         *
         *  Converts a transaction object into a formatted JSON list of contracts bound to the transaction.
         *
         *  @param[in] hashCaller Genesis hash of the callers sig chain (0 if not logged in)
         *  @param[in] tx The transaction with contracts to convert to JSON
         *  @param[in] nVerbose The verbose output level.
         *  @param[in] hashCoinbase Used to filter out coinbase transactions to only those belonging to hashCoinbase
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ContractsToJSON(const uint256_t& hashCaller, const TAO::Ledger::Transaction& tx,
                                   const uint32_t nVerbose = 0, const uint256_t& hashCoinbase = 0,
                                   const std::map<std::string, std::vector<Clause>>& vWhere = std::map<std::string, std::vector<Clause>>());


        /** ContractToJSON
         *
         *  Converts a serialized contract stream to formattted JSON
         *
         *  @param[in] hashCaller Genesis hash of the callers sig chain (0 if not logged in)
         *  @param[in] contract The contract to de-serialize
         *  @param[in] nContract the id of the contract within the transaction
         *  @param[in] nVerbose The verbose output level.
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ContractToJSON(const uint256_t& hashCaller, const TAO::Operation::Contract& contract,
                                  const uint32_t nContract, const uint32_t nVerbose = 0);

        /** ObjectToJSON
         *
         *  Converts an Object Register to formattted JSON with no external lookups
         *
         *  @param[in] object The Object Register to convert
         *
         *  @return the formatted JSON object
         *
         **/
        json::json ObjectToJSON(const TAO::Register::Object& object);


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
        json::json ObjectToJSON(const json::json& params,
                                const TAO::Register::Object& object,
                                const TAO::Register::Address& hashRegister,
                                bool fLookupName = true);


        /** FilterResponse
        *
        *  If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field
        *
        *  @param[in] params The parameters passed into the request
        *  @param[in] response The reponse JSON to be filtered.
        *
        **/
        void FilterResponse(const json::json& params, json::json& response);


        /** GetListParams
        *
        *  Extracts the paramers applicable to a List API call in order to apply a filter/offset/limit to the result
        *
        *  @param[in] params The parameters passed into the request
        *  @param[out] strOrder The sort order to apply
        *  @param[out] nLimit The number of results to return
        *  @param[out] nOffset The offset to apply to the results
        *
        **/
        void GetListParams(const json::json& params, std::string &strOrder, uint32_t &nLimit, uint32_t &nOffset);


        /** MatchesWhere
        *
        *  Checks to see if the json response matches the where clauses
        *
        *  @param[in] obj The JSON to be filtered
        *  @param[in] vWhere Vector of clauses to apply to filter the results
        *  @param[in] vIgnore Vector of fieldnames to ignore in the vWhere list.  This is useful for those /list/xxx methods that
        *             require non-standard params but do not want them interpreted as a where clause.
        *
        *  @return True if the json response meets all of the clauses
        *
        **/
        bool MatchesWhere(const json::json& obj, const std::vector<Clause>& vWhere, const std::vector<std::string>& vIgnore = std::vector<std::string>());


        /** RegisterType
         *
         *  Returns a type string for the register type
         *
         *  @param[in] nType The register type enum
         *
         *  @return A string representation of the register type
         *
         **/
        std::string RegisterType(const uint8_t nType);


        /** ObjectType
         *
         *  Returns a type string for the register object type
         *
         *  @param[in] nType The object type enum
         *
         *  @return A string representation of the object register type
         *
         **/
        std::string ObjectType(const uint8_t nType);

    }
}
