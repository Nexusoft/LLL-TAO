/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/mining_config.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>

#include <string>
#include <vector>

namespace LLP
{
    /* ValidateMiningPubkey
     *
     * Validates that a mining public key is configured.
     */
    bool ValidateMiningPubkey(std::string& strError)
    {
        /* Check for miningpubkey argument */
        if(config::HasArg("-miningpubkey"))
        {
            std::string strPubkey = config::GetArg("-miningpubkey", "");
            if(strPubkey.empty())
            {
                strError = "miningpubkey argument is empty";
                return false;
            }
            
            /* Basic validation - should be non-empty hex string */
            if(strPubkey.length() < 10)
            {
                strError = "miningpubkey appears to be too short";
                return false;
            }
            
            return true;
        }
        
        /* Check for miningaddress argument as alternative */
        if(config::HasArg("-miningaddress"))
        {
            std::string strAddress = config::GetArg("-miningaddress", "");
            if(strAddress.empty())
            {
                strError = "miningaddress argument is empty";
                return false;
            }
            
            /* Basic validation - should be non-empty */
            if(strAddress.length() < 10)
            {
                strError = "miningaddress appears to be too short";
                return false;
            }
            
            return true;
        }
        
        /* Neither miningpubkey nor miningaddress found */
        strError = "Missing mining configuration: add -miningpubkey=YOUR_PUBKEY or -miningaddress=YOUR_ADDRESS to nexus.conf";
        return false;
    }


    /* GetMiningPubkey
     *
     * Retrieves the configured mining public key.
     */
    bool GetMiningPubkey(std::string& strPubkey)
    {
        /* Check for miningpubkey first */
        if(config::HasArg("-miningpubkey"))
        {
            strPubkey = config::GetArg("-miningpubkey", "");
            return !strPubkey.empty();
        }
        
        /* Fall back to miningaddress */
        if(config::HasArg("-miningaddress"))
        {
            strPubkey = config::GetArg("-miningaddress", "");
            return !strPubkey.empty();
        }
        
        return false;
    }


    /* LoadMiningConfig
     *
     * Auto-configuration helper for mining that reads and validates required configuration.
     */
    bool LoadMiningConfig()
    {
        /* Log where we're loading config from */
        debug::log(0, FUNCTION, "Loading mining configuration from command-line args and nexus.conf");
        
        /* Validate mining pubkey is present */
        std::string strError;
        if(!ValidateMiningPubkey(strError))
        {
            debug::error(FUNCTION, "Mining configuration validation failed: ", strError);
            debug::error(FUNCTION, "Add miningpubkey=YOUR_PUBKEY to nexus.conf or use -miningpubkey command-line argument");
            return false;
        }
        
        /* Get and log the mining pubkey (first 20 chars for security) */
        std::string strPubkey;
        if(GetMiningPubkey(strPubkey))
        {
            std::string strDisplay = strPubkey.substr(0, std::min((size_t)20, strPubkey.length())) + "...";
            debug::log(0, FUNCTION, "Mining pubkey configured: ", strDisplay);
        }
        
        /* Check llpallowip configuration */
        if(!config::HasArg("-llpallowip"))
        {
            /* Default to localhost if not configured */
            debug::log(0, FUNCTION, "No -llpallowip configured, defaulting to 127.0.0.1 (localhost only)");
            config::mapMultiArgs["-llpallowip"].push_back("127.0.0.1");
        }
        else
        {
            /* Log configured IPs */
            const auto& vIPs = config::mapMultiArgs["-llpallowip"];
            debug::log(0, FUNCTION, "Mining LLP allowed IPs configured: ", vIPs.size(), " entries");
            for(const auto& strIP : vIPs)
            {
                debug::log(1, FUNCTION, "  Allowed IP: ", strIP);
            }
        }
        
        /* Check if mining is enabled */
        bool fMiningEnabled = config::GetBoolArg("-mining", false);
        if(fMiningEnabled)
        {
            debug::log(0, FUNCTION, "Mining is enabled via -mining flag");
        }
        else
        {
            debug::log(0, FUNCTION, "Mining is not enabled (use -mining=1 to enable at startup)");
        }
        
        debug::log(0, FUNCTION, "Mining configuration loaded successfully");
        return true;
    }
}
