/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_NETWORK_H
#define NEXUS_LLP_INCLUDE_NETWORK_H

#include <cstdint>

#ifdef WIN32
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#endif

typedef u_int SOCKET;

#ifdef WIN32
#define MSG_NOSIGNAL        0
#define MSG_DONTWAIT        0
typedef int socklen_t;
#else
#include "errno.h"
#define GetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEALREADY         EALREADY
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define WSAENOTSOCK         EBADF
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#endif

#ifdef WIN32
// In MSVC, this is defined as a macro, undefine it to prevent a compile and link error
#undef SetPort
#endif

#ifndef MAINNET_PORT
#define MAINNET_PORT 9323
#endif

#ifndef TESTNET_PORT
#define TESTNET_PORT 8313
#endif

#ifndef MAINNET_CORE_LLP_PORT
#define MAINNET_CORE_LLP_PORT 9324
#endif

#ifndef TESTNET_CORE_LLP_PORT
#define TESTNET_CORE_LLP_PORT 8329
#endif

#ifndef MAINNET_MINING_LLP_PORT
#define MAINNET_MINING_LLP_PORT 9325
#endif

#ifndef TESTNET_MINING_LLP_PORT
#define TESTNET_MINING_LLP_PORT 8325
#endif

#include "../../Util/include/args.h"
#include "../../Util/templates/serialize.h"

namespace LLP
{

    static const uint8_t pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

    /** Services flags */
    enum
    {
        NODE_NETWORK = (1 << 0),
    };


    /** IP address (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96)) */
    class CNetAddr
    {
        protected:
            uint8_t ip[16]; // in network byte order

        public:
            CNetAddr();
            CNetAddr(const struct in_addr& ipv4Addr);
            explicit CNetAddr(const char *pszIp, bool fAllowLookup = false);
            explicit CNetAddr(const std::string &strIp, bool fAllowLookup = false);
            void Init();
            void SetIP(const CNetAddr& ip);
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

    #ifdef USE_IPV6
            CNetAddr(const struct in6_addr& pipv6Addr);
            bool GetIn6Addr(struct in6_addr* pipv6Addr) const;
    #endif

            friend bool operator==(const CNetAddr& a, const CNetAddr& b);
            friend bool operator!=(const CNetAddr& a, const CNetAddr& b);
            friend bool operator<(const CNetAddr& a, const CNetAddr& b);

            IMPLEMENT_SERIALIZE
                (
                READWRITE(FLATDATA(ip));
                )
    };


    /** A combination of a network address (CNetAddr) and a (TCP) port */
    class CService : public CNetAddr
    {
        protected:
            uint16_t port; // host order

        public:
            CService();
            CService(const CNetAddr& ip, uint16_t port);
            CService(const struct in_addr& ipv4Addr, uint16_t port);
            CService(const struct sockaddr_in& addr);
            explicit CService(const char *pszIpPort, int portDefault, bool fAllowLookup = false);
            explicit CService(const char *pszIpPort, bool fAllowLookup = false);
            explicit CService(const std::string& strIpPort, int portDefault, bool fAllowLookup = false);
            explicit CService(const std::string& strIpPort, bool fAllowLookup = false);
            void Init();
            void SetPort(uint16_t portIn);
            uint16_t GetPort() const;
            bool GetSockAddr(struct sockaddr_in* paddr) const;
            friend bool operator==(const CService& a, const CService& b);
            friend bool operator!=(const CService& a, const CService& b);
            friend bool operator<(const CService& a, const CService& b);
            std::vector<uint8_t> GetKey() const;
            std::string ToString() const;
            std::string ToStringPort() const;
            std::string ToStringIPPort() const;
            void print() const;

    #ifdef USE_IPV6
            CService(const struct in6_addr& ipv6Addr, uint16_t port);
            bool GetSockAddr6(struct sockaddr_in6* paddr) const;
            CService(const struct sockaddr_in6& addr);
    #endif

            IMPLEMENT_SERIALIZE
            (
                CService* pthis = const_cast<CService*>(this);
                READWRITE(FLATDATA(ip));
                uint16_t portN = htons(port);
                READWRITE(portN);
                if (fRead)
                    pthis->port = ntohs(portN);
            )
    };


    /** A CService with information about it as peer */
    class CAddress : public CService
    {
        public:
            CAddress();
            explicit CAddress(CService ipIn, uint64_t nServicesIn = NODE_NETWORK);

            void Init();

            IMPLEMENT_SERIALIZE
                (
                CAddress* pthis = const_cast<CAddress*>(this);
                CService* pip = (CService*)pthis;
                if (fRead)
                    pthis->Init();
                if (nSerType & SER_DISK)
                    READWRITE(nSerVersion);
                if ((nSerType & SER_DISK) || (!(nSerType & SER_GETHASH)))
                    READWRITE(nTime);
                READWRITE(nServices);
                READWRITE(*pip);
                )

            void print() const;

        // TODO: make private (improves encapsulation)
        public:
            uint64_t nServices;

            // disk and network only
            uint32_t nTime;

            // memory only
            int64_t nLastTry;
    };


    /** Extended statistics about a CAddress */
    class CAddrInfo : public CAddress
    {
    private:

        /* Who Gave us this Address. */
        CNetAddr source;


        /* The last time this connection was seen. */
        uint32_t nLastSuccess;


        /* The last time this node was tried. */
        uint32_t nLastAttempt;


        /* Number of attempts to connect since last try. */
        uint32_t nAttempts;



    public:

        IMPLEMENT_SERIALIZE(
            CAddress* pthis = (CAddress*)(this);
            READWRITE(*pthis);
            READWRITE(source);
            READWRITE(nLastSuccess);
            READWRITE(nAttempts);
        )

        void Init()
        {
            nLastSuccess = 0;
            nLastAttempt = 0;
            nAttempts    = 0;
        }

        CAddrInfo(const CAddress &addrIn, const CNetAddr &addrSource) : CAddress(addrIn), source(addrSource)
        {
            Init();
        }

        CAddrInfo() : CAddress(), source()
        {
            Init();
        }
    };


    /* Proxy Settings for Nexus Core. */
    static CService addrProxy("127.0.0.1", 9050);


    /* Get the Main Core LLP Port for Nexus. */
    inline uint16_t GetCorePort(const bool testnet = config::fTestNet){ return testnet ? TESTNET_CORE_LLP_PORT : MAINNET_CORE_LLP_PORT; }


    /* Get the Main Mining LLP Port for Nexus. */
    inline uint16_t GetMiningPort(const bool testnet = config::fTestNet){ return testnet ? TESTNET_MINING_LLP_PORT : MAINNET_MINING_LLP_PORT; }


    /* Get the Main Message LLP Port for Nexus. */
    inline uint16_t GetDefaultPort(const bool testnet = config::fTestNet){ return testnet ? TESTNET_PORT : MAINNET_PORT; }


    /** Connect to a socket with given connection timeout flag. **/
    bool ConnectSocket(const CService &addr, SOCKET& hSocketRet, int nTimeout = 5000);

}

#endif
