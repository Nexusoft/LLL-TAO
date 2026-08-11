/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/config.h>
#include <Util/include/args.h>
#include <unit/catch2/catch.hpp>
#include <fstream>
#include <cstdio>

TEST_CASE("Config file parsing tests", "[config]")
{
    // Create a temporary config file for testing
    const std::string tempConfigPath = "/tmp/test_nexus.conf";
    
    SECTION("Parse simple alphanumeric value")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password=Test123\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "Test123");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse value with hash in middle")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password=Test#123\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "Test#123");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse value with special characters")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password=Test!@$%^&*()\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "Test!@$%^&*()");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse double-quoted value with hash")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password=\"!@#$^&*()\"\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "!@#$^&*()");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse single-quoted value with hash")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password='Test#123'\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "Test#123");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse quoted value with spaces")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "password=\"My Pass 123\"\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "My Pass 123");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse value with inline comment")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "verbose=3 # this is my verbosity\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-verbose") > 0);
        REQUIRE(mapSettings["-verbose"] == "3");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Skip comment line")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "# password=ignored\n";
        out << "username=validuser\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") == 0);
        REQUIRE(mapSettings.count("-username") > 0);
        REQUIRE(mapSettings["-username"] == "validuser");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse multiple values including hash characters")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "username=FLUFFER\n";
        out << "password=!@#$^&*()\n";
        out << "pin=!@#$^&*()\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-username") > 0);
        REQUIRE(mapSettings["-username"] == "FLUFFER");
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "!@#$^&*()");
        REQUIRE(mapSettings.count("-pin") > 0);
        REQUIRE(mapSettings["-pin"] == "!@#$^&*()");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Trim whitespace from keys and values")
    {
        // Write test config
        std::ofstream out(tempConfigPath);
        out << "  password  = Test123  \n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "Test123");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Parse value starting with hash character")
    {
        // Write test config - hash at start of unquoted value should be preserved
        std::ofstream out(tempConfigPath);
        out << "hashtag=#trending\n";
        out.close();
        
        // Set up for reading
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-hashtag") > 0);
        REQUIRE(mapSettings["-hashtag"] == "#trending");
        
        // Clean up
        std::remove(tempConfigPath.c_str());
    }
}
