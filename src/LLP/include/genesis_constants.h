/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>
#include <string>

/* Global LLP namespace. */
namespace LLP
{

    /* GenesisConstants namespace for mining authentication utilities. */
    namespace GenesisConstants
    {
        /* Expected length of a genesis hash in hexadecimal format. */
        static const uint32_t GENESIS_HEX_LENGTH = 64;

        /* Validation result codes for genesis validation. */
        enum ValidationResult
        {
            VALID           = 0,   // Genesis is valid
            INVALID_SIZE    = 1,   // Wrong hex string length
            INVALID_TYPE    = 2,   // Wrong type byte for network
            NOT_ON_CHAIN    = 3,   // Genesis not found on blockchain
            ZERO_GENESIS    = 4    // Genesis is zero
        };

        /** ValidateGenesis
         *
         *  Validate a genesis hash for mining authentication.
         *  Used for Falcon signature verification and ChaCha20 key derivation.
         *
         *  @param[in] hashGenesis The genesis hash to validate.
         *
         *  @return ValidationResult indicating success or failure reason.
         *
         **/
        ValidationResult ValidateGenesis(const uint256_t& hashGenesis);


        /** ValidateGenesisString
         *
         *  Validate a genesis hash from hex string and parse it.
         *
         *  @param[in] strGenesis The genesis hash as hex string.
         *  @param[out] hashGenesis The parsed genesis hash.
         *
         *  @return ValidationResult indicating success or failure reason.
         *
         **/
        ValidationResult ValidateGenesisString(const std::string& strGenesis, uint256_t& hashGenesis);


        /** IsValidGenesisType
         *
         *  Check if genesis has valid type byte for user sigchain.
         *
         *  @param[in] hashGenesis The genesis hash to check.
         *
         *  @return True if type byte matches expected user type.
         *
         **/
        bool IsValidGenesisType(const uint256_t& hashGenesis);


        /** ExistsOnChain
         *
         *  Check if genesis exists on the blockchain.
         *
         *  @param[in] hashGenesis The genesis hash to check.
         *
         *  @return True if genesis has transactions on chain.
         *
         **/
        bool ExistsOnChain(const uint256_t& hashGenesis);


        /** GetValidationResultString
         *
         *  Get human-readable string for validation result.
         *
         *  @param[in] result The validation result code.
         *
         *  @return Human-readable description of the result.
         *
         **/
        std::string GetValidationResultString(ValidationResult result);


        /** IsAutoCreditEnabled
         *
         *  Check if auto-credit feature is enabled via config.
         *
         *  @return True if auto-credit is enabled.
         *
         **/
        bool IsAutoCreditEnabled();

    } // namespace GenesisConstants

} // namespace LLP
