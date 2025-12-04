/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_cache.h>

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
        /* Extract IP from address string (format could be "ip:port") */
        std::string strIP = strAddress;
        size_t nColonPos = strAddress.find(':');
        if(nColonPos != std::string::npos)
            strIP = strAddress.substr(0, nColonPos);

        /* Trim whitespace */
        strIP = ToLower(ParseString(strIP, ' '));

        /* Check against known localhost patterns */
        if(strIP == LOCALHOST_IPV4)
            return true;

        if(strIP == LOCALHOST_IPV6)
            return true;

        if(strIP == LOCALHOST_NAME)
            return true;

        /* Check for 127.x.x.x range */
        if(strIP.substr(0, 4) == "127.")
            return true;

        /* Check for ::1 variants */
        if(strIP == "::1" || strIP == "0:0:0:0:0:0:0:1")
            return true;

        return false;
    }


    /** GetPurgeTimeout
     *
     *  Get the appropriate purge timeout for a given address.
     *
     **/
    uint64_t GetPurgeTimeout(const std::string& strAddress)
    {
        /* Localhost miners get extended timeout */
        if(IsLocalhost(strAddress))
            return LOCALHOST_CACHE_PURGE_TIMEOUT;

        /* Standard timeout for remote miners */
        return DEFAULT_CACHE_PURGE_TIMEOUT;
    }

} // namespace NodeCache
} // namespace LLP
