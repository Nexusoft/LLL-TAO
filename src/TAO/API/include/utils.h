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
#include <TAO/Register/types/object.h>
#include <Legacy/types/transaction.h>

#include <LLC/hash/argon2.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** NamespaceHash
         *
         *  Generates a lightweight argon2 hash of the namespace string.
         *
         *  @return The 256 bit hash of this key in the series.
         **/
        uint256_t NamespaceHash(const SecureString& strNamespace);


        /** RegisterAddressFromName
         *
         *  Resolves a register address from a name.  
         *  The register address is a hash of the fully-namespaced name in the format of namespacehash:objecttype:name. 
         *
         *  @param[in] params The json request params
         *  @param[in] strObjectType The object type to include in the string to build the register hash from 
         *  @param[in] strObjectName The name parameter to use in the register hash  
         *  
         *  @return The 256 bit hash of the object name.
         **/
        uint256_t RegisterAddressFromName(const json::json& params, const std::string& strObjectType, const std::string& strObjectName );

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


        /** ObjectRegisterToJSON
        *
        *  Converts an Object Register to formattted JSON
        *
        *  @param[in] object The Object Register to convert
        *  @param[in] strDataField An optional data field to filter the response on
        *
        *  @return the formatted JSON object
        *
        **/
        json::json ObjectRegisterToJSON(const TAO::Register::Object object, const std::string strDataField);

    }
}
