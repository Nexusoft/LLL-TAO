/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

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
    static std::vector<std::string> DNS_SeedNodes =
    {
        "node1.nexusearth.com",
        "node1.mercuryminer.com",
        "node1.nexusminingpool.com",
        "node1.nexus2.space",
        "node1.barbequemedia.com",
        "node1.nxsorbitalscan.com",
        "node1.nxs.efficienthash.com",
        "node1.henryskinner.net",
        "node2.nexusearth.com",
        "node2.mercuryminer.com",
        "node2.nexusminingpool.com",
        "node2.nexus2.space",
        "node2.barbequemeportParseddia.com",
        "node2.nxsorbitalscan.com",
        "node2.nxs.efficienthash.com",
        "node2.henryskinner.net",
        "node3.nexusearth.com",
        "node3.mercuryminer.com",
        "node3.nexusminingpool.com",
        "node3.nexus2.space",
        "node3.barbequemedia.com",
        "node3.nxsorbitalscan.com",
        "node3.nxs.efficienthash.com",
        "node3.henryskinner.net",
        "node4.nexusearth.com",
        "node4.mercuryminer.com",
        "node4.nexus2.space",
        "node4.barbequemedia.com",
        "node4.nxsorbitalscan.com",
        "node4.nxs.efficienthash.com",
        "node4.henryskinner.net",
        "node5.nexusearth.com",
        "node5.mercuryminer.com",
        "node5.barbequemedia.com",
        "node5.nxs.efficienthash.com",
        "node5.henryskinner.net",
        "node6.nexusearth.com",
        "node6.mercuryminer.com",
        "node6.barbequemedia.com",
        "node6.nxs.efficienthash.com",
        "node6.henryskinner.net",
        "node7.nexusearth.com",
        "node7.mercuryminer.com",
        "node7.barbequemedia.com",
        "node7.nxs.efficienthash.com",
        "node7.henryskinner.net",
        "node8.nexusearth.com",
        "node8.mercuryminer.com",
        "node8.barbequemedia.com",
        "node8.nxs.efficienthash.com",
        "node8.henryskinner.net",
        "node9.nexusearth.com",
        "node9.mercuryminer.com",
        "node9.nxs.efficienthash.com",
        "node10.nexusearth.com",
        "node10.mercuryminer.com",
        "node10.nxs.efficienthash.com",
        "node11.nexusearth.com",
        "node11.mercuryminer.com",
        "node11.nxs.efficienthash.com",
        "node12.nexusearth.com",
        "node12.mercuryminer.com",
        "node12.nxs.efficienthash.com",
        "node13.nexusearth.com",
        "node13.mercuryminer.com",
        "node13.nxs.efficienthash.com",
        "node14.mercuryminer.com",
        "node15.mercuryminer.com",
        "node16.mercuryminer.com",
        "node17.mercuryminer.com",
        "node18.mercuryminer.com",
        "node19.mercuryminer.com",
        "node20.mercuryminer.com",
        "node21.mercuryminer.com"
    };


    /** DNS_SeedNodes_Testnet
     *
     *  Testnet seed nodes.
     *
     **/
    static std::vector<std::string> DNS_SeedNodes_Testnet =
    {
        "test1.nexusoft.io"
    };


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
     *   Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     **/
    bool LookupHost(const char *pszName,
                    std::vector<BaseAddress>& vAddr,
                    uint32_t nMaxSolutions = 0,
                    bool fAllowLookup = true);


    /** LookupHostNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] pszName
     *  @param[out] vAddr
     *  @param[in] nMaxSolutions
     *
     **/
    bool LookupHostNumeric(const char *pszName,
                           std::vector<BaseAddress>& vAddr,
                           uint32_t nMaxSolutions = 0);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] pszName
     *  @param[out] addr
     *  @param[in] portDefault
     *  @param[in] fAllowLookup
     *
     *  @return
     *
     **/
    bool Lookup(const char *pszName,
                BaseAddress& addr,
                uint16_t portDefault = 0,
                bool fAllowLookup = true);


    /** Lookup
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] pszName
     *  @param[out] vAddr
     *  @param[in] portDefault
     *  @param[in] fAllowLookup
     *  @param[in] nMaxSolutions
     *
     **/
    bool Lookup(const char *pszName,
                std::vector<BaseAddress>& vAddr,
                uint16_t portDefault = 0,
                bool fAllowLookup = true,
                uint32_t nMaxSolutions = 0);


    /** LookupNumeric
     *
     *  Standard Wrapper Function to Interact with cstdlib DNS functions.
     *
     *  @param[in] pszName
     *  @param[out] addr
     *  @param[in] portDefault
     *
     **/
    bool LookupNumeric(const char *pszName,
                       BaseAddress& addr,
                       uint16_t portDefault = 0);
}

#endif
