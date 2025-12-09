/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GENESIS_CONSTANTS_H
#define NEXUS_LLP_INCLUDE_GENESIS_CONSTANTS_H

#include <LLC/types/uint1024.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace LLP
{
namespace GenesisConstants
{
    /***************************************************************************
     * GENESIS VALIDATION SYSTEM
     * 
     * The Genesis validation module validates Tritium genesis hashes to
     * enable auto-credit mining rewards directly to trust accounts
     * without requiring wallet login or manual claim transactions.
     * 
     * This system validates:
     * - Genesis hash format and type byte
     * - On-chain existence
     * - Trust account resolution via deterministic addressing
     * - Account ownership and type verification
     **************************************************************************/

    /***************************************************************************
     * Genesis Hash Constants
     **************************************************************************/
    
    /** Genesis hash size in bytes (uint256_t) */
    static const size_t GENESIS_HASH_SIZE = 32;
    
    /** Genesis hash hex string length (64 characters) */
    static const size_t GENESIS_HEX_LENGTH = 64;

    /***************************************************************************
     * Validation Result Enumeration
     **************************************************************************/
    
    /** ValidationResult
     *
     *  Enumeration of possible genesis validation results.
     *
     **/
    enum ValidationResult : uint8_t
    {
        /** Genesis is valid */
        VALID = 0x00,
        
        /** Genesis has invalid size */
        INVALID_SIZE = 0x01,
        
        /** Genesis has invalid type byte */
        INVALID_TYPE = 0x02,
        
        /** Genesis not found on blockchain */
        NOT_ON_CHAIN = 0x03,
        
        /** No default account found for genesis */
        NO_DEFAULT_ACCOUNT = 0x04,
        
        /** Default account is invalid */
        DEFAULT_ACCOUNT_INVALID = 0x05,
        
        /** Genesis is zero */
        ZERO_GENESIS = 0x06
    };

    /***************************************************************************
     * Genesis Validation Functions
     **************************************************************************/

    /** ValidateGenesis
     *
     *  Validate a genesis hash for auto-credit eligibility.
     *  Checks genesis type and on-chain existence.
     *
     *  @param[in] hashGenesis The genesis hash to validate
     *
     *  @return ValidationResult indicating success or failure reason
     *
     **/
    ValidationResult ValidateGenesis(const uint256_t& hashGenesis);

    /** ValidateGenesisString
     *
     *  Validate a genesis hash from hex string.
     *
     *  @param[in] strGenesis The genesis hash as hex string
     *  @param[out] hashGenesis The parsed genesis hash (if valid)
     *
     *  @return ValidationResult indicating success or failure reason
     *
     **/
    ValidationResult ValidateGenesisString(const std::string& strGenesis, uint256_t& hashGenesis);

    /** IsValidGenesisType
     *
     *  Check if genesis has valid type byte for user sigchain.
     *
     *  @param[in] hashGenesis The genesis hash to check
     *
     *  @return true if type byte matches expected user type
     *
     **/
    bool IsValidGenesisType(const uint256_t& hashGenesis);

    /** ExistsOnChain
     *
     *  Check if genesis exists on the blockchain.
     *
     *  @param[in] hashGenesis The genesis hash to check
     *
     *  @return true if genesis has transactions on chain
     *
     **/
    bool ExistsOnChain(const uint256_t& hashGenesis);

    /** ResolveDefaultAccount
     *
     *  Resolve the trust account address for a genesis hash.
     *  Uses deterministic addressing to derive the trust account.
     *  Trust accounts are created automatically for every sigchain.
     *
     *  @param[in] hashGenesis The genesis hash to resolve
     *  @param[out] hashDefault The resolved trust account address
     *
     *  @return true if trust account found
     *
     **/
    bool ResolveDefaultAccount(const uint256_t& hashGenesis, TAO::Register::Address& hashDefault);

    /** ValidateDefaultAccount
     *
     *  Validate that an account is valid for auto-credit.
     *  Checks ownership, type (ACCOUNT or TRUST), and token type (for ACCOUNT).
     *
     *  @param[in] hashDefault The account address to validate
     *  @param[in] hashGenesis The expected owner genesis
     *  @param[out] account The account object (if valid)
     *
     *  @return true if account is valid for auto-credit
     *
     **/
    bool ValidateDefaultAccount(const TAO::Register::Address& hashDefault, 
                                const uint256_t& hashGenesis,
                                TAO::Register::Object& account);

    /** GetValidationResultString
     *
     *  Get human-readable string for validation result.
     *
     *  @param[in] result The validation result
     *
     *  @return String description of the result
     *
     **/
    std::string GetValidationResultString(ValidationResult result);

    /** IsAutoCreditEnabled
     *
     *  Check if auto-credit feature is enabled.
     *  Enabled by default, disabled with -noautocredit flag.
     *
     *  @return true if auto-credit is enabled
     *
     **/
    bool IsAutoCreditEnabled();

} // namespace GenesisConstants
} // namespace LLP

#endif
