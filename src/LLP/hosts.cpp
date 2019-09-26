/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/port.h>
#include <LLP/include/base_address.h>
#include <LLP/include/hosts.h>
#include <Util/include/debug.h>

#include <vector>
#include <mutex>

/* The lookup mutex for thread safe calls to getaddrinfo and freeaddrinfo. */
namespace
{
    std::mutex LOOKUP_MUTEX;
}

namespace LLP
{

    /** The DNS Lookup Routine to find the Nodes that are set as DNS seeds. **/
    std::vector<BaseAddress> DNS_Lookup(const std::vector<std::string> &DNS_Seed)
    {
        std::vector<BaseAddress> vNodes;

        /* Loop through the list of DNS seeds and look them up. */
        uint32_t nSeeds = DNS_Seed.size();
        for(uint32_t nSeed = 0; nSeed < nSeeds; ++nSeed)
        {
            debug::log(0, nSeed, " Host: ",  DNS_Seed[nSeed]);
            if(LookupHost(DNS_Seed[nSeed].c_str(), vNodes))
            {
                /* Set the port for each lookup address to the default port. */
                for(BaseAddress& ip : vNodes)
                {
                    ip.SetPort(GetDefaultPort());
                    debug::log(0, "DNS Seed: ", ip.ToStringIP());
                }
            }
        }

        return vNodes;
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupIntern(const std::string &strName, std::vector<BaseAddress> &vAddr, uint32_t nMaxSolutions, bool fAllowLookup)
    {
        /* Do a bounds check on the lookup name. */
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        /* Ensure the address list is empty before lookup. */
        vAddr.clear();

        /* Fill out the addrinfo struct with lookup information. */
        struct addrinfo aiHint;
        aiHint.ai_flags = AI_ADDRCONFIG | (fAllowLookup ? 0 : AI_NUMERICHOST);
        aiHint.ai_family = AF_UNSPEC;
        aiHint.ai_socktype = SOCK_STREAM;
        aiHint.ai_protocol = IPPROTO_TCP;
        aiHint.ai_addrlen = socklen_t(0);
        aiHint.ai_addr = 0;
        aiHint.ai_canonname = 0;
        aiHint.ai_next = 0;

        /* This will contain the list of results after the lookup is performed. */
        struct addrinfo *aiRes = nullptr;

        /* Lock the lookup mutex. */
        std::unique_lock<std::mutex> lk(::LOOKUP_MUTEX);

        /* Attempt to obtain address info for the lookup address. */
        if(getaddrinfo(strName.c_str(), nullptr, &aiHint, &aiRes) != 0)
            return false;

        /* Loop through the list of address info */
        struct addrinfo *aiNext = aiRes;
        while(aiNext != nullptr && (nMaxSolutions == 0 || vAddr.size() < nMaxSolutions))
        {
            /* Check if it is a IPv4 address. */
            if(aiNext->ai_family == AF_INET)
            {
                /* Check for address length consistency. */
                if(aiNext->ai_addrlen < sizeof(sockaddr_in))
                {
                    debug::error(FUNCTION, "invalid ai_addrlen: < sizeof(sockaddr_in)");
                    aiNext = aiNext->ai_next;
                    continue;
                }

                /* Add the address to the list of addresses. */
                vAddr.push_back(BaseAddress(((struct sockaddr_in*)(aiNext->ai_addr))->sin_addr));
            }
            /* Check if it is a IPv6 address. */
            else if(aiNext->ai_family == AF_INET6)
            {
                /* Check for address length consistency. */
                if(aiNext->ai_addrlen < sizeof(sockaddr_in6))
                {
                    debug::error(FUNCTION, "invalid ai_addrlen: < sizeof(sockaddr_in6)");
                    aiNext = aiNext->ai_next;
                    continue;
                }

                /* Add the address to the list of addresses. */
                vAddr.push_back(BaseAddress(((struct sockaddr_in6*)(aiNext->ai_addr))->sin6_addr));
            }

            /* Set the next pointer. */
            aiNext = aiNext->ai_next;
        }

        /* Free the memory associated with the address info. */
        freeaddrinfo(aiRes);

        /* If there are any addresses added, return true. */
        return (vAddr.size() > 0);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupHost(const std::string &strName, std::vector<BaseAddress>& vAddr, uint32_t nMaxSolutions, bool fAllowLookup)
    {
        /* Do a bounds check on the lookup name. */
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        return LookupIntern(strName, vAddr, nMaxSolutions, fAllowLookup);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupHostNumeric(const std::string &strName,
                           std::vector<BaseAddress>& vAddr,
                           uint32_t nMaxSolutions)
    {
        /* Do a bounds check on the lookup name. */
        uint32_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        /* Lookup the host. */
        return LookupHost(strName, vAddr, nMaxSolutions, false);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool Lookup(const std::string &strName, std::vector<BaseAddress>& vAddr, uint16_t portDefault, bool fAllowLookup,
                uint32_t nMaxSolutions)
    {
        /* Do a bounds check on the lookup name. */
        uint32_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        /* Lookup the host. */
        if(!LookupIntern(strName, vAddr, nMaxSolutions, fAllowLookup))
            return false;

        /* Set the ports to the default port. */
        for(uint32_t i = 0; i < vAddr.size(); ++i)
            vAddr[i].SetPort(portDefault);

        return true;
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool Lookup(const std::string &strName, BaseAddress &addr, uint16_t portDefault, bool fAllowLookup)
    {
        /* Do a bounds check on the lookup name. */
        uint32_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        /* Lookup the host with a max solution of 1. */
        std::vector<BaseAddress> vAddr;
        if(!Lookup(strName, vAddr, portDefault, fAllowLookup, 1))
            return false;

        /* If the lookup was successful, assign the output to the only item in the list. */
        addr = vAddr[0];
        return true;
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupNumeric(const std::string &strName, BaseAddress& addr, uint16_t portDefault)
    {
        /* Do a bounds check on the lookup name. */
        uint32_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        /* If the lookup was successful, assign the output to the only item in the list. */
        return Lookup(strName, addr, portDefault, false);
    }
}
