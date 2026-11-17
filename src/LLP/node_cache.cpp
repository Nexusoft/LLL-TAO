/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_cache.h>
#include <LLP/include/network.h>

#include <Util/include/string.h>

#include <algorithm>

namespace LLP
{
namespace NodeCache
{
    /** IsLocalhost
     *
     *  Check if an address qualifies for localhost exception handling.
     *
     **/
    bool IsLocalhost(const std::string& strAddress)
    {
        std::string strIP = strAddress;
        if(!strIP.empty() && strIP.front() == '[')
        {
            const size_t nEnd = strIP.find(']');
            if(nEnd == std::string::npos || (nEnd + 1 < strIP.size() && strIP[nEnd + 1] != ':'))
                return false;
            strIP = strIP.substr(1, nEnd - 1);
        }
        else
        {
            const size_t nColon = strIP.find(':');
            if(nColon != std::string::npos && nColon == strIP.rfind(':'))
                strIP = strIP.substr(0, nColon);
        }

        strIP = ToLower(strIP);
        if(strIP == LOCALHOST_NAME)
            return true;

        in_addr ipv4{};
        if(inet_pton(AF_INET, strIP.c_str(), &ipv4) == 1)
            return (ntohl(ipv4.s_addr) >> 24) == 127;

        in6_addr ipv6{};
        const uint8_t loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        return inet_pton(AF_INET6, strIP.c_str(), &ipv6) == 1
            && std::equal(std::begin(loopback), std::end(loopback),
                          reinterpret_cast<const uint8_t*>(&ipv6));
    }


    /** GetPurgeTimeout
     *
     *  Get the appropriate purge timeout for a given address.
     *
     **/
    uint64_t GetPurgeTimeout(const std::string& strAddress, bool fAddressHasPort)
    {
        if(IsLocalhost(strAddress))
            return LOCALHOST_CACHE_PURGE_TIMEOUT;

        /* IPv6 endpoints are stored without brackets, so their final colon
         * can only be interpreted as a port when the caller knows the format. */
        if(fAddressHasPort && !strAddress.empty() && strAddress.front() != '[')
        {
            const size_t nPort = strAddress.rfind(':');
            if(nPort != std::string::npos && IsLocalhost(strAddress.substr(0, nPort)))
                return LOCALHOST_CACHE_PURGE_TIMEOUT;
        }

        return DEFAULT_CACHE_PURGE_TIMEOUT;
    }


    /** GetSessionLivenessTimeout
     *
     *  Get the rolling session liveness timeout used for keepalive expiry.
     *
     **/
    uint64_t GetSessionLivenessTimeout([[maybe_unused]] const std::string& strAddress)
    {
        return SESSION_LIVENESS_TIMEOUT_SECONDS;
    }

} // namespace NodeCache
} // namespace LLP
