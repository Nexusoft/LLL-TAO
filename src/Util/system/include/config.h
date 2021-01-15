/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_CONFIG_H
#define NEXUS_UTIL_INCLUDE_CONFIG_H

#include <string>
#include <vector>
#include <map>


#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace config
{

    /** ReadConfigFile
     *
     *  Read the Config file from the Disk.
     *
     *  @param[out] mapSettingsRet The map of config settings.
     *
     *  @param[out] mapMultiSettingsRet The map of multiple config settings per key.
     *
     *  @param argc[in] Number of command line arguments
     *
     *  @param arv[in] Command line arguments
     *
     **/
    void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
        std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet);


    /** ReadConfigFile
     *
     *  Read the Config file from the Disk, loading -datadir setting if provided.
     *
     *  Use this version if need to read config file before parsing command line parameters.
     *
     *  @param[out] mapSettingsRet The map of config settings.
     *
     *  @param[out] mapMultiSettingsRet The map of multiple config settings per key.
     *
     *  @param argc[in] Number of command line arguments
     *
     *  @param arv[in] Command line arguments
     *
     **/
    void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
        std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet,
        const int argc, const char*const argv[]);


    /** CreatePidFile
     *
     *  Setup PID file for Linux users.
     *
     *  @param[in] path The path to the pid file.
     *  @param[in] pid The process ID.
     *
     **/
    void CreatePidFile(const std::string &path, pid_t pid);


    /** GetDefaultDataDir
     *
     *  Get the default directory Nexus data is stored in.
     *
     *  @param[in] strName The name of the default data directory.
     *
     *  @return The system complete path to the default data directory.
     *
     **/
    std::string GetDefaultDataDir(std::string strName = "Nexus");


    /** GetConfigFile
     *
     *  Get the Location of the Config File.
     *
     *  @return The system complete path to the config file.
     *
     **/
    std::string GetConfigFile();


    /** GetPidFile
     *
     *  Get the Location of the PID File.
     *
     *  @return The system complete path to the pid file.
     *
     **/
    std::string GetPidFile();


    /** GetDataDir
     *
     *  Get the location that Nexus data is being stored in.
     *
     *  @param[in] fNetSpecific Flag indicating directory is testnet specific.
     *
     *  @return The system complete path to the data directory.
     *
     **/
    std::string GetDataDir(bool fNetSpecific = true);

}
#endif
