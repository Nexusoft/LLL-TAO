/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/network.h>
#include <LLP/include/hosts.h>

#include <vector>

#include <Util/include/strlcpy.h>

namespace LLP
{

    /** DNS Query of Domain Names Associated with Seed Nodes **/
    std::vector<CAddress> DNS_Lookup(std::vector<std::string> DNS_Seed)
    {
        std::vector<CAddress> vNodes;
        for (int nSeed = 0; nSeed < DNS_Seed.size(); nSeed ++ )
        {
            debug::log(0, "%u Host: %s", nSeed, DNS_Seed[nSeed].c_str());
            std::vector<LLP::CNetAddr> vaddr;
            if (LookupHost(DNS_Seed[nSeed].c_str(), vaddr))
            {
                for(CNetAddr& ip : vaddr)
                {
                    CAddress addr = CAddress(CService(ip, GetDefaultPort()));
                    vNodes.push_back(addr);

                    debug::log(0, "DNS Seed: %s", addr.ToStringIP().c_str());
                }
            }
        }

        return vNodes;
    }


    bool static LookupIntern(const char *pszName, std::vector<CNetAddr>& vIP, uint32_t nMaxSolutions, bool fAllowLookup)
    {
        vIP.clear();
        struct addrinfo aiHint;
        memset(&aiHint, 0, sizeof(struct addrinfo));

        aiHint.ai_socktype = SOCK_STREAM;
        aiHint.ai_protocol = IPPROTO_TCP;
    #ifdef WIN32ßßßß
        aiHint.ai_family = AF_UNSPEC;
        aiHint.ai_flags = fAllowLookup ? 0 : AI_NUMERICHOST;
    #else
        aiHint.ai_family = AF_UNSPEC;
        aiHint.ai_flags = AI_ADDRCONFIG | (fAllowLookup ? 0 : AI_NUMERICHOST);
    #endif
        struct addrinfo *aiRes = nullptr;
        int nErr = getaddrinfo(pszName, nullptr, &aiHint, &aiRes);
        if (nErr)
            return false;

        struct addrinfo *aiTrav = aiRes;
        while (aiTrav != nullptr && (nMaxSolutions == 0 || vIP.size() < nMaxSolutions))
        {
            if (aiTrav->ai_family == AF_INET)
            {
                assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in));
                vIP.push_back(CNetAddr(((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr));
            }

            if (aiTrav->ai_family == AF_INET6)
            {
                assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in6));
                vIP.push_back(CNetAddr(((struct sockaddr_in6*)(aiTrav->ai_addr))->sin6_addr));
            }

            aiTrav = aiTrav->ai_next;
        }

        freeaddrinfo(aiRes);

        return (vIP.size() > 0);
    }

    bool LookupHost(const char *pszName, std::vector<CNetAddr>& vIP, uint32_t nMaxSolutions, bool fAllowLookup)
    {
        if (pszName[0] == 0)
            return false;
        char psz[256];
        char *pszHost = psz;
        strlcpy(psz, pszName, sizeof(psz));
        if (psz[0] == '[' && psz[strlen(psz)-1] == ']')
        {
            pszHost = psz+1;
            psz[strlen(psz)-1] = 0;
        }

        return LookupIntern(pszHost, vIP, nMaxSolutions, fAllowLookup);
    }

    bool LookupHostNumeric(const char *pszName, std::vector<CNetAddr>& vIP, uint32_t nMaxSolutions)
    {
        return LookupHost(pszName, vIP, nMaxSolutions, false);
    }

    bool Lookup(const char *pszName, std::vector<CService>& vAddr, int portDefault, bool fAllowLookup, uint32_t nMaxSolutions)
    {
        if (pszName[0] == 0)
            return false;
        int port = portDefault;
        char psz[256];
        char *pszHost = psz;
        strlcpy(psz, pszName, sizeof(psz));
        char* pszColon = strrchr(psz+1,':');
        char *pszPortEnd = nullptr;
        int portParsed = pszColon ? strtoul(pszColon+1, &pszPortEnd, 10) : 0;
        if (pszColon && pszPortEnd && pszPortEnd[0] == 0)
        {
            if (psz[0] == '[' && pszColon[-1] == ']')
            {
                pszHost = psz+1;
                pszColon[-1] = 0;
            }
            else
                pszColon[0] = 0;
            if (port >= 0 && port <= USHRT_MAX)
                port = portParsed;
        }
        else
        {
            if (psz[0] == '[' && psz[strlen(psz)-1] == ']')
            {
                pszHost = psz+1;
                psz[strlen(psz)-1] = 0;
            }

        }

        std::vector<CNetAddr> vIP;
        bool fRet = LookupIntern(pszHost, vIP, nMaxSolutions, fAllowLookup);
        if (!fRet)
            return false;
        vAddr.resize(vIP.size());
        for (uint32_t i = 0; i < vIP.size(); i++)
            vAddr[i] = CService(vIP[i], port);
        return true;
    }

    bool Lookup(const char *pszName, CService& addr, int portDefault, bool fAllowLookup)
    {
        std::vector<CService> vService;
        bool fRet = Lookup(pszName, vService, portDefault, fAllowLookup, 1);
        if (!fRet)
            return false;
        addr = vService[0];
        return true;
    }

    bool LookupNumeric(const char *pszName, CService& addr, int portDefault)
    {
        return Lookup(pszName, addr, portDefault, false);
    }
}
