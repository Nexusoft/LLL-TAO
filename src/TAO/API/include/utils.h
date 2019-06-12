/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Legacy/types/transaction.h>

#include <TAO/Register/types/object.h>

#include <Util/include/json.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {


        /** CreateName
        *
        *  Creates a new Name Object register for the given name and register address adds the register operation to the transaction
        *
        *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
        *  @param[in] strFullName The Name of the object.  May include the namespace suffix
        *  @param[in] hashRegister The register address that the Name object should resolve to
        *  @param[out] contract The contract to create a name for.
        *
        **/
        void CreateName(const uint256_t& hashGenesis, const std::string strFullName,
                        const uint256_t& hashRegister, TAO::Operation::Contract& contract);


        /** CreateNameFromTransfer
        *
        *  Creates a new Name Object register for an object being transferred
        *
        *  @param[in] hashTransfer The transaction ID of the transfer transaction being claimed
        *  @param[in] uint256_t hashGenesis The genesis hash of the signature chain to create the Name for
        *
        *  @return contract The contract to create a name for.
        *
        **/
        TAO::Operation::Contract CreateNameFromTransfer(const uint512_t& hashTransfer,
                                    const uint256_t& hashGenesis);


        /** AddressFromName
         *
         *  Resolves a register address from a name by looking up a Name object.
         *
         *  @param[in] params The json request params
         *  @param[in] strObjectName The name parameter to use in the register hash
         *
         *  @return The 256 bit hash of the object name.
         **/
        uint256_t AddressFromName(const json::json& params, const std::string& strObjectName);


        /** IsRegisterAddress
         *
         *  Determins whether a string value is a register address.
         *  This only checks to see if the value is 64 characters in length and all hex characters (i.e. can be converted to a uint256).
         *  It does not check to see whether the register address exists in the database
         *
         *  @param[in] strValueToCheck The value to check
         *
         *  @return True if the value is 64 characters in length and all hex characters (i.e. can be converted to a uint256).
         **/
        bool IsRegisterAddress(const std::string& strValueToCheck);


        /** GetDigits
        *
        *  Retrieves the number of digits that applies to amounts for this token or account object.
        *  If the object register passed in is a token account then we need to look at the token definition
        *  in order to get the digits.  The token is obtained by looking at the token_address field,
        *  which contains the register address of the issuing token
        *
        *  @param[in] object The Object Register to determine the digits for
        *
        *  @return the number of digits that apply to amounts for this token or account
        *
        **/
        uint64_t GetDigits(const TAO::Register::Object& object);


        /** GetTokenNameForAccount
        *
        *  Retrieves the token name for the token that this account object is used for.
        *  The token is obtained by looking at the token_address field,
        *  which contains the register address of the issuing token
        *
        *  @param[in] hashCaller genesis ID hash of the caller of the API
        *  @param[in] object The Object Register of the token account
        *
        *  @return the token name for the token that this account object is used for
        *
        **/
        std::string GetTokenNameForAccount(const uint256_t& hashCaller, const TAO::Register::Object& object);


        /** GetRegistersOwnedBySigChain
        *
        *  Scans a signature chain to work out all registers that it owns
        *
        *  @param[in] hashGenesis The genesis hash of the signature chain to scan
        *  @param[out] vRegisters The list of register addresses from sigchain.
        *
        *  @return A vector of register addresses owned by the sig chain
        *
        **/
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<uint256_t>& vRegisters);


        /** GetObjectName
        *
        *  Scans the Name records associated with the hashCaller sig chain to find an entry with a matching hashObject address
        *
        *  @param[in] hashObject register address of the object to look up
        *  @param[in] hashCaller genesis ID hash of the caller of the API
        *  @param[in] hashOwner genesid ID hash of the object owner
        *
        *  @return the name of the object, if one is found
        *
        **/
        std::string GetRegisterName(const uint256_t& hashObject, const uint256_t& hashCaller, const uint256_t& hashOwner);


    }
}
