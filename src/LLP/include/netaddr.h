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
    class NetAddr
    {
    protected:
        uint8_t ip[16]; // in network byte order

    public:
        NetAddr();
        NetAddr(const struct in_addr& ipv4Addr);
        explicit NetAddr(const char *pszIp, bool fAllowLookup = false);
        explicit NetAddr(const std::string &strIp, bool fAllowLookup = false);
        ~NetAddr();

        void Init();
        void SetIP(const NetAddr& ip);
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
        int GetByte(int n) const;
        uint64_t GetHash() const;
        bool GetInAddr(struct in_addr* pipv4Addr) const;
        std::vector<uint8_t> GetGroup() const;
        void print() const;

        NetAddr(const struct in6_addr& pipv6Addr);
        bool GetIn6Addr(struct in6_addr* pipv6Addr) const;

        friend bool operator==(const NetAddr& a, const NetAddr& b);
        friend bool operator!=(const NetAddr& a, const NetAddr& b);
        friend bool operator<(const NetAddr& a, const NetAddr& b);

        IMPLEMENT_SERIALIZE
        (
            READWRITE(FLATDATA(ip));
        )
    };

}
#endif
