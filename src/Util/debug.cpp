/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/args.h>
#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <string>
#include <stdarg.h>
#include <stdio.h>

#ifndef WIN32
#include <execinfo.h>

#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#define strlwr(psz)         to_lower(psz)
#define _strlwr(psz)        to_lower(psz)
#endif

namespace debug
{
    static FILE* fileout = NULL;
    static std::recursive_mutex DEBUG_MUTEX;

    /* Prints output to the console. It may also write output to a debug.log
     * if the global fileout file is assigned. */
    int log(const char* pszFormat, ...)
    {
        LOCK(DEBUG_MUTEX);

        /* print to console */
        int ret = 0;
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
        ret = vprintf(pszFormat, arg_ptr);
        va_end(arg_ptr);

        /* print to debug.log */
        if (!fileout)
        {
            std::string pathDebug = config::GetDataDir() + "debug.log";
            fileout = fopen(pathDebug.c_str(), "a");
            if (fileout)
                setbuf(fileout, NULL); // unbuffered
        }

        if (fileout)
        {
            va_start(arg_ptr, pszFormat);
            ret = vfprintf(fileout, pszFormat, arg_ptr);
            va_end(arg_ptr);
        }

    #ifdef WIN32
        if (fPrintToDebugger)
        {
            /* accumulate a line at a time */
            {
                static char pszBuffer[50000];
                static char* pend;
                if (pend == NULL)
                        pend = pszBuffer;

                va_start(arg_ptr, pszFormat);
                int limit = END(pszBuffer) - pend - 2;
                int ret = _vsnprintf(pend, limit, pszFormat, arg_ptr);
                va_end(arg_ptr);
                if (ret < 0 || ret >= limit)
                {
                        pend = END(pszBuffer) - 2;
                        *pend++ = '\n';
                }
                else
                        pend += ret;

                    *pend = '\0';

                char* p1 = pszBuffer;
                char* p2;

                while ((p2 = strchr(p1, '\n')))
                {
                    p2++;
                    char c = *p2;

                    *p2 = '\0';

                    OutputDebugStringA(p1);
                    *p2 = c;
                    p1 = p2;
                }

                if (p1 != pszBuffer)
                    memmove(pszBuffer, p1, pend - p1 + 1);

                pend -= (p1 - pszBuffer);
            }
        }
    #endif
        return ret;
    }

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
    std::string real_strprintf(const std::string &format, ...)
    {
        char buffer[50000];
        char* p = buffer;
        int limit = sizeof(buffer);
        int ret;
        while(true)
        {
            va_list arg_ptr;
            va_start(arg_ptr, 0);
            ret = _vsnprintf(p, limit, format.c_str(), arg_ptr);
            va_end(arg_ptr);
            if (ret >= 0 && ret < limit)
                break;
            if (p != buffer)
                delete[] p;
            limit *= 2;
            p = new char[limit];
            if (p == NULL)
                throw std::bad_alloc();
        }
        std::string str(p, p+ret);
        if (p != buffer)
            delete[] p;

        return str;
    }

    /*  Prints output with a red error caption to the console. It may also write output to a debug.log
     *  if the global fileout file is assigned. */
    bool error(const char *format, ...)
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

        debug::log(ANSI_COLOR_RED "ERROR: %s" ANSI_COLOR_RESET "\n", buffer);
        return false;
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

        debug::log(ANSI_COLOR_FUNCTION "%s::%s()" ANSI_COLOR_RESET " : %s\n", base, __func__, buffer);
    }

    /*  Prints and logs the stack trace of the code execution call stack up to
     *  the point where this function is called to debug.log */
    void LogStackTrace()
    {
        debug::log("\n\n******* exception encountered *******\n");
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
        GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
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
        debug::log("\n%s", pszMessage);
    }

    /*  Prints the exception with the named calling thread and throws it */
    void PrintException(std::exception* pex, const char* pszThread)
    {
        char pszMessage[10000];
        FormatException(pszMessage, pex, pszThread);
        debug::log("\n\n************************\n%s\n", pszMessage);
        fprintf(stderr, "\n\n************************\n%s\n", pszMessage);

        throw;
    }

    /*  Prints the exception with the named calling thread but does not throw it. */
    void PrintExceptionContinue(std::exception* pex, const char* pszThread)
    {
        char pszMessage[10000];
        FormatException(pszMessage, pex, pszThread);
        debug::log("\n\n************************\n%s\n", pszMessage);
        fprintf(stderr, "\n\n************************\n%s\n", pszMessage);

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
    void ShrinkDebugFile()
    {
        /* Scroll debug.log if it's getting too big */
        std::string pathLog = config::GetDataDir() + "\\debug.log";
        FILE* file = fopen(pathLog.c_str(), "r");
        if (file && GetFilesize(file) > 10 * 1000000)
        {
            /* Restart the file with some of the end */
            char pch[200000];
            fseek(file, -sizeof(pch), SEEK_END);
            int nBytes = fread(pch, 1, sizeof(pch), file);
            fclose(file);

            file = fopen(pathLog.c_str(), "w");
            if (file)
            {
                fwrite(pch, 1, nBytes, file);
                fclose(file);
            }
        }
    }

}
