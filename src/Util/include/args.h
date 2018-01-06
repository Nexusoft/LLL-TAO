/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_ARGS_H
#define NEXUS_UTIL_INCLUDE_ARGS_H

#include <map>
#include <string>
#include <vector>

#if defined(MAC_OSX) || defined(WIN32)
typedef int64_t int64;
typedef uint64_t uint64;
#else
typedef long long  int64;
typedef unsigned long long  uint64;
#endif

extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugger;
extern bool fRequestShutdown;
extern bool fShutdown;
extern bool fDaemon;
extern bool fClient;
extern bool fServer;
extern bool fCommandLine;
extern std::string strMiscWarning;
extern bool fTestNet;
extern bool fNoListen;
extern bool fUseProxy;
extern bool fAllowDNS;
extern bool fLogTimestamps;


/**
* Parse the Argument Parameters
*
* @param argc Total Number of Arguments
* @param argv The Arguments Array
*/
void ParseParameters(int argc, const char*const argv[]);


/**
* Give Opposite Argument Settings
*
* @param strArg Argument (e.g. "-foo")
*/
void InterpretNegativeSetting(std::string name, std::map<std::string, std::string>& mapSettingsRet);


/**
* Return string argument or default value
*
* @param strArg Argument to get (e.g. "-foo")
* @param default (e.g. "1")
* @return command-line argument or default value
*/
std::string GetArg(const std::string& strArg, const std::string& strDefault);

/**
* Return integer argument or default value
*
* @param strArg Argument to get (e.g. "-foo")
* @param default (e.g. 1)
* @return command-line argument (0 if invalid number) or default value
*/
int64 GetArg(const std::string& strArg, int64 nDefault);

/**
* Return boolean argument or default value
*
* @param strArg Argument to get (e.g. "-foo")
* @param default (true or false)
* @return command-line argument or default value
*/
bool GetBoolArg(const std::string& strArg, bool fDefault=false);

/**
* Set an argument if it doesn't already have a value
*
* @param strArg Argument to set (e.g. "-foo")
* @param strValue Value (e.g. "1")
* @return true if argument gets set, false if it already had a value
*/
bool SoftSetArg(const std::string& strArg, const std::string& strValue);

/**
* Set a boolean argument if it doesn't already have a value
*
* @param strArg Argument to set (e.g. "-foo")
* @param fValue Value (e.g. false)
* @return true if argument gets set, false if it already had a value
*/
bool SoftSetBoolArg(const std::string& strArg, bool fValue);

#endif
