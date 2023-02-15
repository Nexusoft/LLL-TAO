/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/Register/types/address.h>

#include <Util/include/json.h>

namespace Legacy         { class Transaction; }

/* Forward declarations. */
namespace TAO::Operation { class Contract;    }
namespace TAO::Register  { class Object;      }
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
     *  @param[in] rContract The contract to de-serialize
     *  @param[in] nContract the id of the contract within the transaction
     *  @param[in] nVerbose The verbose output level.
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) encoding::json ContractToJSON(const TAO::Operation::Contract& rContract,
                                                        const uint32_t nContract, const uint32_t nVerbose = 0);


    /** ConditionToJSON
     *
     *  Converts a serialized contract condition stream to formattted JSON
     *
     *  @param[in] rContract The contract to de-serialize
     *  @param[in] nContract the id of the contract within the transaction
     *  @param[in] nVerbose The verbose output level.
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((pure)) std::string ConditionToJSON(const TAO::Operation::Contract& rContract, const uint32_t nVerbose = 0);


    /** RegisterToJSON
     *
     *  Converts an Object Register to formattted JSON with no external lookups
     *
     *  @param[in] object The Object Register to convert
     *  @param[in] hashRegister The address of the object.
     *
     *  @return the formatted JSON object
     *
     **/
    __attribute__((const)) encoding::json RegisterToJSON(const TAO::Register::Object& rObject,
                                                         const TAO::Register::Address& hashRegister = uint256_t(0));


    /** MembersToJSON
     *
     *  Converts an Object Register's data members to formattted JSON with no external lookups
     *
     *  @param[in] object The Object Register to convert
     *  @param[out] jRet The returned encoding object.
     *
     *
     **/
    void MembersToJSON(const TAO::Register::Object& rObject, encoding::json &jRet);


    /** StateToJSON
     *
     *  Converts an Register's state into formattted JSON with no external lookups
     *
     *  @param[in] vState The register's state to output.
     *  @param[out] jRet The returned encoding object.
     *
     *
     **/
    void StateToJSON(const std::vector<uint8_t>& vState, encoding::json &jRet);


    /** StateToJSON
     *
     *  Converts an Register's state into unformatted string with no external lookups
     *
     *  @param[in] vState The register's state to output.
     *  @param[out] jRet The returned encoding object.
     *
     *
     **/
    __attribute__((const)) std::string StateToJSON(const std::vector<uint8_t>& vState);


    /** StandardToJSON
     *
     *  Encodes the object based on the given command-set standards.
     *
     *  @param[in] jParams The json parameters to check against.
     *  @param[in] rObject The object that we are checking for.
     *  @param[in] hashRegister The address of standard we are encoding for.
     *
     *  @return True if the object type is what was specified.
     *
     **/
    __attribute__((pure)) encoding::json StandardToJSON(const encoding::json& jParams,
                                                        const TAO::Register::Object& rObject,
                                                        const TAO::Register::Address& hashRegister = uint256_t(0));


    /** ChannelToJSON
     *
     *  Generate a json object with channel related data.
     *
     *  @param[in] nChannel The block production channel.
     *
     *  @return encoded json object with given channel stats.
     *
     **/
    __attribute__((pure)) encoding::json ChannelToJSON(const uint32_t nChannel);


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


    /** AddressToJSON
     *
     *  Gets info about an address and creates a json object based on register address
     *
     *  @param[in] hashRegister The address that we are getting information for.
     *  @param[in] rContract The contract that we are getting address info for.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json AddressToJSON(const TAO::Register::Address& hashRegister, const TAO::Operation::Contract& rContract);


    /** AddressToJSON
     *
     *  Gets info about an address and creates a json object based on register address for a foreign register.
     *  This is a register that is not included in the current register's pre-state contract.
     *
     *  @param[in] hashRegister The address that we are getting information for.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json AddressToJSON(const TAO::Register::Address& hashRegister);


    /** RegisterTypesToJSON
     *
     *  Get's the names of the types for this given register to populate among contracts that need to have this info.
     *
     *  @param[in] rContract The contract we are extracting types from using the pre-state.
     *  @param[out] jTypes The returned json value with types populated.
     *
     **/
    void RegisterTypesToJSON(const TAO::Operation::Contract& rContract, encoding::json &jTypes);


    /** RegisterTypesToJSON
     *
     *  Get's the names of the types for this given register to populate among contracts that need to have this info.
     *
     *  @param[in] rObject The object we are extracting types from.
     *  @param[out] jTypes The returned json value with types populated.
     *
     **/
    void RegisterTypesToJSON(const TAO::Register::Object& rObject, encoding::json &jTypes);
}
