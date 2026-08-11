/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_TCP_KEEPALIVE_H
#define NEXUS_LLP_INCLUDE_TCP_KEEPALIVE_H

#include <LLP/include/network.h>

#include <Util/include/debug.h>
#include <Util/include/args.h>

#include <cstdint>

namespace LLP
{
namespace TcpKeepalive
{

    /** Default keepalive parameters.
     *
     *  TCP_KEEPIDLE  = 120 s  — first probe after 2 minutes of idle.
     *                           Well below typical NAT/firewall idle timeouts
     *                           (30–60 minutes) so the OS sends probes long
     *                           before the path goes stale.
     *
     *  TCP_KEEPINTVL =  30 s  — interval between subsequent probes.
     *
     *  TCP_KEEPCNT   =   3    — give up after 3 unanswered probes
     *                           (total detection time ≈ 120 + 3×30 = 210 s).
     *
     *  All values are overridable at runtime via command-line flags:
     *      -tcpkeepidle=<seconds>
     *      -tcpkeepintvl=<seconds>
     *      -tcpkeepcnt=<count>
     */
    constexpr int DEFAULT_KEEPIDLE  = 120;
    constexpr int DEFAULT_KEEPINTVL =  30;
    constexpr int DEFAULT_KEEPCNT   =   3;


    /** ApplyKeepalive
     *
     *  Enable TCP keepalive on the given socket file descriptor.
     *
     *  Cross-platform:
     *    - Linux:   SO_KEEPALIVE + TCP_KEEPIDLE + TCP_KEEPINTVL + TCP_KEEPCNT
     *    - macOS:   SO_KEEPALIVE + TCP_KEEPALIVE (idle only; interval/count
     *               are kernel-tunable but not per-socket)
     *    - Windows: SO_KEEPALIVE  (idle/interval set via SIO_KEEPALIVE_VALS
     *               ioctl if available; count is OS-managed)
     *
     *  @param[in] nFd   The socket file descriptor.
     *
     *  @return true if SO_KEEPALIVE was enabled successfully.
     *          Individual parameter failures are logged but not fatal.
     */
    inline bool ApplyKeepalive(int nFd)
    {
        /* Read runtime overrides (fall back to compiled defaults). */
        const int nIdle  = static_cast<int>(config::GetArg(std::string("-tcpkeepidle"),  DEFAULT_KEEPIDLE));
        const int nIntvl = static_cast<int>(config::GetArg(std::string("-tcpkeepintvl"), DEFAULT_KEEPINTVL));
        const int nCnt   = static_cast<int>(config::GetArg(std::string("-tcpkeepcnt"),   DEFAULT_KEEPCNT));

        /* 1. Enable SO_KEEPALIVE — required on all platforms. */
        int nEnable = 1;
        if(setsockopt(nFd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&nEnable), sizeof(nEnable)) != 0)
        {
            debug::error(FUNCTION, "setsockopt(SO_KEEPALIVE) failed: ", WSAGetLastError());
            return false;
        }

    #if defined(__linux__)
        /* 2a. Linux: per-socket idle, interval, and probe count. */
        if(setsockopt(nFd, IPPROTO_TCP, TCP_KEEPIDLE, &nIdle, sizeof(nIdle)) != 0)
            debug::error(FUNCTION, "setsockopt(TCP_KEEPIDLE) failed: ", WSAGetLastError());

        if(setsockopt(nFd, IPPROTO_TCP, TCP_KEEPINTVL, &nIntvl, sizeof(nIntvl)) != 0)
            debug::error(FUNCTION, "setsockopt(TCP_KEEPINTVL) failed: ", WSAGetLastError());

        if(setsockopt(nFd, IPPROTO_TCP, TCP_KEEPCNT, &nCnt, sizeof(nCnt)) != 0)
            debug::error(FUNCTION, "setsockopt(TCP_KEEPCNT) failed: ", WSAGetLastError());

    #elif defined(__APPLE__)
        /* 2b. macOS: TCP_KEEPALIVE sets the idle time (equivalent to TCP_KEEPIDLE).
         *     Interval and count are system-wide sysctl knobs on macOS. */
        if(setsockopt(nFd, IPPROTO_TCP, TCP_KEEPALIVE, &nIdle, sizeof(nIdle)) != 0)
            debug::error(FUNCTION, "setsockopt(TCP_KEEPALIVE) failed: ", WSAGetLastError());

    #elif defined(WIN32)
        /* 2c. Windows: SIO_KEEPALIVE_VALS sets idle and interval together.
         *     Probe count is managed by the OS (default 10). */
        struct tcp_keepalive kaVals;
        kaVals.onoff             = 1;
        kaVals.keepalivetime     = static_cast<ULONG>(nIdle) * 1000;   /* milliseconds */
        kaVals.keepaliveinterval = static_cast<ULONG>(nIntvl) * 1000;  /* milliseconds */

        DWORD dwBytesReturned = 0;
        if(WSAIoctl(nFd, SIO_KEEPALIVE_VALS, &kaVals, sizeof(kaVals),
                    nullptr, 0, &dwBytesReturned, nullptr, nullptr) != 0)
        {
            debug::error(FUNCTION, "WSAIoctl(SIO_KEEPALIVE_VALS) failed: ", WSAGetLastError());
        }
    #endif

        debug::log(2, FUNCTION, "TCP keepalive enabled: idle=", nIdle,
                   "s intvl=", nIntvl, "s cnt=", nCnt);

        return true;
    }

} /* namespace TcpKeepalive */
} /* namespace LLP */

#endif
