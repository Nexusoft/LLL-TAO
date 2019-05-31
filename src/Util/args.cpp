/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>
#include <Util/include/mutex.h>

#include <cstring>
#include <string>
#include <cmath>

namespace config
{
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgs;
    std::map<uint32_t, std::vector<std::string> > mapIPFilters;

    std::atomic<bool> fShutdown(false);
    std::atomic<bool> fDebug(false);
    std::atomic<bool> fPrintToConsole(false);
    std::atomic<bool> fDaemon(false);
    std::atomic<bool> fClient(false);
    std::atomic<bool> fCommandLine(false);
    std::atomic<bool> fTestNet(false);
    std::atomic<bool> fListen(false);
    std::atomic<bool> fUseProxy(false);
    std::atomic<bool> fAllowDNS(false);
    std::atomic<bool> fLogTimestamps(false);
    std::atomic<bool> fAPISessions(false);

    std::mutex ARGS_MUTEX;

    /* Give Opposite Argument Settings */
    void InterpretNegativeSetting(const std::string &name, std::map<std::string, std::string>& mapSettingsRet)
    {
        // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
        if (name.find("-no") == 0)
        {
            std::string positive("-");
            positive.append(name.begin()+3, name.end());
            if (mapSettingsRet.count(positive) == 0)
            {
                bool value = !GetBoolArg(name);
                mapSettingsRet[positive] = (value ? "1" : "0");
            }
        }
    }

    /* Parse the Argument Parameters */
    void ParseParameters(int argc, const char*const argv[])
    {
        LOCK(ARGS_MUTEX);

        for (int i = 1; i < argc; ++i)
        {
            /* Check for arguments value. */
            std::string strArg = std::string(argv[i]);
            if (strArg[0] != '-')
                break;

            /* Compress '--' value into '-' */
            if (strArg[1] == '-')
                strArg.erase(0, 1);

            /* Find value split. */
            std::string::size_type pos = strArg.find("=");

            /* Parse key and value. */
            std::string strKey = strArg.substr(0, pos);
            std::string strVal = "";

            /* Add value if not boolean. */
            if(pos != strArg.npos)
                strVal = strArg.substr(pos + 1);

            /* Build args map. */
            mapArgs[strKey] = strVal;
            mapMultiArgs[strKey].push_back(strVal);

            /* Interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set */
            InterpretNegativeSetting(strKey, mapArgs);
        }
    }


    /* Return string argument or default value */
    std::string GetArg(const std::string& strArg, const std::string& strDefault)
    {
        LOCK(ARGS_MUTEX);

        if (mapArgs.count(strArg))
            return mapArgs[strArg];

        return strDefault;
    }


    /* Return integer argument or default value. */
    int64_t GetArg(const std::string& strArg, int64_t nDefault)
    {
        LOCK(ARGS_MUTEX);

        if (mapArgs.count(strArg))
            return convert::atoi64(mapArgs[strArg]);

        return nDefault;
    }


    /* Return boolean argument or default value */
    bool GetBoolArg(const std::string& strArg, bool fDefault)
    {
        LOCK(ARGS_MUTEX);

        if (mapArgs.count(strArg))
        {
            if (mapArgs[strArg].empty())
                return true;
            return (convert::atoi32(mapArgs[strArg]) != 0);
        }
        return fDefault;
    }


    /* Set an argument if it doesn't already have a value */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue)
    {
        LOCK(ARGS_MUTEX);

        if (mapArgs.count(strArg))
            return false;

        mapArgs[strArg] = strValue;
        return true;
    }


    /* Set a boolean argument if it doesn't already have a value */
    bool SoftSetBoolArg(const std::string& strArg, bool fValue)
    {
        if (fValue)
            return SoftSetArg(strArg, std::string("1"));
        else
            return SoftSetArg(strArg, std::string("0"));
    }


    /* Caches some of the common arguments into global variables for quick/easy access */
    void CacheArgs()
    {
        fDebug                  = GetBoolArg("-debug", false);
        fPrintToConsole         = GetBoolArg("-printtoconsole", false);
        fDaemon                 = GetBoolArg("-daemon", false);
        fTestNet                = GetArg("-testnet", 0) > 0;
        fListen                 = GetBoolArg("-listen", true);
        //fUseProxy               = GetBoolArg("-proxy")
        fAllowDNS               = GetBoolArg("-allowdns", true);
        fLogTimestamps          = GetBoolArg("-logtimestamps", false);
        fAPISessions            = GetBoolArg("-apisessions", false);


        /* Parse the allowip entries and add them to a map for easier processing when new connections are made*/
        const std::vector<std::string>& vIPPortFilters = config::mapMultiArgs["-llpallowip"];

        for(const auto& entry : vIPPortFilters)
        {
            /* ensure it has a port*/
            std::size_t nPortPos = entry.find(":");
            if( nPortPos == std::string::npos)
                continue;

            std::string strIP = entry.substr(0, nPortPos);
            uint32_t nPort = stoi(entry.substr( nPortPos +1));

            mapIPFilters[nPort].push_back(strIP);
        }
    }
}
