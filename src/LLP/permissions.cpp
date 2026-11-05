/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/permissions.h>

#include <LLP/include/port.h>

#include <Util/include/args.h>
#include <Util/include/string.h>
#include <Util/include/debug.h>


/*  IP Filtering Definitions. IP's are Filtered By Ports. */
bool CheckPermissions(const std::string& strAddress, const uint16_t nPort)
{
    /* Make a const copy of our IP filters for easy access. */
    static const std::map<uint16_t, std::vector<std::string>> mapFilters = config::mapIPFilters;

    /* Build some constants if we need them. */
    static const uint16_t TRITIUM_MAINNET_PORT_CHECK     = config::GetArg(std::string("-port"),    TRITIUM_MAINNET_PORT);
    static const uint16_t TRITIUM_MAINNET_SSL_PORT_CHECK = config::GetArg(std::string("-sslport"), TRITIUM_MAINNET_SSL_PORT);

    /* Bypass localhost addresses first. */
    if(strAddress == "127.0.0.1" || strAddress == "::1") //XXX: we may not want this rule, assess security
        return true;

    /* Split the Address into String Vector. */
    std::vector<std::string> vAddress;
    ParseString(strAddress, '.', vAddress);

    /* Check our expected sizes. */
    if(vAddress.size() != 4)
        return debug::error("Address size not at least 4 bytes.");

    /* Determine whether or not the current port is open by default, or closed requiring an llpallowip whitelist.
     * Ports open by default can also use a whitelist, and will no longer be treated as open for other addresses */
    bool fStandardPort = false;
    if(config::fTestNet.load()) //XXX: icky, this should be cleaned up here
        fStandardPort = true;
    else
    {
        /* Use a switch statement to check each of our ports. */
        switch(nPort)
        {
            /* Handle the mainnet port. */
            case MAINNET_TIME_LLP_PORT:
                fStandardPort = true;
                break;

            /* Handle the hard coded ports here. */
            case 80:
            case 443:
                fStandardPort = true;
                break;

            default:
                fStandardPort = false;
                break;
        }

        /* We need to use this because we can't evaluate a constexpr in the switch statement from our static const variables. */
        if(nPort == TRITIUM_MAINNET_PORT_CHECK || nPort == TRITIUM_MAINNET_SSL_PORT_CHECK)
            fStandardPort = true;
    }

    /* Check for lookup ports. */
    if(nPort == LLP::GetLookupPort())
        return true;

    /* If no llpallowip whitelist defined for a default open port then we assume permission */
    if(!mapFilters.count(nPort) && fStandardPort)
        return true;

    /* Check if our map is empty without standard port. */
    if(!mapFilters.count(nPort))
        return false;

    /* Check against the llpallowip list from config / commandline parameters. */
    const std::vector<std::string> vFilters = mapFilters.at(nPort);
    for(const auto& strIPFilter : vFilters)
    {
        /* Split the components of the IP so that we can check for wildcard ranges. */
        std::vector<std::string> vCheck;
        ParseString(strIPFilter, '.', vCheck);

        /* Skip invalid inputs. */
        if(vCheck.size() != 4)
            continue;

        /* Check the components of IP address. */
        bool fIPMatches = true;
        for(uint8_t nByte = 0; nByte < 4; ++nByte)
        {
            /* Check that the address matches with wildcards. */
            if(vCheck[nByte] != "*" && vCheck[nByte] != vAddress[nByte])
                fIPMatches = false;
        }

        /* if the IP matches then the address being checked is on the whitelist */
        if(fIPMatches)
            return true;
    }

    return false;
}
