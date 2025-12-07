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
    /* LoadMiningConfig
     *
     * Simplified mining configuration loader for Stateless Miner architecture.
     * Only checks if mining is enabled - all authentication is now miner-driven.
     */
    bool LoadMiningConfig()
    {
        debug::log(0, FUNCTION, "Loading mining configuration...");
        
        bool fMiningEnabled = config::GetBoolArg("-mining", false);
        
        if(!fMiningEnabled)
        {
            debug::log(0, FUNCTION, "Mining server disabled");
            return true;
        }
        
        debug::log(0, FUNCTION, "Mining server enabled");
        debug::log(0, FUNCTION, "Authentication: MINER-DRIVEN (Falcon keys from NexusMiner)");
        debug::log(0, FUNCTION, "Reward Routing: AUTO-CREDIT to Username:default");
        
        if(!config::HasArg("-llpallowip"))
        {
            config::mapMultiArgs["-llpallowip"].push_back("127.0.0.1");
        }
        
        return true;
    }
}
