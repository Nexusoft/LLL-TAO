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

/* Test the simplified mining configuration for Stateless Miner architecture.
 * In this architecture, all authentication is miner-driven using Falcon keys.
 * Node configuration only requires mining=1 to enable the mining server.
 */

TEST_CASE("Simplified Mining Configuration Tests", "[miner][config]")
{
    /* Save original config state to restore after tests */
    std::map<std::string, std::string> mapOriginalArgs = config::mapArgs;
    std::map<std::string, std::vector<std::string>> mapOriginalMultiArgs = config::mapMultiArgs;
    
    SECTION("LoadMiningConfig - mining enabled")
    {
        /* Set mining enabled */
        config::mapArgs["-mining"] = "1";
        config::mapMultiArgs.erase("-llpallowip");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        REQUIRE(fLoaded);
        
        /* Check that llpallowip was auto-configured to localhost */
        REQUIRE(config::mapMultiArgs.count("-llpallowip") > 0);
        REQUIRE(config::mapMultiArgs["-llpallowip"].size() > 0);
        REQUIRE(config::mapMultiArgs["-llpallowip"][0] == "127.0.0.1");
    }
    
    SECTION("LoadMiningConfig - mining disabled")
    {
        /* Mining disabled (or not set) */
        config::mapArgs.erase("-mining");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        /* Should still succeed - mining is just disabled */
        REQUIRE(fLoaded);
    }
    
    SECTION("LoadMiningConfig - with custom llpallowip")
    {
        /* Set mining enabled with custom IP allow list */
        config::mapArgs["-mining"] = "1";
        config::mapMultiArgs["-llpallowip"].clear();
        config::mapMultiArgs["-llpallowip"].push_back("192.168.1.1");
        config::mapMultiArgs["-llpallowip"].push_back("10.0.0.1");
        
        bool fLoaded = LLP::LoadMiningConfig();
        
        REQUIRE(fLoaded);
        
        /* Custom IPs should be preserved */
        REQUIRE(config::mapMultiArgs["-llpallowip"].size() == 2);
    }
    
    /* Restore original config state */
    config::mapArgs = mapOriginalArgs;
    config::mapMultiArgs = mapOriginalMultiArgs;
}
