/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_CONFIG_H
#define NEXUS_UTIL_INCLUDE_CONFIG_H

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

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


/* Read the Config file from the Disk. */
void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet, std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet);


/* Setup PID file for Linux users. */
void CreatePidFile(const boost::filesystem::path &path, pid_t pid);


/* Check if set to start when system boots. */
bool GetStartOnSystemStartup();


/* Setup to auto start when system boots. */
bool SetStartOnSystemStartup(bool fAutoStart);


/* Get the default directory Nexus data is stored in. */
boost::filesystem::path GetDefaultDataDir(std::string strName = "Nexus");


/* Get the Location of the Config File. */
boost::filesystem::path GetConfigFile();


/* Get the Location of the PID File. */
boost::filesystem::path GetPidFile();


/* Get the location that Nexus data is being stored in. */
const boost::filesystem::path &GetDataDir(bool fNetSpecific = true);


#endif
