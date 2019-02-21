/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/debug.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/version.h>

#include <string>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <vector>

#ifndef WIN32
#include <execinfo.h>

#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#define strlwr(psz)         to_lower(psz)
#define _strlwr(psz)        to_lower(psz)

#else


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600    //targeting minimum Windows Vista version for winsock2, etc.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1  //prevents windows.h from including winsock.h and messing up winsock2.h definitions we use
#endif

#ifndef NOMINMAX
#define NOMINMAX //prevents windows.h from including min/max and potentially interfering with std::min/std::max
#endif

#include <windows.h>

#endif



namespace debug
{
    static FILE* fileout = nullptr;
    std::mutex DEBUG_MUTEX;
    std::ofstream ssFile;

    /*  Safer snprintf output string is always null terminated even if the limit
     *  is reach. Returns the number of characters printed. */
    int my_snprintf(char* buffer, size_t limit, const char* format, ...)
    {
        if (limit == 0)
            return 0;
        va_list arg_ptr;
        va_start(arg_ptr, format);
        int ret = _vsnprintf(buffer, limit, format, arg_ptr);
        va_end(arg_ptr);
        if (ret < 0 || ret >= (int)limit)
        {
            ret = limit - 1;
            buffer[limit-1] = 0;
        }
        return ret;
    }


    /* Prints output into a string that is returned. */
    std::string real_strprintf(const char* format, ...)
    {
        char buffer[50000];
        char* p = buffer;
        int limit = sizeof(buffer);
        int ret;

        while(true)
        {
            va_list arg_ptr;
            va_start(arg_ptr, format);
            ret = _vsnprintf(p, limit, format, arg_ptr);
            va_end(arg_ptr);
            if (ret >= 0 && ret < limit)
                break;
            if (p != buffer)
                delete[] p;
            limit *= 2;
            p = new char[limit];
            if (p == nullptr)
                throw std::bad_alloc();
        }
        std::string str(p, p+ret);
        if (p != buffer)
            delete[] p;

        return str;
    }


    /*  Prints output with base class and function information. */
    void print_base(const char* base, const char* format, ...)
    {
        char buffer[50000];
        int limit = sizeof(buffer);
        va_list arg_ptr;
        va_start(arg_ptr, format);
        int ret = _vsnprintf(buffer, limit, format, arg_ptr);
        va_end(arg_ptr);
        if (ret < 0 || ret >= limit)
        {
            buffer[limit-1] = 0;
        }

        debug::log(0, ANSI_COLOR_FUNCTION, base, "::", __func__, "()", ANSI_COLOR_RESET, " : ", buffer);
    }


    /* Write startup information into the log file */
    void InitializeLog(int argc, char** argv)
    {
        log(0, "\nStartup time ", convert::DateTimeStrFormat(runtime::timestamp()));
        log(0, version::CLIENT_VERSION_BUILD_STRING);

    #ifdef WIN32
        log(0, "Microsoft Windows Build (created ", version::CLIENT_DATE, ")");
    #else 
        #ifdef MAC_OSX
        log(0, "Mac OSX Build (created ", version::CLIENT_DATE, ")");
        #else
        log(0, "Linux Build (created ", version::CLIENT_DATE, ")");
        #endif
    #endif

        /* Log configuration file parameters. Need to read them into our own map copy first */
        std::map<std::string, std::string> mapBasicConfig;  //not used
        std::map<std::string, std::vector<std::string> > mapMultiConfig; //All values stored here whether multi or not, will use this

        config::ReadConfigFile(mapBasicConfig, mapMultiConfig);

        std::string confFileParams = "";

        for (const auto& argItem : mapMultiConfig)
        {
            for (int i = 0; i < argItem.second.size(); i++)
            {
                confFileParams += argItem.first;

                if (!argItem.second[i].empty())
                    confFileParams += "=" + argItem.second[i];

                confFileParams += " ";
            }
        }

        if (confFileParams == "")
            confFileParams = "(none)";

        log(0, "Configuration file parameters: ", confFileParams);

        /* Log command line parameters (which can override conf file settings) */
        std::string cmdLineParms = "";

        for (int i = 1; i < argc; i++)
            cmdLineParms += std::string(argv[i]) + " ";

        if (cmdLineParms == "")
            cmdLineParms = "(none)";

        log(0, "Command line parameters: ", cmdLineParms);

        log(0, "\n");
    }


    /*  Prints and logs the stack trace of the code execution call stack up to
     *  the point where this function is called to debug.log */
    void LogStackTrace()
    {
        debug::log(0, "\n\n******* exception encountered *******");
        if (fileout)
        {
        #ifndef WIN32
            void* pszBuffer[32];
            size_t size;
            size = backtrace(pszBuffer, 32);
            backtrace_symbols_fd(pszBuffer, size, fileno(fileout));
        #endif
        }
    }

    /* Outputs a formatted string for the calling thread and exception thrown */
    void FormatException(char* pszMessage, std::exception* pex, const char* pszThread)
    {
    #ifdef WIN32
        char pszModule[MAX_PATH];
        pszModule[0] = '\0';
        GetModuleFileNameA(nullptr, pszModule, sizeof(pszModule));
    #else
        const char* pszModule = "Nexus";
    #endif

        if (pex)
            snprintf(pszMessage, 1000,
                "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
            else
            snprintf(pszMessage, 1000,
                "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
    }

    /*  Prints and logs the exception that is thrown with the named calling thread. */
    void LogException(std::exception* pex, const char* pszThread)
    {
        char pszMessage[10000];
        FormatException(pszMessage, pex, pszThread);
        debug::log(0, pszMessage);
    }

    /*  Prints the exception with the named calling thread and throws it */
    void PrintException(std::exception* pex, const char* pszThread)
    {
        char pszMessage[10000];
        FormatException(pszMessage, pex, pszThread);
        debug::log(0, "\n\n************************\n", pszMessage);
        fprintf(stderr, "\n\n************************\n%s\n", pszMessage);

        throw;
    }

    /*  Prints the exception with the named calling thread but does not throw it. */
    void PrintExceptionContinue(std::exception* pex, const char* pszThread)
    {
        char pszMessage[10000];
        FormatException(pszMessage, pex, pszThread);
        debug::log(0, "************************", pszMessage);
        fprintf(stderr, "************************%s", pszMessage);

    }

    /*  Gets the size of the file in bytes. */
    int GetFilesize(FILE* file)
    {
        int nSavePos = ftell(file);
        int nFilesize = -1;
        if (fseek(file, 0, SEEK_END) == 0)
            nFilesize = ftell(file);
        fseek(file, nSavePos, SEEK_SET);
        return nFilesize;
    }

    /*  Shrinks the size of the debug.log file if it has grown exceptionally large.
     *  It keeps some of the end of the file with most recent log history before
     *  shrinking it down. */
    void ShrinkDebugFile(std::string debugPath)
    {
        /* Scroll debug.log if it's getting too big */
        FILE* file = fopen(debugPath.c_str(), "r");
        if (file && GetFilesize(file) > 10 * 1000000)
        {
            /* Restart the file with some of the end */
            char pch[200000];

            /* define pchSize instead of passing -sizeof() directly to fseek
               fseek size parameter is long int, which on Windows is 32-bit and throws compile warning for conversion overflow
               if you pass -sizeof() which is type size_t, or 64 bit on Windows. So we convert the positive, then pass negative of it */
            uint32_t pchSize = sizeof(pch);
            fseek(file, -pchSize, SEEK_END);

            int nBytes = fread(pch, 1, sizeof(pch), file);
            fclose(file);

            file = fopen(debugPath.c_str(), "w");
            if (file)
            {
                fwrite(pch, 1, nBytes, file);
                fclose(file);
            }
        }
    }


    /*  Open the debug log file. */
    bool init(std::string debugPath)
    {
        LOCK(DEBUG_MUTEX);

        ssFile.open(debugPath, std::ios::app);
        return ssFile.is_open();
    }


    /*  Close the debug log file. */
    void shutdown()
    {
        LOCK(DEBUG_MUTEX);

        if(ssFile.is_open())
            ssFile.close();
    }

}
