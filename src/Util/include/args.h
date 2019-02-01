/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_ARGS_H
#define NEXUS_UTIL_INCLUDE_ARGS_H

#include <map>
#include <vector>
#include <string>

namespace config
{

    extern std::map<std::string, std::string> mapArgs;
    extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
    extern std::string strMiscWarning;

    extern bool fDebug;
    extern bool fPrintToConsole;
    extern bool fRequestShutdown;
    extern bool fShutdown;
    extern bool fDaemon;
    extern bool fClient;
    extern bool fServer;
    extern bool fCommandLine;
    extern bool fTestNet;
    extern bool fListen;
    extern bool fUseProxy;
    extern bool fAllowDNS;
    extern bool fLogTimestamps;


    /** InterpretNegativeSetting
     *
     *  Give Opposite Argument Settings.
     *
     *  @param[in] name The Argument to interpret the opposite of. (e.g. "-foo")
     *  @param[out] mapSettingsRet The map of settings to store the negative setting in.
     *
     **/
    void InterpretNegativeSetting(const std::string &name, std::map<std::string, std::string>& mapSettingsRet);


    /** ParseParameters
     *
     *  Parse the Argument Parameters.
     *
     *  @param argc Total Number of Arguments.
     *  @param argv The Arguments Array.
     *
     **/
    void ParseParameters(int argc, const char*const argv[]);


    /** GetArg
     *
     *  Return string argument or default value.
     *
     *  @param strArg Argument to get (e.g. "-foo")
     *  @param strDefault The default string argument. (e.g. "1")
     *
     *  @return command-line argument or default value.
     *
     **/
    std::string GetArg(const std::string& strArg, const std::string& strDefault);


    /** GetArg
     *
     *  Return integer argument or default value.
     *
     *  @param strArg Argument to get. (e.g. "-foo")
     *  @param nDefault The default integer argument. (e.g. 1)
     *
     *  @return command-line argument (0 if invalid number) or default value.
     *
     **/
    int64_t GetArg(const std::string& strArg, int64_t nDefault);


    /** GetBoolArg
     *
     *  Return boolean argument or default value.
     *
     *  @param strArg Argument to get (e.g. "-foo")
     *  @param fDefault The default bool argument. (true or false)
     *
     *  @return command-line argument or default value
     *
     **/
    bool GetBoolArg(const std::string& strArg, bool fDefault=false);


    /** SoftSetArg
     *
     * Set an argument if it doesn't already have a value.
     *
     * @param strArg Argument to set. (e.g. "-foo")
     * @param strValue Value to set. (e.g. "1")
     *
     * @return true if argument gets set, false if it already had a value.
     *
     **/
    bool SoftSetArg(const std::string& strArg, const std::string& strValue);


    /** SoftSetBoolArg
     *
     * Set a boolean argument if it doesn't already have a value
     *
     * @param strArg Argument to set. (e.g. "-foo")
     * @param fValue Value to set. (e.g. false)
     *
     * @return True if argument gets set, false if it already had a value.
     *
     **/
    bool SoftSetBoolArg(const std::string& strArg, bool fValue);

    /** CacheArgs
    *
    *  Caches some of the common arguments into global variables for quick/easy access
    *
    **/
    void CacheArgs();

}
#endif
