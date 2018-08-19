/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_PERMISSIONS_H
#define NEXUS_LLP_INCLUDE_PERMISSIONS_H

#include <string>
#include <vector>

#include "../../Util/include/args.h"
#include "../../Util/include/parse.h"


/** IP Filtering Definitions
    IP's are Filtered By Ports.
    Format is IP and PORT. **/
inline bool CheckPermissions(std::string strAddress, uint32_t nPort)
{
    /* Bypass localhost addresses first. */
    if(strAddress == "127.0.0.1")
        return true;
    
    /* Split the Address into String Vector. */
    std::vector<std::string> vAddress = Split(strAddress, '.');
    if(vAddress.size() != 4)
        return error("Address size not at least 4 bytes.");
    
    /* Check against the commandline parameters. */
    const std::vector<std::string>& vAllow = mapMultiArgs["-llpallowip"];
    for(int nIndex = 0; nIndex < vAllow.size(); nIndex++)
    {
        /* Detect if the port for LLP filtering is a wildcard or not. */
        bool fWildcardPort = (vAllow[nIndex].find(":") == std::string::npos);
        
        /* Scan out the data for the wildcard port. */
        std::vector<std::string> vCheck = Split(vAllow[nIndex], ',');
        
        /* Skip invalid inputs. */
        if(vCheck.size() != 4)
            continue;
        
        /* Check the Wildcard port. */
        if(!fWildcardPort) {
            std::vector<std::string> strPort = Split(vCheck[3], ':');
            vCheck[3] = strPort[0];
            
            uint32_t nPortCheck = boost::lexical_cast<uint32_t>(strPort[1]);
            if(nPort != nPortCheck)
                return error("Bad Port.");
        }
        
        /* Check the components of IP address. */
        for(int nByte = 0; nByte < 4; nByte++)
            if(vCheck[nByte] != "*" && vCheck[nByte] != vAddress[nByte])
                return error("Check %s - %s\n", vCheck[nByte].c_str(), vAddress[nByte].c_str());
    }
    
    return true;;
}



inline bool WildcardMatch(const char* psz, const char* mask)
{
    for(;;)
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
        psz++;
        mask++;
    }
}

inline bool WildcardMatch(const std::string& str, const std::string& mask)
{
    return WildcardMatch(str.c_str(), mask.c_str());
}

#endif
