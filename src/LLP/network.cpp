/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/network.h>
#include <LLP/include/hosts.h>

#include <LLC/hash/SK.h>

#include <Util/include/debug.h>
#include <Util/include/strlcpy.h>

#ifndef WIN32
#include <sys/fcntl.h>
#endif


namespace LLP
{


    CAddress::CAddress() : CService()
    {
        Init();
    }


    CAddress::CAddress(CService ipIn, uint64_t nServicesIn) : CService(ipIn)
    {
        Init();
        nServices = nServicesIn;
    }


    void CAddress::Init()
    {
        nServices = NODE_NETWORK;
        nTime = 100000000;
        nLastTry = 0;
    }


    void CNetAddr::Init()
    {
        memset(ip, 0, 16);
    }


    void CNetAddr::SetIP(const CNetAddr& ipIn)
    {
        memcpy(ip, ipIn.ip, sizeof(ip));
    }


    CNetAddr::CNetAddr()
    {
        Init();
    }


    CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
    {
        memcpy(ip,    pchIPv4, 12);
        memcpy(ip+12, &ipv4Addr, 4);
    }


    #ifdef USE_IPV6
    CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr)
    {
        memcpy(ip, &ipv6Addr, 16);
    }
    #endif


    CNetAddr::CNetAddr(const char *pszIp, bool fAllowLookup)
    {
        Init();
        std::vector<CNetAddr> vIP;
        if (LookupHost(pszIp, vIP, 1, fAllowLookup))
            *this = vIP[0];
    }


    CNetAddr::CNetAddr(const std::string &strIp, bool fAllowLookup)
    {
        Init();
        std::vector<CNetAddr> vIP;
        if (LookupHost(strIp.c_str(), vIP, 1, fAllowLookup))
            *this = vIP[0];
    }


    int CNetAddr::GetByte(int n) const
    {
        return ip[15-n];
    }


    bool CNetAddr::IsIPv4() const
    {
        return (memcmp(ip, pchIPv4, sizeof(pchIPv4)) == 0);
    }


    bool CNetAddr::IsRFC1918() const
    {
        return IsIPv4() && (
            GetByte(3) == 10 ||
            (GetByte(3) == 192 && GetByte(2) == 168) ||
            (GetByte(3) == 172 && (GetByte(2) >= 16 && GetByte(2) <= 31)));
    }


    bool CNetAddr::IsRFC3927() const
    {
        return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
    }


    bool CNetAddr::IsRFC3849() const
    {
        return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
    }


    bool CNetAddr::IsRFC3964() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x02);
    }


    bool CNetAddr::IsRFC6052() const
    {
        static const uint8_t pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC6052, sizeof(pchRFC6052)) == 0);
    }


    bool CNetAddr::IsRFC4380() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
    }


    bool CNetAddr::IsRFC4862() const
    {
        static const uint8_t pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC4862, sizeof(pchRFC4862)) == 0);
    }


    bool CNetAddr::IsRFC4193() const
    {
        return ((GetByte(15) & 0xFE) == 0xFC);
    }


    bool CNetAddr::IsRFC6145() const
    {
        static const uint8_t pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};
        return (memcmp(ip, pchRFC6145, sizeof(pchRFC6145)) == 0);
    }


    bool CNetAddr::IsRFC4843() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10);
    }


    bool CNetAddr::IsLocal() const
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


    bool CNetAddr::IsMulticast() const
    {
        return    (IsIPv4() && (GetByte(3) & 0xF0) == 0xE0)
            || (GetByte(15) == 0xFF);
    }


    bool CNetAddr::IsValid() const
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


    bool CNetAddr::IsRoutable() const
    {
        return IsValid() && !(IsRFC1918() || IsRFC3927() || IsRFC4862() || IsRFC4193() || IsRFC4843() || IsLocal());
    }


    std::string CNetAddr::ToStringIP() const
    {
        if (IsIPv4())
            return debug::strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
        else
            return debug::strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
                            GetByte(15) << 8 | GetByte(14), GetByte(13) << 8 | GetByte(12),
                            GetByte(11) << 8 | GetByte(10), GetByte(9) << 8 | GetByte(8),
                            GetByte(7) << 8 | GetByte(6), GetByte(5) << 8 | GetByte(4),
                            GetByte(3) << 8 | GetByte(2), GetByte(1) << 8 | GetByte(0));
    }


    std::string CNetAddr::ToString() const
    {
        return ToStringIP();
    }


    bool operator==(const CNetAddr& a, const CNetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) == 0);
    }


    bool operator!=(const CNetAddr& a, const CNetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) != 0);
    }


    bool operator<(const CNetAddr& a, const CNetAddr& b)
    {
        return (memcmp(a.ip, b.ip, 16) < 0);
    }


    bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
    {
        if (!IsIPv4())
            return false;
        memcpy(pipv4Addr, ip+12, 4);
        return true;
    }


    #ifdef USE_IPV6
    bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
    {
        memcpy(pipv6Addr, ip, 16);
        return true;
    }
    #endif


    // get canonical identifier of an address' group
    // no two connections will be attempted to addresses with the same group
    std::vector<uint8_t> CNetAddr::GetGroup() const
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


    uint64_t CNetAddr::GetHash() const
    {
        return LLC::SK64(&ip[0], &ip[16]);
    }


    void CNetAddr::print() const
    {
        printf("CNetAddr(%s)\n", ToString().c_str());
    }


    void CService::Init()
    {
        port = 0;
    }


    CService::CService()
    {
        Init();
    }


    CService::CService(const CNetAddr& cip, uint16_t portIn) : CNetAddr(cip), port(portIn)
    {
    }


    CService::CService(const struct in_addr& ipv4Addr, uint16_t portIn) : CNetAddr(ipv4Addr), port(portIn)
    {
    }


    #ifdef USE_IPV6
    CService::CService(const struct in6_addr& ipv6Addr, uint16_t portIn) : CNetAddr(ipv6Addr), port(portIn)
    {
    }
    #endif


    CService::CService(const struct sockaddr_in& addr) : CNetAddr(addr.sin_addr), port(ntohs(addr.sin_port))
    {
        assert(addr.sin_family == AF_INET);
    }


    #ifdef USE_IPV6
    CService::CService(const struct sockaddr_in6 &addr) : CNetAddr(addr.sin6_addr), port(ntohs(addr.sin6_port))
    {
    assert(addr.sin6_family == AF_INET6);
    }
    #endif


    CService::CService(const char *pszIpPort, bool fAllowLookup)
    {
        Init();
        CService ip;
        if (Lookup(pszIpPort, ip, 0, fAllowLookup))
            *this = ip;
    }


    CService::CService(const char *pszIpPort, int portDefault, bool fAllowLookup)
    {
        Init();
        CService ip;
        if (Lookup(pszIpPort, ip, portDefault, fAllowLookup))
            *this = ip;
    }


    CService::CService(const std::string &strIpPort, bool fAllowLookup)
    {
        Init();
        CService ip;
        if (Lookup(strIpPort.c_str(), ip, 0, fAllowLookup))
            *this = ip;
    }


    CService::CService(const std::string &strIpPort, int portDefault, bool fAllowLookup)
    {
        Init();
        CService ip;
        if (Lookup(strIpPort.c_str(), ip, portDefault, fAllowLookup))
            *this = ip;
    }


    uint16_t CService::GetPort() const
    {
        return port;
    }


    bool operator==(const CService& a, const CService& b)
    {
        return (CNetAddr)a == (CNetAddr)b && a.port == b.port;
    }


    bool operator!=(const CService& a, const CService& b)
    {
        return (CNetAddr)a != (CNetAddr)b || a.port != b.port;
    }


    bool operator<(const CService& a, const CService& b)
    {
        return (CNetAddr)a < (CNetAddr)b || ((CNetAddr)a == (CNetAddr)b && a.port < b.port);
    }


    bool CService::GetSockAddr(struct sockaddr_in* paddr) const
    {
        if (!IsIPv4())
            return false;
        memset(paddr, 0, sizeof(struct sockaddr_in));
        if (!GetInAddr(&paddr->sin_addr))
            return false;
        paddr->sin_family = AF_INET;
        paddr->sin_port = htons(port);
        return true;
    }


    #ifdef USE_IPV6
    bool CService::GetSockAddr6(struct sockaddr_in6* paddr) const
    {
        memset(paddr, 0, sizeof(struct sockaddr_in6));
        if (!GetIn6Addr(&paddr->sin6_addr))
            return false;
        paddr->sin6_family = AF_INET6;
        paddr->sin6_port = htons(port);
        return true;
    }
    #endif


    std::vector<uint8_t> CService::GetKey() const
    {
        std::vector<uint8_t> vKey;
        vKey.resize(18);
        memcpy(&vKey[0], ip, 16);
        vKey[16] = port / 0x100;
        vKey[17] = port & 0x0FF;
        return vKey;
    }


    std::string CService::ToStringPort() const
    {
        return debug::strprintf(":%i", port);
    }


    std::string CService::ToStringIPPort() const
    {
        return ToStringIP() + ToStringPort();
    }


    std::string CService::ToString() const
    {
        return ToStringIPPort();
    }


    void CService::print() const
    {
        printf("CService(%s)\n", ToString().c_str());
    }


    void CService::SetPort(uint16_t portIn)
    {
        port = portIn;
    }
}
