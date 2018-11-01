/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_CONFIG_H
#define NEXUS_UTIL_INCLUDE_CONFIG_H

#include <string>
#include <vector>
#include <map>


#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#else
typedef int pid_t; /* define for windows compatiblity */
#endif

namespace config
{

    /* Read the Config file from the Disk. */
    void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
        std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet);


    /* Setup PID file for Linux users. */
    void CreatePidFile(const std::string &path, pid_t pid);


    /* Get the default directory Nexus data is stored in. */
    std::string GetDefaultDataDir(std::string strName = "TAO");


    /* Get the Location of the Config File. */
    std::string GetConfigFile();


    /* Get the Location of the PID File. */
    std::string GetPidFile();


    /* Get the location that Nexus data is being stored in. */
    std::string GetDataDir(bool fNetSpecific = true);

    /* Check if set to start when system boots. */
    bool GetStartOnSystemStartup();

    /* Setup to auto start when system boots. */
    bool SetStartOnSystemStartup(bool fAutoStart);

}
#endif
