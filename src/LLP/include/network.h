/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NETWORK_H
#define NEXUS_LLP_INCLUDE_NETWORK_H

#ifdef WIN32
/* Windows specific defs and includes */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600    //minimum Windows Vista version for winsock2, etc.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1  //prevents windows.h from including winsock.h and messing with winsock2.h definitions we use
#endif

#ifndef NOMINMAX
#define NOMINMAX //prevents windows.h from including min/max and potentially interfering with std::min/std::max
#endif

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <ws2tcpip.h>


typedef int socklen_t;

#define MSG_NOSIGNAL        0
#define MSG_DONTWAIT        0

#else
/* Non-windows defs and includes */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <ifaddrs.h>
#include <errno.h>

typedef u_int SOCKET;

/* These alias winsock names to map them for non-Windows */
#define WSAGetLastError()   errno
#define closesocket(x)      close(x)
#define WSAEADDRINUSE       EADDRINUSE
#define WSAEALREADY         EALREADY
#define WSAENOTSOCK         EBADF
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEINTR            EINTR
#define WSAEINVAL           EINVAL
#define WSAEISCONN          EISCONN
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1

#endif // ifdef WIN32


namespace LLP
{
    /** NetworkStartup
     *
     *  Perform any necessary processing to initialize the underlying network
     *  resources such as sockets, etc.
     *
     *  Call this method before starting up any servers or related services.
     *
     *  This method specifically supports calling WSAStartup to initialize Windows sockets.
     *
     *  @return true if the network resource initialization was successful, false otherwise
     *
     **/
    bool NetworkStartup();


    /** NetworkShutdown
     *
     *  Perform any necessary processing to shutdown and release underlying network resources.
     *
     *  Call this method during system shutdown.
     *
     *  This method specifically supports calling WSACleanup to shutdown Windows sockets.
     *
     *  @return true if the network resource initialization was successful, false otherwise
     *
     **/
    bool NetworkShutdown();

}

#endif
