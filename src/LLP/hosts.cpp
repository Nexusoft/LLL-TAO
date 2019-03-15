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

    /*  These addresses are the first point of contact on the P2P network
     *  They are established and maintained by the owners of each domain. */
    const std::vector<std::string> DNS_SeedNodes =
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


    /*  Testnet seed nodes. */
    const std::vector<std::string> DNS_SeedNodes_Testnet =
    {
        "test1.nexusoft.io",
        "lisptest1.mercuryminer.com",
        "lisptest2.mercuryminer.com",
        "lisptest3.mercuryminer.com",
        "lisptest4.mercuryminer.com",
        "lisptest5.mercuryminer.com",
        "testlisp.nexusminingpool.com",
        "nexus-lisp-seed.lispers.net",
        "fe00::255:255"
        "test1.mercuryminer.com",
        "test2.mercuryminer.com",
        "test3.mercuryminer.com",
        "test4.mercuryminer.com",
        "test5.mercuryminer.com",
        "test.nexusminingpool.com",
    };

    /** The DNS Lookup Routine to find the Nodes that are set as DNS seeds. **/
    std::vector<BaseAddress> DNS_Lookup(const std::vector<std::string> &DNS_Seed)
    {
        std::vector<BaseAddress> vNodes;
        for (int nSeed = 0; nSeed < DNS_Seed.size(); ++nSeed)
        {
            debug::log(0, nSeed, " Host: ",  DNS_Seed[nSeed]);
            if (LookupHost(DNS_Seed[nSeed].c_str(), vNodes))
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
        if (s == 0 || s > 255)
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
        while (aiTrav != nullptr && (nMaxSolutions == 0 || vAddr.size() < nMaxSolutions))
        {
            if (aiTrav->ai_family == AF_INET)
            {
                if(aiTrav->ai_addrlen < sizeof(sockaddr_in))
                {
                    debug::error(FUNCTION, "invalid ai_addrlen: < sizeof(sockaddr_in)");
                    aiTrav = aiTrav->ai_next;
                    continue;
                }
                vAddr.push_back(BaseAddress(((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr));
            }

            if (aiTrav->ai_family == AF_INET6)
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
        if (s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        return LookupIntern(strName, vAddr, nMaxSolutions, fAllowLookup);
    }


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupHostNumeric(const std::string &strName,
                           std::vector<BaseAddress>& vAddr,
                           uint32_t nMaxSolutions)
    {
        size_t s = strName.size();
        if (s == 0 || s > 255)
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
        if (s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");


        if(!LookupIntern(strName, vAddr, nMaxSolutions, fAllowLookup))
            return false;

        /* Set the ports to the default port. */
        for (uint32_t i = 0; i < vAddr.size(); ++i)
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
        if (s == 0 || s > 255)
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
        if (s == 0 || s > 255)
            return debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");

        return Lookup(strName, addr, portDefault, false);
    }
}
