/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_config.h>

#include <Util/include/config.h>
#include <Util/include/args.h>

#include <string>

/* Test the mining configuration validation and auto-configuration helpers.
 */

TEST_CASE("Mining Configuration Tests", "[miner][config]")
{
    /* Save original config state to restore after tests */
    std::map<std::string, std::string> mapOriginalArgs = config::mapArgs;
    std::map<std::string, std::vector<std::string>> mapOriginalMultiArgs = config::mapMultiArgs;
    
    SECTION("ValidateMiningPubkey - with valid miningpubkey")
    {
        /* Set up valid miningpubkey */
        config::mapArgs["-miningpubkey"] = "1234567890abcdef1234567890abcdef";
        
        std::string strError;
        bool fValid = LLP::ValidateMiningPubkey(strError);
        
        REQUIRE(fValid);
        REQUIRE(strError.empty());
    }
    
    SECTION("ValidateMiningPubkey - with valid miningaddress")
    {
        /* Clear miningpubkey and set miningaddress instead */
        config::mapArgs.erase("-miningpubkey");
        config::mapArgs["-miningaddress"] = "8BuhE1y5oard3cEWqCSYK7NhqR1PC8j9kECftvTdHMjE4LLt8jd";
        
        std::string strError;
        bool fValid = LLP::ValidateMiningPubkey(strError);
        
        REQUIRE(fValid);
        REQUIRE(strError.empty());
    }
    
    SECTION("ValidateMiningPubkey - missing both pubkey and address")
    {
        /* Clear both config options */
        config::mapArgs.erase("-miningpubkey");
        config::mapArgs.erase("-miningaddress");
        
        std::string strError;
        bool fValid = LLP::ValidateMiningPubkey(strError);
        
        REQUIRE_FALSE(fValid);
        REQUIRE_FALSE(strError.empty());
        REQUIRE(strError.find("Missing mining configuration") != std::string::npos);
    }
    
    SECTION("ValidateMiningPubkey - empty miningpubkey")
    {
        /* Set empty miningpubkey */
        config::mapArgs["-miningpubkey"] = "";
        
        std::string strError;
        bool fValid = LLP::ValidateMiningPubkey(strError);
        
        REQUIRE_FALSE(fValid);
        REQUIRE_FALSE(strError.empty());
    }
    
    SECTION("ValidateMiningPubkey - too short miningpubkey")
    {
        /* Set very short pubkey */
        config::mapArgs["-miningpubkey"] = "abc";
        
        std::string strError;
        bool fValid = LLP::ValidateMiningPubkey(strError);
        
        REQUIRE_FALSE(fValid);
        REQUIRE(strError.find("too short") != std::string::npos);
    }
    
    SECTION("GetMiningPubkey - retrieve miningpubkey")
    {
        std::string strTestPubkey = "1234567890abcdef1234567890abcdef";
        config::mapArgs["-miningpubkey"] = strTestPubkey;
        
        std::string strPubkey;
        bool fFound = LLP::GetMiningPubkey(strPubkey);
        
        REQUIRE(fFound);
        REQUIRE(strPubkey == strTestPubkey);
    }
    
    SECTION("GetMiningPubkey - retrieve miningaddress when no pubkey")
    {
        std::string strTestAddress = "8BuhE1y5oard3cEWqCSYK7NhqR1PC8j9kECftvTdHMjE4LLt8jd";
        config::mapArgs.erase("-miningpubkey");
        config::mapArgs["-miningaddress"] = strTestAddress;
        
        std::string strPubkey;
        bool fFound = LLP::GetMiningPubkey(strPubkey);
        
        REQUIRE(fFound);
        REQUIRE(strPubkey == strTestAddress);
    }
    
    SECTION("GetMiningPubkey - missing both")
    {
        config::mapArgs.erase("-miningpubkey");
        config::mapArgs.erase("-miningaddress");
        
        std::string strPubkey;
        bool fFound = LLP::GetMiningPubkey(strPubkey);
        
        REQUIRE_FALSE(fFound);
    }
    
    SECTION("LoadMiningConfig - with valid config")
    {
        /* Set up valid configuration */
        config::mapArgs["-miningpubkey"] = "1234567890abcdef1234567890abcdef";
        config::mapMultiArgs["-llpallowip"].clear();
        config::mapMultiArgs["-llpallowip"].push_back("192.168.1.1");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        REQUIRE(fLoaded);
    }
    
    SECTION("LoadMiningConfig - auto-default llpallowip")
    {
        /* Set up config without llpallowip */
        config::mapArgs["-miningpubkey"] = "1234567890abcdef1234567890abcdef";
        config::mapMultiArgs.erase("-llpallowip");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        REQUIRE(fLoaded);
        
        /* Check that llpallowip was auto-configured */
        REQUIRE(config::mapMultiArgs.count("-llpallowip") > 0);
        REQUIRE(config::mapMultiArgs["-llpallowip"].size() > 0);
        REQUIRE(config::mapMultiArgs["-llpallowip"][0] == "127.0.0.1");
    }
    
    SECTION("LoadMiningConfig - missing pubkey")
    {
        /* Clear pubkey */
        config::mapArgs.erase("-miningpubkey");
        config::mapArgs.erase("-miningaddress");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        REQUIRE_FALSE(fLoaded);
    }
    
    /* Restore original config state */
    config::mapArgs = mapOriginalArgs;
    config::mapMultiArgs = mapOriginalMultiArgs;
}
