/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_HOSTS_H
#define NEXUS_LLP_INCLUDE_HOSTS_H

#include <string>
#include <vector>

namespace LLP
{
    class BaseAddress;


    /** DNS_Lookup
     *
     *  The DNS Lookup Routine to find the Nodes that are set as DNS seeds.
     *
     *  @param[in] DNS_Seed List of dns seed names to translate into addresses.
     *
     *  @return Returns the list of addresses that were successfully found from the lookup list.
     *
     **/
    std::vector<BaseAddress> DNS_Lookup(const std::vector<std::string> &DNS_Seed);


    /** LookupHost
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName The DNS name or numeric (i.e 192.168.0.1) name of the host to look up.
     *  @param[out] vAddr The list of solutions for the lookup address.
     *  @param[in] nMaxSolutions The maximum number of look up solutions (a lookup may have more than one solution).
     *  @param[in] fAllowLookup The flag for specifying numeric lookups vs. DNS lookups.
     *
     *  @return Returns true if lookup was successful, false otherwise.
     *
     **/
    bool LookupHost(const std::string &strName, std::vector<BaseAddress>& vAddr, uint32_t nMaxSolutions = 0,
                    bool fAllowLookup = true);


    /** LookupHostNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName The numeric (i.e 192.168.0.1) name of the host to look up.
     *  @param[out] vAddr The list of solutions for the lookup address.
     *  @param[in] nMaxSolutions The maximum number of look up solutions (a lookup may have more than one solution).
     *
     *  @return Returns true if lookup was successful, false otherwise.
     *
     **/
    bool LookupHostNumeric(const std::string &strName, std::vector<BaseAddress>& vAddr, uint32_t nMaxSolutions = 0);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName The DNS name or numeric (i.e 192.168.0.1) name of the host to look up.
     *  @param[out] addr The address to update if lookup was successful.
     *  @param[in] portDefault The port number.
     *  @param[in] fAllowLookup The flag for specifying numeric lookups vs. DNS lookups
     *
     *  @return Returns true if lookup was successful, false otherwise.
     *
     **/
    bool Lookup(const std::string &strName, BaseAddress& addr, uint16_t portDefault = 0, bool fAllowLookup = true);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName The DNS name or numeric (i.e 192.168.0.1) name of the host to look up.
     *  @param[out] vAddr The list of solutions for the lookup address.
     *  @param[in] portDefault The port number.
     *  @param[in] fAllowLookup The flag for specifying numeric lookups vs. DNS lookups
     *  @param[in] nMaxSolutions The maximum number of look up solutions (a lookup may have more than one solution).
     *
     *  @return Returns true if lookup was successful, false otherwise.
     *
     **/
    bool Lookup(const std::string &strName, std::vector<BaseAddress>& vAddr, uint16_t portDefault = 0, bool fAllowLookup = true,
                uint32_t nMaxSolutions = 0);


    /** LookupNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName The DNS name or numeric (i.e 192.168.0.1) name of the host to look up.
     *  @param[out] addr The address to update if lookup was successful.
     *  @param[in] portDefault The port number.
     *
     *  @return Returns true if lookup was successful, false otherwise.
     *
     **/
    bool LookupNumeric(const std::string &strName, BaseAddress& addr, uint16_t portDefault = 0);
}

#endif
