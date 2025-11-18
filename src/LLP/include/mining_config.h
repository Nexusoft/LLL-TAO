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
     *  Auto-configuration helper for mining that reads and validates required configuration.
     *  
     *  Reads configuration from:
     *  - Command-line arguments (via config::mapArgs)
     *  - nexus.conf file
     *  
     *  Validates:
     *  - miningpubkey or miningaddress is present
     *  - llpallowip is configured (defaults to 127.0.0.1 if not set)
     *  - mining enable flag if relevant
     *  
     *  @return true if configuration is valid and complete, false otherwise
     *
     **/
    bool LoadMiningConfig();


    /** ValidateMiningPubkey
     *
     *  Validates that a mining public key is configured.
     *  
     *  Checks for -miningpubkey or -miningaddress arguments.
     *  
     *  @param[out] strError Error message if validation fails
     *  
     *  @return true if valid pubkey found, false otherwise
     *
     **/
    bool ValidateMiningPubkey(std::string& strError);


    /** GetMiningPubkey
     *
     *  Retrieves the configured mining public key.
     *  
     *  @param[out] strPubkey The public key string
     *  
     *  @return true if pubkey found, false otherwise
     *
     **/
    bool GetMiningPubkey(std::string& strPubkey);
}
