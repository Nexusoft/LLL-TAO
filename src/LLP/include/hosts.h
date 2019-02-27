/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_HOSTS_H
#define NEXUS_LLP_INCLUDE_HOSTS_H

#include <string>
#include <vector>

namespace LLP
{
    class BaseAddress;


    /** DNS_SeedNodes
     *
     *  These addresses are the first point of contact on the P2P network
     *  They are established and maintained by the owners of each domain.
     *
     **/
    extern const std::vector<std::string> DNS_SeedNodes;


    /** DNS_SeedNodes_Testnet
     *
     *  Testnet seed nodes.
     *
     **/
    extern const std::vector<std::string> DNS_SeedNodes_Testnet;


    /** DNS_Lookup
     *
     *  The DNS Lookup Routine to find the Nodes that are set as DNS seeds.
     *
     *  @param[in] DNS_Seed List of dns seed names to translate into addresses.
     *
     **/
    std::vector<BaseAddress> DNS_Lookup(const std::vector<std::string> &DNS_Seed);


    /** LookupHost
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName
     *  @param[out] vAddr
     *  @param[in] nMaxSolutions
     *  @param[in] fAllowLookup
     *
     **/
    bool LookupHost(const std::string &strName,
                    std::vector<BaseAddress>& vAddr,
                    uint32_t nMaxSolutions = 0,
                    bool fAllowLookup = true);


    /** LookupHostNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName
     *  @param[out] vAddr
     *  @param[in] nMaxSolutions
     *
     **/
    bool LookupHostNumeric(const std::string &strName,
                           std::vector<BaseAddress>& vAddr,
                           uint32_t nMaxSolutions = 0);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName
     *  @param[out] addr
     *  @param[in] portDefault
     *  @param[in] fAllowLookup
     *
     *  @return
     *
     **/
    bool Lookup(const std::string &strName,
                BaseAddress& addr,
                uint16_t portDefault = 0,
                bool fAllowLookup = true);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName
     *  @param[out] vAddr
     *  @param[in] portDefault
     *  @param[in] fAllowLookup
     *  @param[in] nMaxSolutions
     *
     **/
    bool Lookup(const std::string &strName,
                std::vector<BaseAddress>& vAddr,
                uint16_t portDefault = 0,
                bool fAllowLookup = true,
                uint32_t nMaxSolutions = 0);


    /** LookupNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] strName
     *  @param[out] addr
     *  @param[in] portDefault
     *
     **/
    bool LookupNumeric(const std::string &strName,
                       BaseAddress& addr,
                       uint16_t portDefault = 0);
}

#endif
