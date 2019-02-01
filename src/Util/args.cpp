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
#include <Util/include/strlcpy.h>

namespace config
{
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgs;

    bool fDebug = false;
    bool fPrintToConsole = false;
    bool fRequestShutdown = false;
    bool fShutdown = false;
    bool fDaemon = false;
    bool fServer = false;
    bool fClient = false;
    bool fCommandLine = false;
    bool fTestNet = false;
    bool fListen = false;
    bool fUseProxy = false;
    bool fAllowDNS = false;
    bool fLogTimestamps = false;

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
        //mapArgs.clear();
        //mapMultiArgs.clear();
        for (int i = 1; i < argc; ++i)
        {
            char psz[10000];
            strlcpy(psz, argv[i], sizeof(psz));
            char* pszValue = (char*)"";
            if (strchr(psz, '='))
            {
                pszValue = strchr(psz, '=');
                *pszValue++ = '\0';
            }
            #ifdef WIN32
            _strlwr(psz);
            if (psz[0] == '/')
                psz[0] = '-';
            #endif
            if (psz[0] != '-')
                break;

            mapArgs[psz] = pszValue;
            mapMultiArgs[psz].push_back(pszValue);
        }

        for(const auto& entry : mapArgs)
        {
            std::string name = entry.first;

            //  interpret --foo as -foo (as long as both are not set)
            if (name.find("--") == 0)
            {
                std::string singleDash(name.begin()+1, name.end());
                if (mapArgs.count(singleDash) == 0)
                    mapArgs[singleDash] = entry.second;

                name = singleDash;
            }

            // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
            InterpretNegativeSetting(name, mapArgs);
        }
    }

    /* Return string argument or default value */
    std::string GetArg(const std::string& strArg, const std::string& strDefault)
    {
        if (mapArgs.count(strArg))
            return mapArgs[strArg];
        return strDefault;
    }

    /* Return integer argument or default value */
    int64_t GetArg(const std::string& strArg, int64_t nDefault)
    {
        if (mapArgs.count(strArg))
            return atoi64(mapArgs[strArg]);
        return nDefault;
    }

    /* Return boolean argument or default value */
    bool GetBoolArg(const std::string& strArg, bool fDefault)
    {
        if (mapArgs.count(strArg))
        {
            if (mapArgs[strArg].empty())
                return true;
            return (atoi(mapArgs[strArg]) != 0);
        }
        return fDefault;
    }

    /* Set an argument if it doesn't already have a value */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue)
    {
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
        fServer                 = GetBoolArg("-server", false);
        fTestNet                = GetBoolArg("-testnet", false) ||
                                  GetBoolArg("-lispnet", false);
        fListen                 = GetBoolArg("-listen", true);
        //fUseProxy               = GetBoolArg("-proxy")
        fAllowDNS               = GetBoolArg("-allowdns", true);
        fLogTimestamps          = GetBoolArg("-logtimestamps", false);
    }
}
