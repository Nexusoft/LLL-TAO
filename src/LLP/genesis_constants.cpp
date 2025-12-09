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
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/names.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>

/* Global LLP namespace. */
namespace LLP
{

    /* GenesisConstants namespace. */
    namespace GenesisConstants
    {

        /* Validate a genesis hash for auto-credit eligibility. */
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


        /* Resolve the default account address for a genesis hash.
         * Uses SessionDB::ListRegisters to enumerate accounts owned by the genesis.
         * The default account is always created with every sigchain. */
        bool ResolveDefaultAccount(const uint256_t& hashGenesis, TAO::Register::Address& hashDefault)
        {
            /* First validate the genesis. */
            ValidationResult result = ValidateGenesis(hashGenesis);
            if(result != VALID)
            {
                debug::log(2, FUNCTION, "Genesis validation failed: ", GetValidationResultString(result));
                return false;
            }

            /* List all registers owned by this genesis.
             * This works without login - it's a database lookup. */
            std::set<TAO::Register::Address> setRegisters;
            if(!LLD::Sessions || !LLD::Sessions->ListRegisters(hashGenesis, setRegisters))
            {
                debug::log(0, FUNCTION, "Failed to list registers for genesis ", hashGenesis.SubString());
                debug::log(0, FUNCTION, "  SessionDB may not be initialized or genesis has no registers");
                return false;
            }

            debug::log(2, FUNCTION, "Found ", setRegisters.size(), " registers for genesis ", hashGenesis.SubString());

            /* Iterate through registers to find the default NXS account.
             * The default account is always created with every sigchain. */
            for(const auto& hashRegister : setRegisters)
            {
                /* Read the register object */
                TAO::Register::Object account;
                if(!LLD::Register->ReadObject(hashRegister, account, TAO::Ledger::FLAGS::LOOKUP))
                    continue;

                /* Parse the account object before accessing fields */
                if(!account.Parse())
                    continue;

                /* Check if this is an ACCOUNT type */
                if(account.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                    continue;

                /* Check if this is an NXS account (token = 0) */
                uint256_t hashToken = account.get<uint256_t>("token");
                if(hashToken != 0)
                    continue;

                /* Verify ownership matches genesis */
                if(account.hashOwner != hashGenesis)
                {
                    debug::log(2, FUNCTION, "Skipping account with mismatched owner");
                    continue;
                }

                /* Found the default NXS account! */
                hashDefault = hashRegister;

                debug::log(0, FUNCTION, "✓ Resolved default NXS account: ", hashDefault.ToString(),
                          " for genesis ", hashGenesis.SubString());

                return true;
            }

            debug::log(0, FUNCTION, "No NXS account found for genesis ", hashGenesis.SubString());
            debug::log(0, FUNCTION, "  The default account should be created automatically with every sigchain");
            return false;
        }


        /* Validate that a default account is valid for auto-credit. */
        bool ValidateDefaultAccount(const TAO::Register::Address& hashDefault, 
                                    const uint256_t& hashGenesis,
                                    TAO::Register::Object& account)
        {
            /* Read the account state from the register database. */
            if(!LLD::Register->ReadState(hashDefault, account))
            {
                debug::log(0, FUNCTION, "Failed to read account state for ", hashDefault.SubString());
                return false;
            }

            /* Verify ownership - account must be owned by the genesis. */
            if(account.hashOwner != hashGenesis)
            {
                debug::log(0, FUNCTION, "Account ownership mismatch: expected ", hashGenesis.SubString(),
                          " but got ", account.hashOwner.SubString());
                return false;
            }

            /* Parse the account object. */
            if(!account.Parse())
            {
                debug::log(0, FUNCTION, "Failed to parse account object");
                return false;
            }

            /* Verify account type - must be an ACCOUNT or TRUST object.
             * TRUST accounts are preferred for mining rewards as they're deterministically addressed. */
            uint8_t nBase = account.Base();
            uint8_t nStandard = account.Standard();
            
            if(nStandard == TAO::Register::OBJECTS::TRUST)
            {
                /* Trust account - no token field to check, always holds NXS */
                debug::log(2, FUNCTION, "Validated TRUST account for auto-credit");
                return true;
            }
            else if(nBase == TAO::Register::OBJECTS::ACCOUNT)
            {
                /* Regular account - verify it's NXS (token = 0) */
                uint256_t hashToken = account.get<uint256_t>("token");
                if(hashToken != 0)
                {
                    debug::log(0, FUNCTION, "Invalid token type: expected NXS (0) but got ", 
                              hashToken.SubString());
                    return false;
                }
                debug::log(2, FUNCTION, "Validated ACCOUNT for auto-credit");
                return true;
            }
            else
            {
                debug::log(0, FUNCTION, "Invalid account type: expected ACCOUNT or TRUST but got ", 
                          uint32_t(nBase), " (standard: ", uint32_t(nStandard), ")");
                return false;
            }
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
                
                case NO_DEFAULT_ACCOUNT:
                    return "No default account found";
                
                case DEFAULT_ACCOUNT_INVALID:
                    return "Default account invalid";
                
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
