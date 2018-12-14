/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/netaddr.h>
#include <LLP/include/network.h>
#include <LLP/include/hosts.h>  //LookupHost
#include <Util/include/debug.h> //debug::log
#include <LLC/hash/SK.h>        //LLC::SK64

#include <cstring> //memset, memcmp, memcpy

namespace LLP
{
    static const uint8_t pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

    void NetAddr::Init()
    {
        memset(ip, 0, 16);
    }


    void NetAddr::SetIP(const NetAddr& ipIn)
    {
        memcpy(ip, ipIn.ip, sizeof(ip));
    }


    NetAddr::NetAddr()
    {
        Init();
    }


    NetAddr::NetAddr(const struct in_addr& ipv4Addr)
    {
        memcpy(ip,    pchIPv4, 12);
        memcpy(ip+12, &ipv4Addr, 4);
    }


    NetAddr::NetAddr(const struct in6_addr& ipv6Addr)
    {
        memcpy(ip, &ipv6Addr, 16);
    }


    NetAddr::NetAddr(const char *pszIp, bool fAllowLookup)
    {
        Init();
        std::vector<NetAddr> vIP;
        if (LookupHost(pszIp, vIP, 1, fAllowLookup))
            *this = vIP[0];
    }


    NetAddr::NetAddr(const std::string &strIp, bool fAllowLookup)
    {
        Init();
        std::vector<NetAddr> vIP;
        if (LookupHost(strIp.c_str(), vIP, 1, fAllowLookup))
            *this = vIP[0];
    }


    int NetAddr::GetByte(int n) const
    {
        return ip[15-n];
    }


    bool NetAddr::IsIPv4() const
    {
        return (memcmp(ip, pchIPv4, sizeof(pchIPv4)) == 0);
    }


    bool NetAddr::IsRFC1918() const
    {
        return IsIPv4() && (
            GetByte(3) == 10 ||
            (GetByte(3) == 192 && GetByte(2) == 168) ||
            (GetByte(3) == 172 && (GetByte(2) >= 16 && GetByte(2) <= 31)));
    }


    bool NetAddr::IsRFC3927() const
    {
        return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
    }


    bool NetAddr::IsRFC3849() const
    {
        return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
    }


    bool NetAddr::IsRFC3964() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x02);
    }


    bool NetAddr::IsRFC6052() const
    {
        static const uint8_t pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC6052, sizeof(pchRFC6052)) == 0);
    }


    bool NetAddr::IsRFC4380() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
    }


    bool NetAddr::IsRFC4862() const
    {
        static const uint8_t pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC4862, sizeof(pchRFC4862)) == 0);
    }


    bool NetAddr::IsRFC4193() const
    {
        return ((GetByte(15) & 0xFE) == 0xFC);
    }


    bool NetAddr::IsRFC6145() const
    {
        static const uint8_t pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};
        return (memcmp(ip, pchRFC6145, sizeof(pchRFC6145)) == 0);
    }


    bool NetAddr::IsRFC4843() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10);
    }


    bool NetAddr::IsLocal() const
    {
        // IPv4 loopback
    if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
        return true;

    // IPv6 loopback (::1/128)
    static const uint8_t pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    if (memcmp(ip, pchLocal, 16) == 0)
        return true;

    return false;
    }


    bool NetAddr::IsMulticast() const
    {
        return    (IsIPv4() && (GetByte(3) & 0xF0) == 0xE0)
            || (GetByte(15) == 0xFF);
    }


    bool NetAddr::IsValid() const
    {
        // Clean up 3-byte shifted addresses caused by garbage in size field
        // of addr messages from versions before 0.2.9 checksum.
        // Two consecutive addr messages look like this:
        // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
        // so if the first length field is garbled, it reads the second batch
        // of addr misaligned by 3 bytes.
        if (memcmp(ip, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
            return false;

        // unspecified IPv6 address (::/128)
        uint8_t ipNone[16] = {};
        if (memcmp(ip, ipNone, 16) == 0)
            return false;

        // documentation IPv6 address
        if (IsRFC3849())
            return false;

        if (IsIPv4())
        {
            // INADDR_NONE
            uint32_t ipNone = INADDR_NONE;
            if (memcmp(ip+12, &ipNone, 4) == 0)
                return false;

            // 0
            ipNone = 0;
            if (memcmp(ip+12, &ipNone, 4) == 0)
                return false;
        }

        return true;
    }


    bool NetAddr::IsRoutable() const
    {
        return IsValid() && !(IsRFC1918() || IsRFC3927() || IsRFC4862() || IsRFC4193() || IsRFC4843() || IsLocal());
    }


    std::string NetAddr::ToStringIP() const
    {
        if (IsIPv4())
        {
            char dst[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, ip + 12, dst, INET_ADDRSTRLEN);

            return std::string(dst);
        }
        else
        {
            char dst[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, ip, dst, INET6_ADDRSTRLEN);

            return std::string(dst);
        }
    }


    std::string NetAddr::ToString() const
    {
        return ToStringIP();
    }


    bool operator==(const NetAddr& a, const NetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) == 0);
    }


    bool operator!=(const NetAddr& a, const NetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) != 0);
    }


    bool operator<(const NetAddr& a, const NetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) < 0);
    }


    bool NetAddr::GetInAddr(struct in_addr* pipv4Addr) const
    {
        if (!IsIPv4())
            return false;
        memcpy(pipv4Addr, ip+12, 4);
        return true;
    }


    bool NetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
    {
        memcpy(pipv6Addr, ip, 16);
        return true;
    }


    // get canonical identifier of an address' group
    // no two connections will be attempted to addresses with the same group
    std::vector<uint8_t> NetAddr::GetGroup() const
    {
        std::vector<uint8_t> vchRet;
        int nClass = 0; // 0=IPv6, 1=IPv4, 254=local, 255=unroutable
        int nStartByte = 0;
        int nBits = 16;

        // all local addresses belong to the same group
        if (IsLocal())
        {
            nClass = 254;
            nBits = 0;
        }

        // all unroutable addresses belong to the same group
        if (!IsRoutable())
        {
            nClass = 255;
            nBits = 0;
        }
        // for IPv4 addresses, '1' + the 16 higher-order bits of the IP
        // includes mapped IPv4, SIIT translated IPv4, and the well-known prefix
        else if (IsIPv4() || IsRFC6145() || IsRFC6052())
        {
            nClass = 1;
            nStartByte = 12;
        }
        // for 6to4 tunneled addresses, use the encapsulated IPv4 address
        else if (IsRFC3964())
        {
            nClass = 1;
            nStartByte = 2;
        }
        // for Teredo-tunneled IPv6 addresses, use the encapsulated IPv4 address
        else if (IsRFC4380())
        {
            vchRet.push_back(1);
            vchRet.push_back(GetByte(3) ^ 0xFF);
            vchRet.push_back(GetByte(2) ^ 0xFF);
            return vchRet;
        }
        // for he.net, use /36 groups
        else if (GetByte(15) == 0x20 && GetByte(14) == 0x11 && GetByte(13) == 0x04 && GetByte(12) == 0x70)
            nBits = 36;
        // for the rest of the IPv6 network, use /32 groups
        else
            nBits = 32;

        vchRet.push_back(nClass);
        while (nBits >= 8)
        {
            vchRet.push_back(GetByte(15 - nStartByte));
            nStartByte++;
            nBits -= 8;
        }
        if (nBits > 0)
            vchRet.push_back(GetByte(15 - nStartByte) | ((1 << nBits) - 1));

        return vchRet;
    }


    uint64_t NetAddr::GetHash() const
    {
        return LLC::SK64(&ip[0], &ip[16]);
    }


    void NetAddr::print() const
    {
        debug::log(0, "NetAddr(%s)\n", ToString().c_str());
    }
}
