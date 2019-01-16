/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_SERVICE_H
#define NEXUS_LLP_INCLUDE_SERVICE_H

#include <LLP/include/netaddr.h>
#include <Util/templates/serialize.h>
#include <string>
#include <vector>
#include <cstdint>

namespace LLP
{
    /** A combination of a network address (NetAddr) and a (TCP) port */
    class Service : public NetAddr
    {
    protected:
        uint16_t nPort; // host order

    public:
        Service();
        Service(const NetAddr& ip, uint16_t port);
        Service(NetAddr& ip, uint16_t port);
        Service(const struct in_addr& ipv4Addr, uint16_t port);
        Service(const struct in6_addr& ipv6Addr, uint16_t port);
        Service(const struct sockaddr_in& addr);
        Service(const struct sockaddr_in6& addr);
        Service(const char *pszIpPort, uint16_t portDefault, bool fAllowLookup = false);
        Service(const char *pszIpPort, bool fAllowLookup = false);
        Service(const std::string& strIpPort, uint16_t portDefault, bool fAllowLookup = false);
        Service(const std::string& strIpPort, bool fAllowLookup = false);

        virtual ~Service();

        Service &operator=(const Service &other);
        Service &operator=(Service &other);

        void SetPort(uint16_t portIn);
        uint16_t GetPort() const;
        bool GetSockAddr(struct sockaddr_in* paddr) const;
        friend bool operator==(const Service& a, const Service& b);
        friend bool operator!=(const Service& a, const Service& b);
        friend bool operator<(const Service& a, const Service& b);
        std::vector<uint8_t> GetKey() const;
        std::string ToString() const;
        std::string ToStringPort() const;
        std::string ToStringIPPort() const;
        void print() const;


        bool GetSockAddr6(struct sockaddr_in6* paddr) const;


        IMPLEMENT_SERIALIZE
        (
            Service* pthis = const_cast<Service*>(this);
            READWRITE(FLATDATA(ip));
            uint16_t portN = htons(nPort);
            READWRITE(portN);
            if (fRead)
                pthis->nPort = ntohs(portN);
        )
    };

    /* Proxy Settings for Nexus Core. */
    static Service addrProxy(std::string("127.0.0.1"), static_cast<uint16_t>(9050));
}

#endif
