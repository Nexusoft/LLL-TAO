/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <string>

namespace LLP
{
    /** LoadMiningConfig
     *
     *  Simplified mining configuration loader for Stateless Miner architecture.
     *  
     *  Only checks if mining is enabled via the -mining flag.
     *  All authentication is now miner-driven using Falcon keys provided by NexusMiner.
     *  
     *  Reads configuration from:
     *  - Command-line arguments (via config::mapArgs)
     *  - nexus.conf file
     *  
     *  Validates:
     *  - mining enable flag (-mining=1)
     *  - llpallowip is configured (defaults to 127.0.0.1 if not set)
     *  
     *  @return true always (no validation required in stateless mode)
     *
     **/
    bool LoadMiningConfig();
}
