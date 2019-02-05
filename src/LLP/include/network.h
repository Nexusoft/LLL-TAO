/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_NETWORK_H
#define NEXUS_LLP_INCLUDE_NETWORK_H

#ifdef WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600    //targeting minimum Windows Vista version for winsock2, etc.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1  //prevents windows.h from including winsock.h and messing up winsock2.h definitions we use
#endif

#ifndef NOMINMAX
#define NOMINMAX //prevents windows.h from including min/max and potentially interfering with std::min/std::max
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <ifaddrs.h>
#include <errno.h>
#endif


#ifdef WIN32
#define MSG_NOSIGNAL        0
#define MSG_DONTWAIT        0
typedef int socklen_t;
#else
typedef u_int SOCKET;

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

#endif
