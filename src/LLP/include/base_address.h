/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_BASEADDRESS_H
#define NEXUS_LLP_INCLUDE_BASEADDRESS_H

#include <LLP/include/network.h>
#include <Util/templates/serialize.h>
#include <Util/templates/flatdata.h>

#include <vector>
#include <cstdint>

namespace LLP
{
    /** BaseAddress
     *
     *  Serves as the base class IP address class heirarchy.
     *  (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96))
     *
     */
    class BaseAddress
    {
    protected:
        uint8_t ip[16]; /* in network byte order */
        uint16_t nPort; /* host order */

    public:

        /**
         *  Default constructor
         *
         **/
        BaseAddress();


        /**
         *  Copy constructors
         *
         **/
        BaseAddress(const BaseAddress &other, uint16_t port = 0);
        BaseAddress(const struct in_addr &ipv4Addr, uint16_t port = 0);
        BaseAddress(const struct in6_addr &ipv6Addr, uint16_t port = 0);
        BaseAddress(const struct sockaddr_in &addr);
        BaseAddress(const struct sockaddr_in6 &addr);
        BaseAddress(const std::string &strIp, uint16_t portDefault = 0, bool fAllowLookup = false);


        /**
         *  Copy assignment operator
         *
         **/
        BaseAddress &operator=(const BaseAddress &other);


        /**
         *  Default destructor
         *
         **/
        virtual ~BaseAddress();


        /** SetPort
         *
         *  Sets the port number.
         *
         *  @param[in] port The port number to set.
         *
         **/
        void SetPort(uint16_t port);


        /** GetPort
         *
         *  Returns the port number.
         *
         **/
        uint16_t GetPort() const;


        /** SetIP
         *
         *  Sets the IP address.
         *
         *  @param[in] addr The address with the IP address to set.
         *
         **/
        void SetIP(const BaseAddress &addr);


        /** IsIPv4
         *
         *  Determines if address is IPv4 mapped address. (::FFFF:0:0/96, 0.0.0.0/0)
         *
         **/
        bool IsIPv4() const;


        /** IsRFC1918
         *
         *  Determines if address is IPv4 private networks.
         *  (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
         *
         **/
        bool IsRFC1918() const;


        /** IsRFC3849
         *
         *  Determines if address is IPv6 documentation address. (2001:0DB8::/32)
         *
         **/
        bool IsRFC3849() const;


        /** IsRFC3927
         *
         *  Determines if address is IPv4 autoconfig. (169.254.0.0/16)
         *
         **/
        bool IsRFC3927() const;


        /** IsRFC3964
         *
         *  Determines if address is IPv6 6to4 tunneling. (2002::/16)
         *
         **/
        bool IsRFC3964() const;


        /** IsRFC4193
         *
         *  Determines if address is IPv6 unique local. (FC00::/15)
         *
         **/
        bool IsRFC4193() const;


        /** IsRFC4380
         *
         *  Determines if address is IPv6 Teredo tunneling. (2001::/32)
         *
         **/
        bool IsRFC4380() const;


        /** IsRFC4843
         *
         *  Determines if address is IPv6 ORCHID.
         *  (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
         *
         **/
        bool IsRFC4843() const;


        /** IsRFC4862
         *
         *  Determines if address is IPv6 autoconfig. (FE80::/64)
         *
         **/
        bool IsRFC4862() const;


        /** IsRFC6052
         *
         *  Determines if address is IPv6 well-known prefix. (64:FF9B::/96)
         *
         **/
        bool IsRFC6052() const;


        /** IsRFC6145
         *
         *  Determines if address is IPv6 IPv4-translated address. (::FFFF:0:0:0/96)
         *
         **/
        bool IsRFC6145() const;


        /** IsLocal
         *
         *  Determines if address is a local address.
         *
         **/
        bool IsLocal() const;


        /** IsRoutable
         *
         *  Determines if address is a routable address.
         *
         **/
        bool IsRoutable() const;


        /** IsValid
         *
         *  Determines if address is a valid address.
         *
         **/
        bool IsValid() const;


        /** IsMulticast
         *
         *  Determines if address is a multicast address.
         *
         **/
        bool IsMulticast() const;


        /** ToString
         *
         *  Returns the IP and Port in string format. (IP:Port)
         *  NOTE: can't be const, for Windows compile, because calls ToStringIP()
         *
         **/
        std::string ToString() const;


        /** ToStringIP
         *
         *  Returns the IP in string format.
         *
         **/
        std::string ToStringIP() const;



        /** ToStringPort
         *
         *  Returns the Port in string format.
         *
         **/
        std::string ToStringPort() const;


        /** GetByte
         *
         *  Gets a byte from the IP buffer.
         *
         **/
        uint8_t GetByte(uint8_t n) const;

        /** GetHash
         *
         *  Gets a 64-bit hash of the IP.
         *
         **/
        uint64_t GetHash() const;


        /** GetGroup
         *
         *  Gets the canonical identifier of an address' group.
         *  No two connections will be attempted to addresses within
         *  the same group.
         *
         **/
        std::vector<uint8_t> GetGroup() const;


        /** GetInAddr
         *
         *  Gets an IPv4 address struct.
         *
         *  @param[out] pipv4Addr The pointer to the socket address struct.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetInAddr(struct in_addr *pipv4Addr) const;


        /** GetIn6Addr
         *
         *  Gets an IPv6 address struct.
         *
         *  @param[out] pipv6Addr The pointer to the socket address struct.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetIn6Addr(struct in6_addr *pipv6Addr) const;


        /** GetSockAddr
         *
         *  Gets an IPv4 socket address struct.
         *
         *  @param[out] paddr The pointer to the socket address struct.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetSockAddr(struct sockaddr_in *paddr) const;


        /** GetSockAddr6
         *
         *  Gets an IPv6 socket address struct.
         *
         *  @param[out] paddr The pointer to the socket address struct.
         *
         *  @return Returns true if successful, false otherwise.
         *
         **/
        bool GetSockAddr6(struct sockaddr_in6 *paddr) const;


        /** Print
         *
         *  Prints information about this address.
         *
         **/
        virtual void Print() const; 


        /**
         *  Relational operators
         *
         **/
        friend bool operator==(const BaseAddress& a, const BaseAddress& b);
        friend bool operator!=(const BaseAddress& a, const BaseAddress& b);
        friend bool operator<(const BaseAddress& a,  const BaseAddress& b);


        /**
         *  Serialization
         *
         **/
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
