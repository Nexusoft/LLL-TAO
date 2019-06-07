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
#include <cstring> //memset

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
        for(int nSeed = 0; nSeed < DNS_Seed.size(); ++nSeed)
        {
            debug::log(0, nSeed, " Host: ",  DNS_Seed[nSeed]);
            if(LookupHost(DNS_Seed[nSeed].c_str(), vNodes))
            {
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
    bool static LookupIntern(const std::string &strName,
                             std::vector<BaseAddress> &vAddr,
                             uint32_t nMaxSolutions,
                             bool fAllowLookup)
    {
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        vAddr.clear();
        struct addrinfo aiHint;
        memset(&aiHint, 0, sizeof(struct addrinfo));

        aiHint.ai_socktype = SOCK_STREAM;
        aiHint.ai_protocol = IPPROTO_TCP;

        aiHint.ai_family = AF_UNSPEC;
        aiHint.ai_flags = AI_ADDRCONFIG | (fAllowLookup ? 0 : AI_NUMERICHOST);

        struct addrinfo *aiRes = nullptr;
        std::unique_lock<std::mutex> lk(::LOOKUP_MUTEX);
        //{
          if(getaddrinfo(strName.c_str(), nullptr, &aiHint, &aiRes) != 0)
              return false;
        //}
        //lk.unlock();

        struct addrinfo *aiTrav = aiRes;

        /* Address info traversal */
        while(aiTrav != nullptr && (nMaxSolutions == 0 || vAddr.size() < nMaxSolutions))
        {
            if(aiTrav->ai_family == AF_INET)
            {
                if(aiTrav->ai_addrlen < sizeof(sockaddr_in))
                {
                    debug::error(FUNCTION, "invalid ai_addrlen: < sizeof(sockaddr_in)");
                    aiTrav = aiTrav->ai_next;
                    continue;
                }
                vAddr.push_back(BaseAddress(((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr));
            }

            if(aiTrav->ai_family == AF_INET6)
            {
                if(aiTrav->ai_addrlen < sizeof(sockaddr_in6))
                {
                    debug::error(FUNCTION, "invalid ai_addrlen: < sizeof(sockaddr_in6)");
                    aiTrav = aiTrav->ai_next;
                    continue;
                }


                vAddr.push_back(BaseAddress(((struct sockaddr_in6*)(aiTrav->ai_addr))->sin6_addr));
            }

            aiTrav = aiTrav->ai_next;
        }

        //lk.lock();
        //{
            freeaddrinfo(aiRes);
        //}
        //lk.unlock();

        return (vAddr.size() > 0);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupHost(const std::string &strName,
                    std::vector<BaseAddress>& vAddr,
                    uint32_t nMaxSolutions,
                    bool fAllowLookup)
    {
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
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");


        return LookupHost(strName, vAddr, nMaxSolutions, false);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool Lookup(const std::string &strName,
                std::vector<BaseAddress>& vAddr,
                uint16_t portDefault,
                bool fAllowLookup,
                uint32_t nMaxSolutions)
    {

        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");


        if(!LookupIntern(strName, vAddr, nMaxSolutions, fAllowLookup))
            return false;

        /* Set the ports to the default port. */
        for(uint32_t i = 0; i < vAddr.size(); ++i)
            vAddr[i].SetPort(portDefault);

        return true;
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool Lookup(const std::string &strName,
                BaseAddress &addr,
                uint16_t portDefault,
                bool fAllowLookup)
    {
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        std::vector<BaseAddress> vAddr;

        if(!Lookup(strName, vAddr, portDefault, fAllowLookup, 1))
            return false;

        addr = vAddr[0];
        return true;
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupNumeric(const std::string &strName,
                       BaseAddress& addr,
                       uint16_t portDefault)
    {
        size_t s = strName.size();
        if(s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        return Lookup(strName, addr, portDefault, false);
    }
}
