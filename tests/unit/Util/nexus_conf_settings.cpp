/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/config.h>
#include <Util/include/args.h>
#include <LLP/include/port.h>
#include <unit/catch2/catch.hpp>
#include <fstream>
#include <cstdio>

TEST_CASE("nexus.conf mining settings validation", "[config][mining]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_mining.conf";
    
    SECTION("mining - valid boolean values")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "mining=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-mining") > 0);
        REQUIRE(mapSettings["-mining"] == "1");
        
        // Test GetBoolArg
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-mining", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("mining - disabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "mining=0\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-mining", false) == false);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("miningport - valid port number")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningport=9323\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-miningport") > 0);
        REQUIRE(mapSettings["-miningport"] == "9323");
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-miningport", 0) == 9323);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("miningport - default value")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        config::fTestNet = false;
        
        uint16_t port = LLP::GetMiningPort();
        REQUIRE(port == 9323);
    }
    
    SECTION("miningport - custom port")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningport=19323\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-miningport", 0) == 19323);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("legacyminingport - valid port number")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "legacyminingport=8323\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-legacyminingport") > 0);
        REQUIRE(mapSettings["-legacyminingport"] == "8323");
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-legacyminingport", 0) == 8323);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("legacyminingport - default value")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        config::fTestNet = false;
        
        uint16_t port = LLP::GetLegacyMiningPort();
        REQUIRE(port == 8323);
    }
    
    SECTION("legacyminingport - disabled (0)")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "legacyminingport=0\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-legacyminingport", 8323) == 0);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("miningthreads - valid number")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningthreads=8\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-miningthreads", 0) == 8);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("miningtimeout - valid timeout value")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningtimeout=60\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-miningtimeout", 30) == 60);
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf authentication settings validation", "[config][auth]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_auth.conf";
    
    SECTION("autologin - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "autologin=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-autologin", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("username - valid string")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "username=testuser\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-username") > 0);
        REQUIRE(mapSettings["-username"] == "testuser");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("password - with special characters")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "password=P@ssw0rd!#$%\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-password") > 0);
        REQUIRE(mapSettings["-password"] == "P@ssw0rd!#$%");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("pin - numeric string")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "pin=1234\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings.count("-pin") > 0);
        REQUIRE(mapSettings["-pin"] == "1234");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("falcon - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "falcon=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-falcon", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("apiuser and apipassword")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "apiuser=api_admin\n";
        out << "apipassword=SecureAPIPass123!\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings["-apiuser"] == "api_admin");
        REQUIRE(mapSettings["-apipassword"] == "SecureAPIPass123!");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("rpcuser and rpcpassword")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "rpcuser=rpc_admin\n";
        out << "rpcpassword=SecureRPCPass456!\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapSettings["-rpcuser"] == "rpc_admin");
        REQUIRE(mapSettings["-rpcpassword"] == "SecureRPCPass456!");
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf network settings validation", "[config][network]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_network.conf";
    
    SECTION("listen - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "listen=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-listen", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("maxconnections - valid number")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "maxconnections=99\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-maxconnections", 16) == 99);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("llpallowip - single IP")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "llpallowip=127.0.0.1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapMultiSettings.count("-llpallowip") > 0);
        REQUIRE(mapMultiSettings["-llpallowip"].size() == 1);
        REQUIRE(mapMultiSettings["-llpallowip"][0] == "127.0.0.1");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("llpallowip - multiple IPs")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "llpallowip=127.0.0.1\n";
        out << "llpallowip=192.168.1.0/24\n";
        out << "llpallowip=10.0.0.0/8\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        REQUIRE(mapMultiSettings["-llpallowip"].size() == 3);
        REQUIRE(mapMultiSettings["-llpallowip"][0] == "127.0.0.1");
        REQUIRE(mapMultiSettings["-llpallowip"][1] == "192.168.1.0/24");
        REQUIRE(mapMultiSettings["-llpallowip"][2] == "10.0.0.0/8");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("port - custom P2P port")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "port=19888\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-port", 0) == 19888);
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf server settings validation", "[config][server]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_server.conf";
    
    SECTION("server - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "server=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-server", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("daemon - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "daemon=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-daemon", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("rpcport - custom port")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "rpcport=19336\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-rpcport", 0) == 19336);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("apiport - custom port")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "apiport=18080\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-apiport", 0) == 18080);
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf logging settings validation", "[config][logging]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_logging.conf";
    
    SECTION("verbose - level 0")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "verbose=0\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-verbose", -1) == 0);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("verbose - level 3")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "verbose=3\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-verbose", -1) == 3);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("log - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "log=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetBoolArg("-log", false) == true);
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf performance settings validation", "[config][performance]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_performance.conf";
    
    SECTION("dbcache - valid size in MB")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "dbcache=512\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-dbcache", 0) == 512);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("threads - valid thread count")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "threads=8\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-threads", 0) == 8);
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf testnet settings validation", "[config][testnet]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_testnet.conf";
    
    SECTION("testnet - enabled")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "testnet=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        REQUIRE(config::GetArg("-testnet", 0) == 1);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("testnet - ports remain the same for mining")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        config::fTestNet = true;
        
        // Mining ports should be the same on testnet
        uint16_t miningPort = LLP::GetMiningPort();
        uint16_t legacyPort = LLP::GetLegacyMiningPort();
        
        REQUIRE(miningPort == 9323);
        REQUIRE(legacyPort == 8323);
        
        config::fTestNet = false;
    }
}

TEST_CASE("nexus.conf complete mining pool configuration", "[config][integration]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_complete.conf";
    
    SECTION("Complete minimal mining pool config")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "# Autologin\n";
        out << "autologin=1\n";
        out << "username=pooladmin\n";
        out << "password=SecurePass123!\n";
        out << "pin=1234\n";
        out << "\n";
        out << "# API Auth\n";
        out << "apiuser=apiuser\n";
        out << "apipassword=ApiPass456!\n";
        out << "\n";
        out << "# Servers\n";
        out << "server=1\n";
        out << "\n";
        out << "# Mining\n";
        out << "mining=1\n";
        out << "miningport=9323\n";
        out << "legacyminingport=8323\n";
        out << "\n";
        out << "# Network\n";
        out << "listen=1\n";
        out << "maxconnections=99\n";
        out << "llpallowip=127.0.0.1\n";
        out << "\n";
        out << "# Falcon\n";
        out << "falcon=1\n";
        out << "\n";
        out << "# Logging\n";
        out << "verbose=2\n";
        out << "log=1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        // Verify all settings were parsed correctly
        REQUIRE(mapSettings["-autologin"] == "1");
        REQUIRE(mapSettings["-username"] == "pooladmin");
        REQUIRE(mapSettings["-password"] == "SecurePass123!");
        REQUIRE(mapSettings["-pin"] == "1234");
        REQUIRE(mapSettings["-apiuser"] == "apiuser");
        REQUIRE(mapSettings["-apipassword"] == "ApiPass456!");
        REQUIRE(mapSettings["-server"] == "1");
        REQUIRE(mapSettings["-mining"] == "1");
        REQUIRE(mapSettings["-miningport"] == "9323");
        REQUIRE(mapSettings["-legacyminingport"] == "8323");
        REQUIRE(mapSettings["-listen"] == "1");
        REQUIRE(mapSettings["-maxconnections"] == "99");
        REQUIRE(mapSettings["-falcon"] == "1");
        REQUIRE(mapSettings["-verbose"] == "2");
        REQUIRE(mapSettings["-log"] == "1");
        REQUIRE(mapMultiSettings["-llpallowip"][0] == "127.0.0.1");
        
        std::remove(tempConfigPath.c_str());
    }
}

TEST_CASE("nexus.conf error handling", "[config][errors]")
{
    const std::string tempConfigPath = "/tmp/test_nexus_errors.conf";
    
    SECTION("Invalid port - negative number")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningport=-1\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        int64_t port = config::GetArg("-miningport", 9323);
        
        // GetArg returns the parsed value, even if negative
        // Application logic should validate port ranges
        REQUIRE(port == -1);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Invalid port - too large")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "miningport=99999\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        config::mapArgs = mapSettings;
        int64_t port = config::GetArg("-miningport", 9323);
        
        // GetArg returns the parsed value
        // Application should validate port is <= 65535
        REQUIRE(port == 99999);
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Invalid boolean - non-numeric")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "mining=yes\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        // Non-numeric values are stored as strings
        REQUIRE(mapSettings["-mining"] == "yes");
        
        // GetBoolArg treats non-zero/non-empty as true
        config::mapArgs = mapSettings;
        bool mining = config::GetBoolArg("-mining", false);
        REQUIRE(mining == true);  // "yes" is truthy
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Empty value")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "username=\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        // Empty values are stored as empty strings
        REQUIRE(mapSettings.count("-username") > 0);
        REQUIRE(mapSettings["-username"] == "");
        
        std::remove(tempConfigPath.c_str());
    }
    
    SECTION("Missing equals sign - invalid line")
    {
        config::mapArgs.clear();
        config::mapMultiArgs.clear();
        std::ofstream out(tempConfigPath);
        out << "invalidline\n";
        out << "validkey=validvalue\n";
        out.close();
        
        config::mapArgs["-conf"] = tempConfigPath;
        config::mapArgs["-datadir"] = "/tmp/";
        
        std::map<std::string, std::string> mapSettings;
        std::map<std::string, std::vector<std::string>> mapMultiSettings;
        
        config::ReadConfigFile(mapSettings, mapMultiSettings);
        
        // Invalid lines are skipped, valid ones are parsed
        REQUIRE(mapSettings.count("-invalidline") == 0);
        REQUIRE(mapSettings.count("-validkey") > 0);
        REQUIRE(mapSettings["-validkey"] == "validvalue");
        
        std::remove(tempConfigPath.c_str());
    }
}
