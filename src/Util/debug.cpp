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

#include <stdarg.h>
#include <stdio.h>

#ifndef WIN32
#include <execinfo.h>

#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#define strlwr(psz)         to_lower(psz)
#define _strlwr(psz)        to_lower(psz)
#endif

static FILE* fileout = NULL;
static Mutex_t DEBUG_MUTEX;
int OutputDebugStringF(const char* pszFormat, ...)
{
    LOCK(DEBUG_MUTEX);

    // print to console
    int ret = 0;
    va_list arg_ptr;
    va_start(arg_ptr, pszFormat);
    ret = vprintf(pszFormat, arg_ptr);
    va_end(arg_ptr);

    // print to debug.log
    if (!fileout)
    {
        boost::filesystem::path pathDebug = GetDataDir() / "debug.log";
        fileout = fopen(pathDebug.string().c_str(), "a");
        if (fileout) setbuf(fileout, NULL); // unbuffered
    }

    if (fileout)
    {
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
        ret = vfprintf(fileout, pszFormat, arg_ptr);
        va_end(arg_ptr);
    }

#ifdef WIN32
    if (fPrintToDebugger)
    {
        // accumulate a line at a time
        {
            static char pszBuffer[50000];
            static char* pend;
            if (pend == NULL)
                    pend = pszBuffer;
            va_list arg_ptr;
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

// Safer snprintf
//  - prints up to limit-1 characters
//  - output string is always null terminated even if limit reached
//  - return value is the number of characters actually printed
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


std::string real_strprintf(const std::string &format, int dummy, ...)
{
    char buffer[50000];
    char* p = buffer;
    int limit = sizeof(buffer);
    int ret;
    loop
    {
        va_list arg_ptr;
        va_start(arg_ptr, dummy);
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

    printf(ANSI_COLOR_RED "ERROR: %s" ANSI_COLOR_RESET "\n", buffer);
    return false;
}

void debug(const char* base, const char* format, ...)
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

    printf(ANSI_COLOR_FUNCTION "%s::%s()" ANSI_COLOR_RESET " : %s\n", base, __func__, buffer);
}

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

void LogException(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n%s", pszMessage);
}

void PrintException(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n\n************************\n%s\n", pszMessage);
    fprintf(stderr, "\n\n************************\n%s\n", pszMessage);

    throw;
}

void LogStackTrace() {
    printf("\n\n******* exception encountered *******\n");
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

void PrintExceptionContinue(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n\n************************\n%s\n", pszMessage);
    fprintf(stderr, "\n\n************************\n%s\n", pszMessage);

}

int GetFilesize(FILE* file)
{
    int nSavePos = ftell(file);
    int nFilesize = -1;
    if (fseek(file, 0, SEEK_END) == 0)
        nFilesize = ftell(file);
    fseek(file, nSavePos, SEEK_SET);
    return nFilesize;
}

void ShrinkDebugFile()
{
    // Scroll debug.log if it's getting too big
    boost::filesystem::path pathLog = GetDataDir() / "debug.log";
    FILE* file = fopen(pathLog.string().c_str(), "r");
    if (file && GetFilesize(file) > 10 * 1000000)
    {
        // Restart the file with some of the end
        char pch[200000];
        fseek(file, -sizeof(pch), SEEK_END);
        int nBytes = fread(pch, 1, sizeof(pch), file);
        fclose(file);

        file = fopen(pathLog.string().c_str(), "w");
        if (file)
        {
            fwrite(pch, 1, nBytes, file);
            fclose(file);
        }
    }
}
