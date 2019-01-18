/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_NETADDR_H
#define NEXUS_LLP_INCLUDE_NETADDR_H

#include <LLP/include/network.h>
#include <Util/templates/serialize.h>
#include <vector>
#include <cstdint>

namespace LLP
{
    /** IP address (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96)) */
    class BaseAddress
    {
    protected:
        uint8_t ip[16]; // in network byte order
        uint16_t nPort; // host order

    public:
        BaseAddress();
        BaseAddress(const BaseAddress &other, uint16_t port = 0);

        BaseAddress(BaseAddress &other) = delete;

        BaseAddress(const struct in_addr& ipv4Addr, uint16_t port = 0);
        BaseAddress(const struct in6_addr& ipv6Addr, uint16_t port = 0);
        BaseAddress(const struct sockaddr_in& addr);
        BaseAddress(const struct sockaddr_in6& addr);
        BaseAddress(const char *pszIp, uint16_t portDefault = 0, bool fAllowLookup = false);
        BaseAddress(const std::string &strIp, uint16_t portDefault = 0, bool fAllowLookup = false);


        BaseAddress &operator=(const BaseAddress &other);


        virtual ~BaseAddress();

        void SetPort(uint16_t portIn);
        uint16_t GetPort() const;

        bool GetSockAddr(struct sockaddr_in* paddr) const;
        bool GetSockAddr6(struct sockaddr_in6* paddr) const;

        void SetIP(const BaseAddress& ip);
        bool IsIPv4() const;    // IPv4 mapped address (::FFFF:0:0/96, 0.0.0.0/0)
        bool IsRFC1918() const; // IPv4 private networks (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
        bool IsRFC3849() const; // IPv6 documentation address (2001:0DB8::/32)
        bool IsRFC3927() const; // IPv4 autoconfig (169.254.0.0/16)
        bool IsRFC3964() const; // IPv6 6to4 tunneling (2002::/16)
        bool IsRFC4193() const; // IPv6 unique local (FC00::/15)
        bool IsRFC4380() const; // IPv6 Teredo tunneling (2001::/32)
        bool IsRFC4843() const; // IPv6 ORCHID (2001:10::/28)
        bool IsRFC4862() const; // IPv6 autoconfig (FE80::/64)
        bool IsRFC6052() const; // IPv6 well-known prefix (64:FF9B::/96)
        bool IsRFC6145() const; // IPv6 IPv4-translated address (::FFFF:0:0:0/96)
        bool IsLocal() const;
        bool IsRoutable() const;
        bool IsValid() const;
        bool IsMulticast() const;
        std::string ToString() const;
        std::string ToStringIP() const;
        std::string ToStringPort() const;
        std::string ToStringIPPort() const;
        uint8_t GetByte(uint8_t n) const;
        uint64_t GetHash() const;
        bool GetInAddr(struct in_addr* pipv4Addr) const;
        bool GetIn6Addr(struct in6_addr* pipv6Addr) const;
        std::vector<uint8_t> GetGroup() const;
        void print() const;


        friend bool operator==(const BaseAddress& a, const BaseAddress& b);
        friend bool operator!=(const BaseAddress& a, const BaseAddress& b);
        friend bool operator<(const BaseAddress& a,  const BaseAddress& b);

        IMPLEMENT_SERIALIZE
        (
            BaseAddress *pthis = const_cast<BaseAddress *>(this);
            READWRITE(FLATDATA(ip));
            uint16_t portN = htons(nPort);
            READWRITE(portN);
            if(fRead)
                pthis->nPort = ntohs(portN);
        )
    };

    /* Proxy Settings for Nexus Core. */
    static BaseAddress addrProxy(std::string("127.0.0.1"), static_cast<uint16_t>(9050));

}
#endif
