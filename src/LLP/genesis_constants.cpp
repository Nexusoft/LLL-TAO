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


        /* Resolve the default account address for a genesis hash. */
        bool ResolveDefaultAccount(const uint256_t& hashGenesis, TAO::Register::Address& hashDefault)
        {
            /* First validate the genesis. */
            ValidationResult result = ValidateGenesis(hashGenesis);
            if(result != VALID)
            {
                debug::log(2, FUNCTION, "Genesis validation failed: ", GetValidationResultString(result));
                return false;
            }

            /* For Tritium accounts, derive the default account address directly from genesis.
             * The "default" account is automatically created with every sigchain.
             * Address format: Address(name, genesis, Address::ACCOUNT) */
            hashDefault = TAO::Register::Address(std::string("default"), hashGenesis, TAO::Register::Address::ACCOUNT);

            debug::log(2, FUNCTION, "Derived default account address: ", hashDefault.ToString());

            /* Verify the account exists on chain */
            TAO::Register::Object account;
            if(!LLD::Register->ReadObject(hashDefault, account, TAO::Ledger::FLAGS::LOOKUP))
            {
                debug::log(0, FUNCTION, "Default account not found on chain for genesis ", hashGenesis.SubString());
                debug::log(0, FUNCTION, "  Derived address: ", hashDefault.ToString());
                return false;
            }

            /* Parse and validate the account */
            if(!account.Parse())
            {
                debug::log(0, FUNCTION, "Failed to parse default account object");
                return false;
            }

            /* Verify it's an ACCOUNT type */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
            {
                debug::log(0, FUNCTION, "Register is not an account type: ", uint32_t(account.Base()));
                return false;
            }

            /* Verify ownership matches genesis */
            if(account.hashOwner != hashGenesis)
            {
                debug::log(0, FUNCTION, "Account ownership mismatch: expected ", hashGenesis.SubString(),
                          " but owner is ", account.hashOwner.SubString());
                return false;
            }

            /* Verify it's an NXS account (token = 0) */
            uint256_t hashToken = account.get<uint256_t>("token");
            if(hashToken != 0)
            {
                debug::log(0, FUNCTION, "Default account is not NXS type, token: ", hashToken.SubString());
                return false;
            }

            debug::log(0, FUNCTION, "✓ Resolved default account: ", hashDefault.ToString(),
                      " for genesis ", hashGenesis.SubString());

            return true;
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

            /* Verify account type - must be an ACCOUNT object. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
            {
                debug::log(0, FUNCTION, "Invalid account type: expected ACCOUNT but got ", 
                          uint32_t(account.Base()));
                return false;
            }

            /* Verify token type - must be NXS (token = 0). */
            uint256_t hashToken = account.get<uint256_t>("token");
            if(hashToken != 0)
            {
                debug::log(0, FUNCTION, "Invalid token type: expected NXS (0) but got ", 
                          hashToken.SubString());
                return false;
            }

            return true;
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
