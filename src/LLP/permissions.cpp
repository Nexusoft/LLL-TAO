/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/permissions.h>

#include <Util/include/args.h>
#include <Util/include/string.h>
#include <Util/include/debug.h>


/*  IP Filtering Definitions. IP's are Filtered By Ports. */
bool CheckPermissions(const std::string &strAddress, uint32_t nPort)
{
    /* Bypass localhost addresses first. */
    if(strAddress == "127.0.0.1" || strAddress == "::1")
        return true;

    /* Split the Address into String Vector. */
    std::vector<std::string> vAddress = Split(strAddress, '.');
    if(vAddress.size() != 4)
        return debug::error("Address size not at least 4 bytes.");

    /* Check against the llpallowip list from config / commandline parameters. */

    /* If no llpallowip whitelist has been defined for this port then we assume they are allowed */
    if(config::mapIPFilters[nPort].size() == 0)
        return true;

    for(const auto& strIPFilter : config::mapIPFilters[nPort])
    {

        /* Split the components of the IP so that we can check for wildcard ranges. */
        std::vector<std::string> vCheck = Split(strIPFilter, '.');

        /* Skip invalid inputs. */
        if(vCheck.size() != 4)
            continue;

        /* Check the components of IP address. */
        bool fIPMatches = true;
        for(uint8_t nByte = 0; nByte < 4; ++nByte)
            if(vCheck[nByte] != "*" && vCheck[nByte] != vAddress[nByte])
                fIPMatches = false;

        /* if the IP matches then the address being checked is on the whitelist */
        if(fIPMatches)
            return true;

    }

    return false;
}


/* Performs a wildcard match string search given a search string and a mask. */
bool WildcardMatch(const char* psz, const char* mask)
{
    while(true)
    {
        switch (*mask)
        {
        case '\0':
            return (*psz == '\0');
        case '*':
            return WildcardMatch(psz, mask+1) || (*psz && WildcardMatch(psz+1, mask));
        case '?':
            if (*psz == '\0')
                return false;
            break;
        default:
            if (*psz != *mask)
                return false;
            break;
        }
        ++psz;
        ++mask;
    }
}


/* Performs a wildcard match string search given a search string and a mask. */
bool WildcardMatch(const std::string& str, const std::string& mask)
{
    return WildcardMatch(str.c_str(), mask.c_str());
}
