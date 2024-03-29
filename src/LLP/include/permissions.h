/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>


/** CheckPermissions
 *
 *  IP Filtering Definitions. IP's are Filtered By Ports.
 *
 *  @param[in] strAddress the IP address to check.
 *  @param[in] nPort The port number to check.
 *
 *  @return Returns true if address is permissable, false otherwise.
 *
 **/
bool CheckPermissions(const std::string &strAddress, const uint16_t nPort);
