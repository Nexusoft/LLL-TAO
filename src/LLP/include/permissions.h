/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_PERMISSIONS_H
#define NEXUS_LLP_INCLUDE_PERMISSIONS_H

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
bool CheckPermissions(const std::string &strAddress, uint32_t nPort);


/** WildcardMatch
 *
 *  Performs a wildcard match string search given a search string and a mask.
 *
 *  @param[in] psz The string to find wildcard match in.
 *  @param[in] mask The wildcard string to match.
 *
 *  @return Returns true if match was found, false otherwise.
 *
 **/
bool WildcardMatch(const char* psz, const char* mask);


/** WildcardMatch
 *
 *  Performs a wildcard match string search given a search string and a mask.
 *
 *  @param[in] str The string to find wildcard match in.
 *  @param[in] mask The wildcard string to match.
 *
 *  @return Returns true if match was found, false otherwise.
 *
 **/
bool WildcardMatch(const std::string& str, const std::string& mask);


#endif
