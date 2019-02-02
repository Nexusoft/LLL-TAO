/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/mutex.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <fstream>
#include <cstring> /* strlen */

#ifdef WIN32
#include <shlobj.h>
#endif

namespace config
{

    /* Read the Config file from the Disk. */
    void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
                        std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet)
    {
        std::ifstream streamConfig(GetConfigFile());
        if(!streamConfig.is_open())
            return; /* No nexus.conf file is OK */

        std::string line;

        while(!streamConfig.eof())
        {
            std::getline(streamConfig, line);

            if(streamConfig.eof())
                break;

            size_t i = line.find('=');
            if(i == std::string::npos)
                continue;

            std::string strKey = std::string("-") + std::string(line, 0, i);
            std::string strVal = std::string(line, i + 1, line.size() - i - 1);

            if(mapSettingsRet.count(strKey) == 0)
            {
                mapSettingsRet[strKey] = strVal;
                /* interpret nofoo=1 as foo=0 (and nofoo=0 as foo=1) as long as foo not set */
                InterpretNegativeSetting(strKey, mapSettingsRet);
            }

            mapMultiSettingsRet[strKey].push_back(strVal);
        }
        streamConfig.close();
    }


    /* Setup PID file for Linux users. */
    void CreatePidFile(const std::string &path, pid_t pid)
    {
        FILE* file = fopen(path.c_str(), "w");
        if (file)
        {
            fprintf(file, "%d", pid);
            fclose(file);
        }
    }


    #ifdef WIN32

    /* Get the system folder path to the special Appdata directory */
    std::string MyGetSpecialFolderPath(int nFolder, bool fCreate)
    {
        char pszPath[MAX_PATH] = "";
        std::string p;

        if (SHGetSpecialFolderPathA(nullptr, pszPath, nFolder, fCreate))
            p = pszPath;

        else if (nFolder == CSIDL_STARTUP)
        {
            p = getenv("USERPROFILE");
            p.append("\\Start Menu");
            p.append("\\Programs");
            p.append("\\Startup");
        }
        else if (nFolder == CSIDL_APPDATA)
            p = getenv("APPDATA");

        return p;
    }
    #endif


    /* Get the default directory Nexus data is stored in. */
    std::string GetDefaultDataDir(std::string strName)
    {
        /* Windows: C:\Documents and Settings\username\Application Data\Nexus
         * Mac: ~/Library/Application Support/Nexus
         * Unix: ~/.Nexus */
    std::string pathRet;
    #ifdef WIN32
        // Windows
        pathRet = MyGetSpecialFolderPath(CSIDL_APPDATA, true);
        pathRet.append("\\" + strName + "\\");
    #else
        char* pszHome = getenv("HOME");
        if (pszHome == nullptr || strlen(pszHome) == 0)
            pathRet = "/";
        else
            pathRet = pszHome;
    #ifdef MAC_OSX
        // Mac
        pathRet.append("/Library/Application Support");
        filesystem::create_directories(pathRet);
        pathRet.append("/" + strName + "/");
    #else
        // Unix
        pathRet.append("/." + strName + "/");
    #endif

        return pathRet;
    #endif
    }


    /* Get the Location of the Config File. */
    std::string GetConfigFile()
    {
        std::string pathConfigFile(GetDataDir(false));

        pathConfigFile.append(GetArg("-conf", "nexus.conf"));

        return pathConfigFile;
    }


    /* Get the Location of the PID File. */
    std::string GetPidFile()
    {
        std::string pathPidFile(GetDataDir());

        pathPidFile.append(GetArg("-pid", "nexus.pid"));

        return pathPidFile;
    }


    /* Get the location that Nexus data is being stored in. */
    std::string GetDataDir(bool fNetSpecific)
    {
        static std::string pathCached[2];
        static std::mutex csPathCached;
        static bool cachedPath[2] = {false, false};

        std::string &path = pathCached[fNetSpecific];

        // This can be called during exceptions by debug log, so we cache the
        // value so we don't have to do memory allocations after that.
        if (cachedPath[fNetSpecific])
            return path;

        LOCK(csPathCached);

        if (mapArgs.count("-datadir"))
        {
            /* get the command line argument for the data directory */
            path = mapArgs["-datadir"];

            uint32_t s = static_cast<uint32_t>(path.size());

            if(s == 0)
                debug::error("No data directory specified.");

            if(path[s-1] != '/')
                path += '/';

            /* build the system complete path to the data directory */
            if(path[0] != '/')
                path = filesystem::system_complete(path);
        }
        else
            path = GetDefaultDataDir();

        if (fNetSpecific && GetBoolArg("-testnet", false))
            path.append("testnet/");

        filesystem::create_directories(path);

        cachedPath[fNetSpecific]=true;
        return path;
    }
}
