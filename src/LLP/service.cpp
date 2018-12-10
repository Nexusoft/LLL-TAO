/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/service.h>
#include <LLP/include/network.h>

#include <LLP/include/hosts.h> //Lookup
#include <Util/include/debug.h>


namespace LLP
{
    Service::Service()
    {
        Init();
    }

    Service::Service(const NetAddr& cip, uint16_t portIn) : NetAddr(cip), port(portIn)
    {
    }


    Service::Service(const struct in_addr& ipv4Addr, uint16_t portIn) : NetAddr(ipv4Addr), port(portIn)
    {
    }


    Service::Service(const struct in6_addr& ipv6Addr, uint16_t portIn) : NetAddr(ipv6Addr), port(portIn)
    {
    }


    Service::Service(const struct sockaddr_in& addr) : NetAddr(addr.sin_addr), port(ntohs(addr.sin_port))
    {
        assert(addr.sin_family == AF_INET);
    }


    Service::Service(const struct sockaddr_in6 &addr) : NetAddr(addr.sin6_addr), port(ntohs(addr.sin6_port))
    {
        assert(addr.sin6_family == AF_INET6);
    }


    Service::Service(const char *pszIpPort, bool fAllowLookup)
    {
        Init();
        Service ip;
        if (Lookup(pszIpPort, ip, 0, fAllowLookup))
            *this = ip;
    }


    Service::Service(const char *pszIpPort, int portDefault, bool fAllowLookup)
    {
        Init();
        Service ip;
        if (Lookup(pszIpPort, ip, portDefault, fAllowLookup))
            *this = ip;
    }


    Service::Service(const std::string &strIpPort, bool fAllowLookup)
    {
        Init();
        Service ip;
        if (Lookup(strIpPort.c_str(), ip, 0, fAllowLookup))
            *this = ip;
    }


    Service::Service(const std::string &strIpPort, int portDefault, bool fAllowLookup)
    {
        Init();
        Service ip;
        if (Lookup(strIpPort.c_str(), ip, portDefault, fAllowLookup))
            *this = ip;
    }

    void Service::Init()
    {
        port = 0;
    }


    uint16_t Service::GetPort() const
    {
        return port;
    }


    bool operator==(const Service& a, const Service& b)
    {
        return (NetAddr)a == (NetAddr)b && a.port == b.port;
    }


    bool operator!=(const Service& a, const Service& b)
    {
        return (NetAddr)a != (NetAddr)b || a.port != b.port;
    }


    bool operator<(const Service& a, const Service& b)
    {
        return (NetAddr)a < (NetAddr)b || ((NetAddr)a == (NetAddr)b && a.port < b.port);
    }


    bool Service::GetSockAddr(struct sockaddr_in* paddr) const
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


    bool Service::GetSockAddr6(struct sockaddr_in6* paddr) const
    {
        memset(paddr, 0, sizeof(struct sockaddr_in6));
        if (!GetIn6Addr(&paddr->sin6_addr))
            return false;
        paddr->sin6_family = AF_INET6;
        paddr->sin6_port = htons(port);
        return true;
    }


    std::vector<uint8_t> Service::GetKey() const
    {
        std::vector<uint8_t> vKey;
        vKey.resize(18);
        memcpy(&vKey[0], ip, 16);
        vKey[16] = port / 0x100;
        vKey[17] = port & 0x0FF;
        return vKey;
    }


    std::string Service::ToStringPort() const
    {
        return debug::strprintf(":%i", port);
    }


    std::string Service::ToStringIPPort() const
    {
        return ToStringIP() + ToStringPort();
    }


    std::string Service::ToString() const
    {
        return ToStringIPPort();
    }


    void Service::print() const
    {
        debug::log(0, "Service(%s)\n", ToString().c_str());
    }


    void Service::SetPort(uint16_t portIn)
    {
        port = portIn;
    }
}
