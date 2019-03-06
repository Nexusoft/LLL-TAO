/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/base_address.h>
#include <LLP/include/network.h>
#include <LLP/include/hosts.h>   //Lookup
#include <LLC/hash/SK.h>         //LLC::SK64
#include <Util/include/debug.h>  //debug::log
#include <Util/include/memory.h> //memory::compare
#include <algorithm>             //std::copy

namespace LLP
{
    static const uint8_t pchIPv4[12] =  {0,0,0,0,0,0,0,0,0,0,0xff,0xff};
    static const uint8_t pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    static const uint8_t pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
    static const uint8_t pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
    static const uint8_t pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};


    /* Default constructor */
    BaseAddress::BaseAddress()
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(0)
    {
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const BaseAddress &other, uint16_t port)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(port)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        if(port == 0)
            nPort = other.nPort;
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const struct in_addr& ipv4Addr, uint16_t port)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(port)
    {
        std::copy((uint8_t*)&pchIPv4[0], (uint8_t*)&pchIPv4[0] + 12, (uint8_t*)&ip[0]);
        std::copy((uint8_t*)&ipv4Addr, (uint8_t*)&ipv4Addr + 4, (uint8_t*)&ip[0] + 12);
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const struct in6_addr& ipv6Addr, uint16_t port)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(port)
    {
        std::copy((uint8_t*)&ipv6Addr, (uint8_t*)&ipv6Addr + 16, (uint8_t*)&ip[0]);
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const struct sockaddr_in& addr)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(0)
    {
        if(addr.sin_family != AF_INET)
        {
            debug::error(FUNCTION, "improper sockaddr_in for IPv4");
            return;
        }

        std::copy((uint8_t*)&pchIPv4[0], (uint8_t*)&pchIPv4[0] + 12, (uint8_t*)&ip[0]);
        std::copy((uint8_t*)&addr.sin_addr, (uint8_t*)&addr.sin_addr + 4, (uint8_t*)&ip[0] + 12);

        nPort = ntohs(addr.sin_port);
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const struct sockaddr_in6 &addr)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(0)
    {
        if(addr.sin6_family != AF_INET6)
        {
            debug::error(FUNCTION, "improper sockaddr_in6 for IPv6");
            return;
        }

        std::copy((uint8_t*)&addr.sin6_addr, (uint8_t*)&addr.sin6_addr + 16, (uint8_t*)&ip[0]);

        nPort = ntohs(addr.sin6_port);
    }


    /* Copy constructor */
    BaseAddress::BaseAddress(const std::string &strIp, uint16_t portDefault, bool fAllowLookup)
    : ip {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    , nPort(0)
    {

        /* Make sure there is a string to lookup. */
        size_t s = strIp.size();
        if (s == 0 || s > 255)
        {
            debug::error(FUNCTION, "Invalid lookup string of size ", s, ".");
            return;
        }

        if(fAllowLookup)
        {
            if (Lookup(strIp, *this, portDefault, true))
                debug::log(3, FUNCTION, strIp, " resolved to ", ToStringIP());
            else
                debug::log(3, FUNCTION, strIp, " bad lookup");
        }
        else
            Lookup(strIp, *this, portDefault, false);

        nPort = portDefault;
    }


    /* Copy assignment operator */
    BaseAddress &BaseAddress::operator=(const BaseAddress &other)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = other.ip[i];

        nPort = other.nPort;

        return *this;
    }


    /* Default destructor */
    BaseAddress::~BaseAddress()
    {
    }


    /*  Sets the port number. */
    void BaseAddress::SetPort(uint16_t port)
    {
        nPort = port;
    }


    /* Returns the port number. */
    uint16_t BaseAddress::GetPort() const
    {
        return nPort;
    }


    /* Sets the IP address. */
    void BaseAddress::SetIP(const BaseAddress& addr)
    {
        for(uint8_t i = 0; i < 16; ++i)
            ip[i] = addr.ip[i];
    }


    /*  Determines if address is IPv4 mapped address. (::FFFF:0:0/96, 0.0.0.0/0) */
    bool BaseAddress::IsIPv4() const
    {
        return (memory::compare(ip, pchIPv4, sizeof(pchIPv4)) == 0);
    }


    /*  Determines if address is a LISP EID mapped address. */
    bool BaseAddress::IsEID() const
    {
        return (IsIPv4() && GetByte(3) == 240)
        || (!IsIPv4() && (GetByte(15) == 0xfd || GetByte(15) == 0xfe));
    }


    /* Determines if address is IPv4 private networks.
     * (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12) */
    bool BaseAddress::IsRFC1918() const
    {
        return IsIPv4() && (
            GetByte(3) == 10 ||
            (GetByte(3) == 192 && GetByte(2) == 168) ||
            (GetByte(3) == 172 && (GetByte(2) >= 16 && GetByte(2) <= 31)));
    }


    /* Determines if address is IPv6 documentation address. (2001:0DB8::/32) */
    bool BaseAddress::IsRFC3849() const
    {
        return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
    }


    /* Determines if address is IPv4 autoconfig. (169.254.0.0/16) */
    bool BaseAddress::IsRFC3927() const
    {
        return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
    }


    /* Determines if address is IPv6 6to4 tunneling. (2002::/16) */
    bool BaseAddress::IsRFC3964() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x02);
    }


    /* Determines if address is IPv6 unique local. (FC00::/15) */
    bool BaseAddress::IsRFC4193() const
    {
        return ((GetByte(15) & 0xFE) == 0xFC);
    }


    /* Determines if address is IPv6 Teredo tunneling. (2001::/32) */
    bool BaseAddress::IsRFC4380() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
    }


    /* Determines if address is IPv6 ORCHID.
     * (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12) */
    bool BaseAddress::IsRFC4843() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10);
    }


    /* Determines if address is IPv6 autoconfig. (FE80::/64) */
    bool BaseAddress::IsRFC4862() const
    {
        return (memory::compare(ip, pchRFC4862, sizeof(pchRFC4862)) == 0);
    }


    /* Determines if address is IPv6 well-known prefix. (64:FF9B::/96) */
    bool BaseAddress::IsRFC6052() const
    {
        return (memory::compare(ip, pchRFC6052, sizeof(pchRFC6052)) == 0);
    }


    /* Determines if address is IPv6 IPv4-translated address. (::FFFF:0:0:0/96) */
    bool BaseAddress::IsRFC6145() const
    {
        return (memory::compare(ip, pchRFC6145, sizeof(pchRFC6145)) == 0);
    }


    /* Determines if address is a local address. */
    bool BaseAddress::IsLocal() const
    {
        // IPv4 loopback
        if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
            return true;

        // IPv6 loopback (::1/128)
        if (memory::compare(ip, pchLocal, 16) == 0)
            return true;

        return false;
    }


    /* Determines if address is a routable address. */
    bool BaseAddress::IsRoutable() const
    {
        return IsValid() && !(IsRFC3927() ||
                              IsRFC4862() ||
                              IsRFC4193() ||
                              IsRFC4843() ||
                              IsLocal());

    }


    /* Determines if address is a valid address. */
    bool BaseAddress::IsValid() const
    {
        // Clean up 3-byte shifted addresses caused by garbage in size field
        // of addr messages from versions before 0.2.9 checksum.
        // Two consecutive addr messages look like this:
        // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
        // so if the first length field is garbled, it reads the second batch
        // of addr misaligned by 3 bytes.
        //if (memcmp(ip, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
        if (memory::compare(ip, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
            return false;

        // unspecified IPv6 address (::/128)
        uint8_t ipNone[16] = {};
        if (memory::compare(ip, ipNone, 16) == 0)
            return false;

        // documentation IPv6 address
        if (IsRFC3849())
            return false;

        if (IsIPv4())
        {
            // INADDR_NONE
            uint32_t ip_none = INADDR_NONE;

            if (memory::compare(ip+12, (uint8_t *)&ip_none, 4) == 0)
                return false;

            ip_none = 0;

            if (memory::compare(ip+12, (uint8_t *)&ip_none, 4) == 0)
                return false;
        }

        return true;
    }


    /* Determines if address is a multicast address. */
    bool BaseAddress::IsMulticast() const
    {
        return (IsIPv4() && (GetByte(3) & 0xF0) == 0xE0)
            || (GetByte(15) == 0xFF);
    }


    /* Returns the IP and Port in string format. (IP:Port) */
    std::string BaseAddress::ToString() const
    {
        return ToStringIP() + std::string(":") + ToStringPort();
    }


    /* Returns the IP in string format. */
    std::string BaseAddress::ToStringIP() const
    {
        /* inet_ntop on mingw64 is void* (non-const), so compile fails if pass ip within const method. Make a non-const copy we can use */
        uint8_t ipCopy[16];
        for (int i=0; i<16; ++i)
            ipCopy[i] = ip[i];

        if (IsIPv4())
        {
            char dst[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, ipCopy + 12, dst, INET_ADDRSTRLEN);
            return std::string(dst);
        }

        char dst[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, ipCopy, dst, INET6_ADDRSTRLEN);
        return std::string(dst);
    }


    /* Returns the Port in string format. */
    std::string BaseAddress::ToStringPort() const
    {
        return std::to_string(nPort);
    }


    /* Gets a byte from the IP buffer. */
    uint8_t BaseAddress::GetByte(uint8_t n) const
    {
        if(n > 15)
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "out of range ", n));

        return ip[15-n];
    }


    /* Gets a 64-bit hash of the IP. */
    uint64_t BaseAddress::GetHash() const
    {
        return LLC::SK64(&ip[0], &ip[16]);
    }


    /** GetGroup
     *
     *  Gets the canonical identifier of an address' group.
     *  No two connections will be attempted to addresses within
     *  the same group.
     *
     */
    std::vector<uint8_t> BaseAddress::GetGroup() const
    {
        std::vector<uint8_t> vchRet;
        int32_t nClass = 0; // 0=IPv6, 1=IPv4, 254=local, 255=unroutable
        int32_t nStartByte = 0;
        int32_t nBits = 16;

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

        vchRet.push_back(static_cast<uint8_t>(nClass));
        while (nBits >= 8)
        {
            vchRet.push_back(GetByte(static_cast<uint8_t>(15 - nStartByte)));
            ++nStartByte;
            nBits -= 8;
        }
        if (nBits > 0)
        {
            uint8_t b = GetByte(static_cast<uint8_t>(15 - nStartByte));
            uint32_t c = static_cast<uint32_t>(b) | ((1 << nBits) - 1);
            vchRet.push_back(static_cast<uint8_t>(c));
        }


        return vchRet;
    }


    /* Gets an IPv4 address struct. */
    bool BaseAddress::GetInAddr(struct in_addr* pipv4Addr) const
    {
        if (!IsIPv4())
            return false;

        std::copy((uint8_t*)&ip[0] + 12, (uint8_t*)&ip[0] + 16, (uint8_t*)pipv4Addr);
        return true;
    }


    /* Gets an IPv6 address struct. */
    bool BaseAddress::GetIn6Addr(struct in6_addr* pipv6Addr) const
    {
        std::copy((uint8_t*)&ip[0], (uint8_t*)&ip[0] + 16, (uint8_t*)pipv6Addr);
        return true;
    }


    /* Gets an IPv4 socket address struct. */
    bool BaseAddress::GetSockAddr(struct sockaddr_in* paddr) const
    {
        if (!IsIPv4() || !paddr)
            return false;

        paddr->sin_family = 0;
        paddr->sin_port = 0;
        paddr->sin_addr.s_addr = 0;

        if (!GetInAddr(&paddr->sin_addr))
            return false;

        paddr->sin_family = AF_INET;
        paddr->sin_port = htons(nPort);

        return true;
    }


    /* Gets an IPv6 socket address struct. */
    bool BaseAddress::GetSockAddr6(struct sockaddr_in6* paddr) const
    {
        if(!paddr)
            return false;

        paddr->sin6_family = 0;
        paddr->sin6_port = 0;
        paddr->sin6_flowinfo = 0;

        for(uint8_t i = 0; i < 16; ++i)
            paddr->sin6_addr.s6_addr[i] = 0;

        paddr->sin6_scope_id = 0;

        if (!GetIn6Addr(&paddr->sin6_addr))
            return false;

        paddr->sin6_family = AF_INET6;
        paddr->sin6_port = htons(nPort);

        return true;
    }


    /* Prints information about this address. */
    void BaseAddress::Print()
    {
        debug::log(0, "BaseAddress(", ToString(), ")");
    }


    /* Relational operator equals */
    bool operator==(const BaseAddress& a, const BaseAddress& b)
    {
        //return (memcmp(a.ip, b.ip, 16) == 0);
        return (memory::compare(a.ip, b.ip, 16) == 0);
    }


    /* Relational operator not equals */
    bool operator!=(const BaseAddress& a, const BaseAddress& b)
    {
        return (memory::compare(a.ip, b.ip, 16) != 0);
    }


    /* Relational operator less than */
    bool operator<(const BaseAddress& a, const BaseAddress& b)
    {
        return (memory::compare(a.ip, b.ip, 16) < 0);
    }

}
