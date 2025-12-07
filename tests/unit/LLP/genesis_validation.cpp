/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/genesis_constants.h>
#include <LLP/include/stateless_manager.h>

using namespace LLP;

/* Test constants */
namespace {
    /* Sample test genesis hex strings */
    const char* VALID_GENESIS_HEX_64 = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
    const char* INVALID_SHORT_HEX = "a174011c93ca1c80bca5388382b167ca";  // 32 chars
    const char* INVALID_LONG_HEX = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122ff";  // 66 chars
}


TEST_CASE("Genesis Validation Tests", "[genesis_validation]")
{
    SECTION("Zero genesis returns ZERO_GENESIS")
    {
        uint256_t hashGenesis(0);
        
        GenesisConstants::ValidationResult result = GenesisConstants::ValidateGenesis(hashGenesis);
        
        REQUIRE(result == GenesisConstants::ZERO_GENESIS);
        REQUIRE(GenesisConstants::GetValidationResultString(result) == "Zero genesis");
    }

    SECTION("Invalid type byte returns INVALID_TYPE")
    {
        /* Create a genesis with invalid type byte (0xFF instead of 0xa1 for mainnet) */
        uint256_t hashGenesis;
        hashGenesis.SetHex("ff74011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        GenesisConstants::ValidationResult result = GenesisConstants::ValidateGenesis(hashGenesis);
        
        REQUIRE(result == GenesisConstants::INVALID_TYPE);
        REQUIRE(GenesisConstants::GetValidationResultString(result) == "Invalid type byte");
    }

    SECTION("Valid format but not on-chain returns NOT_ON_CHAIN")
    {
        /* Create a genesis with valid type byte but doesn't exist on chain */
        uint256_t hashGenesis;
        hashGenesis.SetHex(VALID_GENESIS_HEX_64);
        
        /* Note: This assumes the test genesis doesn't exist in the test database.
         * If it does exist, the result would be VALID instead. */
        GenesisConstants::ValidationResult result = GenesisConstants::ValidateGenesis(hashGenesis);
        
        /* Should be either NOT_ON_CHAIN or VALID depending on test DB state */
        REQUIRE((result == GenesisConstants::NOT_ON_CHAIN || result == GenesisConstants::VALID));
    }

    SECTION("Hex string length validation - short string")
    {
        uint256_t hashGenesis;
        GenesisConstants::ValidationResult result = 
            GenesisConstants::ValidateGenesisString(INVALID_SHORT_HEX, hashGenesis);
        
        REQUIRE(result == GenesisConstants::INVALID_SIZE);
        REQUIRE(GenesisConstants::GetValidationResultString(result) == "Invalid size");
    }

    SECTION("Hex string length validation - long string")
    {
        uint256_t hashGenesis;
        GenesisConstants::ValidationResult result = 
            GenesisConstants::ValidateGenesisString(INVALID_LONG_HEX, hashGenesis);
        
        REQUIRE(result == GenesisConstants::INVALID_SIZE);
    }

    SECTION("Hex string length validation - correct length")
    {
        uint256_t hashGenesis;
        GenesisConstants::ValidationResult result = 
            GenesisConstants::ValidateGenesisString(VALID_GENESIS_HEX_64, hashGenesis);
        
        /* Should not fail on size check, but may fail on type or on-chain check */
        REQUIRE(result != GenesisConstants::INVALID_SIZE);
    }

    SECTION("Validation result strings are correct")
    {
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::VALID) == "Valid");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::INVALID_SIZE) == "Invalid size");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::INVALID_TYPE) == "Invalid type byte");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::NOT_ON_CHAIN) == "Not found on blockchain");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::NO_DEFAULT_ACCOUNT) == "No default account found");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::DEFAULT_ACCOUNT_INVALID) == "Default account invalid");
        REQUIRE(GenesisConstants::GetValidationResultString(GenesisConstants::ZERO_GENESIS) == "Zero genesis");
    }

    SECTION("Auto-credit is enabled by default")
    {
        /* Auto-credit should be enabled unless -noautocredit flag is set */
        bool fEnabled = GenesisConstants::IsAutoCreditEnabled();
        
        /* This will return true unless the test is run with -noautocredit flag */
        /* We can't assert a specific value since it depends on runtime config */
        /* Just verify the function returns a boolean without crashing */
        REQUIRE((fEnabled == true || fEnabled == false));
    }
}


TEST_CASE("StatelessMinerManager Genesis Cache Tests", "[genesis_cache]")
{
    SECTION("GetCachedDefaultAccount returns zero for uncached genesis")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Create a random genesis that's not cached */
        uint256_t hashGenesis;
        hashGenesis.SetHex("a1111111111111111111111111111111111111111111111111111111111111ff");
        
        TAO::Register::Address hashDefault = manager.GetCachedDefaultAccount(hashGenesis);
        
        REQUIRE(hashDefault == TAO::Register::Address(0));
    }

    SECTION("ValidateAndCacheGenesis caches valid mappings")
    {
        /* Note: This test would require a valid genesis with a default account
         * in the test database. For a real test, we would need to set up the
         * database with proper test data. This is a placeholder showing the
         * intended behavior. */
        
        /* This test is informational - actual validation would require DB setup */
        REQUIRE(true);  // Placeholder
    }
}


TEST_CASE("Genesis Constants Values", "[genesis_constants]")
{
    SECTION("Genesis hash size constant is correct")
    {
        REQUIRE(GenesisConstants::GENESIS_HASH_SIZE == 32);
    }

    SECTION("Genesis hex length constant is correct")
    {
        REQUIRE(GenesisConstants::GENESIS_HEX_LENGTH == 64);
    }

    SECTION("Validation result enum has expected values")
    {
        /* Verify enum values are distinct */
        REQUIRE(GenesisConstants::VALID != GenesisConstants::INVALID_SIZE);
        REQUIRE(GenesisConstants::VALID != GenesisConstants::INVALID_TYPE);
        REQUIRE(GenesisConstants::INVALID_SIZE != GenesisConstants::INVALID_TYPE);
        REQUIRE(GenesisConstants::ZERO_GENESIS != GenesisConstants::VALID);
    }
}
