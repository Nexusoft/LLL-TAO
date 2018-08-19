/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
            
            (c) Copyright The Nexus Developers 2014 - 2018
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "include/args.h"
#include "include/config.h"
#include "include/mutex.h"

#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>

#include <boost/filesystem/fstream.hpp>

/* Read the Config file from the Disk. */
void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
                    std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet)
{
    namespace fs = boost::filesystem;
    namespace pod = boost::program_options::detail;

    fs::ifstream streamConfig(GetConfigFile());
    if (!streamConfig.good())
        return; // No nexus.conf file is OK

    std::set<std::string> setOptions;
    setOptions.insert("*");

    for (pod::config_file_iterator it(streamConfig, setOptions), end; it != end; ++it)
    {
        // Don't overwrite existing settings so command line settings override bitcoin.conf
        std::string strKey = std::string("-") + it->string_key;
        if (mapSettingsRet.count(strKey) == 0)
        {
            mapSettingsRet[strKey] = it->value[0];
            //  interpret nofoo=1 as foo=0 (and nofoo=0 as foo=1) as long as foo not set)
            InterpretNegativeSetting(strKey, mapSettingsRet);
        }
        mapMultiSettingsRet[strKey].push_back(it->value[0]);
    }
}


/* Setup PID file for Linux users. */
void CreatePidFile(const boost::filesystem::path &path, pid_t pid)
{
    FILE* file = fopen(path.string().c_str(), "w");
    if (file)
    {
        fprintf(file, "%d\n", pid);
        fclose(file);
    }
}


#ifdef WIN32
boost::filesystem::path MyGetSpecialFolderPath(int nFolder, bool fCreate)
{
    namespace fs = boost::filesystem;

    char pszPath[MAX_PATH] = "";
    if(SHGetSpecialFolderPathA(NULL, pszPath, nFolder, fCreate))
    {
        return fs::path(pszPath);
    }
    else if (nFolder == CSIDL_STARTUP)
    {
        return fs::path(getenv("USERPROFILE")) / "Start Menu" / "Programs" / "Startup";
    }
    else if (nFolder == CSIDL_APPDATA)
    {
        return fs::path(getenv("APPDATA"));
    }
    return fs::path("");
}
#endif


/* Get the default directory Nexus data is stored in. */
boost::filesystem::path GetDefaultDataDir(std::string strName)
{
    namespace fs = boost::filesystem;

    // Windows: C:\Documents and Settings\username\Application Data\Nexus
    // Mac: ~/Library/Application Support/Nexus
    // Unix: ~/.Nexus
#ifdef WIN32
    // Windows
    return MyGetSpecialFolderPath(CSIDL_APPDATA, true) / strName;
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    pathRet /= "Library/Application Support";
    fs::create_directory(pathRet);
    return pathRet / strName;
#else
    // Unix
    return pathRet / ("." + strName);
#endif
#endif
}


/* Get the Location of the Config File. */
boost::filesystem::path GetConfigFile()
{
    namespace fs = boost::filesystem;

    fs::path pathConfigFile(GetArg("-conf", "nexus.conf"));
    if (!pathConfigFile.is_complete()) pathConfigFile = GetDataDir(false) / pathConfigFile;
    return pathConfigFile;
}


/* Get the Location of the PID File. */
boost::filesystem::path GetPidFile()
{
    namespace fs = boost::filesystem;

    fs::path pathPidFile(GetArg("-pid", "nexus.pid"));
    if (!pathPidFile.is_complete()) pathPidFile = GetDataDir() / pathPidFile;
    return pathPidFile;
}


/* Get the location that Nexus data is being stored in. */
const boost::filesystem::path &GetDataDir(bool fNetSpecific)
{
    namespace fs = boost::filesystem;

    static fs::path pathCached[2];
    static Mutex_t csPathCached;
    static bool cachedPath[2] = {false, false};

    fs::path &path = pathCached[fNetSpecific];

    // This can be called during exceptions by printf, so we cache the
    // value so we don't have to do memory allocations after that.
    if (cachedPath[fNetSpecific])
        return path;

    LOCK(csPathCached);

    if (mapArgs.count("-datadir")) {
        path = fs::system_complete(mapArgs["-datadir"]);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }
    if (fNetSpecific && GetBoolArg("-testnet", false))
        path /= "testnet";

    fs::create_directory(path);

    cachedPath[fNetSpecific]=true;
    return path;
}


#ifdef WIN32
boost::filesystem::path static StartupShortcutPath()
{
    return MyGetSpecialFolderPath(CSIDL_STARTUP, true) / "nexus.lnk";
}


bool GetStartOnSystemStartup()
{
    return filesystem::exists(StartupShortcutPath());
}


bool SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    boost::filesystem::remove(StartupShortcutPath());

    if (fAutoStart)
    {
        CoInitialize(NULL);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                                CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            TCHAR pszExePath[MAX_PATH];
            GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

            TCHAR pszArgs[5] = TEXT("-min");

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(pszArgs);

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = NULL;
            hres = psl->QueryInterface(IID_IPersistFile,
                                    reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().string().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}

#elif defined(LINUX)

// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html


boost::filesystem::path static GetAutostartDir()
{
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
}


boost::filesystem::path static GetAutostartFilePath()
{
    return GetAutostartDir() / "nexus.desktop";
}


bool GetStartOnSystemStartup()
{
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != string::npos &&
            line.find("true") != string::npos)
            return false;
    }
    optionFile.close();

    return true;
}


bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
        boost::filesystem::remove(GetAutostartFilePath());
    else
    {
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
            return false;

        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), ios_base::out|ios_base::trunc);
        if (!optionFile.good())
            return false;
        
        // Write a nexus.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=Nexus\n";
        optionFile << "Exec=" << pszExePath << " -min\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}
#else

// TODO: OSX startup stuff; see:
// http://developer.apple.com/mac/library/documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif
