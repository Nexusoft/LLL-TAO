/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/genesis_constants.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/enum.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>

/* Global LLP namespace. */
namespace LLP
{

    /* GenesisConstants namespace. */
    namespace GenesisConstants
    {

        /* Validate a genesis hash for mining authentication. */
        ValidationResult ValidateGenesis(const uint256_t& hashGenesis)
        {
            /* Check for zero genesis. */
            if(hashGenesis == 0)
                return ZERO_GENESIS;

            /* Check genesis type byte. */
            if(!IsValidGenesisType(hashGenesis))
                return INVALID_TYPE;

            /* Check if genesis exists on chain. */
            if(!ExistsOnChain(hashGenesis))
                return NOT_ON_CHAIN;

            return VALID;
        }


        /* Validate a genesis hash from hex string. */
        ValidationResult ValidateGenesisString(const std::string& strGenesis, uint256_t& hashGenesis)
        {
            /* Check string length. */
            if(strGenesis.length() != GENESIS_HEX_LENGTH)
                return INVALID_SIZE;

            /* Parse hex string. */
            hashGenesis.SetHex(strGenesis);

            /* Validate the parsed genesis. */
            return ValidateGenesis(hashGenesis);
        }


        /* Check if genesis has valid type byte for user sigchain. */
        bool IsValidGenesisType(const uint256_t& hashGenesis)
        {
            /* Get the genesis type byte. */
            uint8_t nType = hashGenesis.GetType();

            /* Get expected user type for current network mode. */
            uint8_t nExpectedType = TAO::Ledger::GENESIS::UserType();

            /* Check if type matches. */
            return (nType == nExpectedType);
        }


        /* Check if genesis exists on the blockchain. */
        bool ExistsOnChain(const uint256_t& hashGenesis)
        {
            /* Check if genesis has any transactions. */
            return LLD::Ledger->HasFirst(hashGenesis);
        }


        /* Get human-readable string for validation result. */
        std::string GetValidationResultString(ValidationResult result)
        {
            switch(result)
            {
                case VALID:
                    return "Valid";
                
                case INVALID_SIZE:
                    return "Invalid size";
                
                case INVALID_TYPE:
                    return "Invalid type byte";
                
                case NOT_ON_CHAIN:
                    return "Not found on blockchain";
                
                case ZERO_GENESIS:
                    return "Zero genesis";
                
                default:
                    return "Unknown validation result";
            }
        }


        /* Check if auto-credit feature is enabled. */
        bool IsAutoCreditEnabled()
        {
            /* Auto-credit is enabled by default, disabled with -noautocredit flag. */
            return !config::GetBoolArg("-noautocredit", false);
        }

    } // namespace GenesisConstants

} // namespace LLP
