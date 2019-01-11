/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_DEBUG_H
#define NEXUS_UTIL_INCLUDE_DEBUG_H

#include <string>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <fstream>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/mutex.h>

#ifdef snprintf
#undef snprintf
#endif
//#define snprintf my_snprintf

#ifndef PRI64d
#if defined(_MSC_VER) || defined(__MSVCRT__)
#define PRI64d  "I64d"
#define PRI64u  "I64u"
#define PRI64x  "I64x"
#else
#define PRI64d  "lld"
#define PRI64u  "llu"
#define PRI64x  "llx"
#endif
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_COLOR_BRIGHT_RED     "\u001b[31;1m"
#define ANSI_COLOR_BRIGHT_GREEN   "\u001b[32;1m"
#define ANSI_COLOR_BRIGHT_YELLOW  "\u001b[33;1m"
#define ANSI_COLOR_BRIGHT_BLUE    "\u001b[34;1m"
#define ANSI_COLOR_BRIGHT_MAGENTA "\u001b[35;1m"
#define ANSI_COLOR_BRIGHT_CYAN    "\u001b[36;1m"
#define ANSI_COLOR_BRIGHT_WHITE   "\u001b[37;1m"

#define ANSI_COLOR_FUNCTION ANSI_COLOR_BRIGHT_BLUE

#define VALUE(data) data
//#define FUNCTION ANSI_COLOR_FUNCTION "%s" ANSI_COLOR_RESET " : "

#define NODE ANSI_COLOR_FUNCTION "Node" ANSI_COLOR_RESET " : "

/* Support for Windows */
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define FUNCTION ANSI_COLOR_FUNCTION, __PRETTY_FUNCTION__, ANSI_COLOR_RESET, " : "

namespace debug
{

    static std::recursive_mutex DEBUG_MUTEX;

    /** Block debug output flags. **/
    enum flags
    {
        header        = (1 << 0),
        tx            = (1 << 1),
        chain         = (1 << 2)
    };


    /** print args
     *
     *  Overload for varadaic templates.
     *
     *  @param[out] s The stream being written to.
     *  @param[in] head The object being written to stream.
     *
     **/
    template<class Head>
    void print_args(std::ostream& s, Head&& head)
    {
        s << std::forward<Head>(head);
    }


    /** print args
     *
     *  Handle for variadic template pack
     *
     *  @param[out] s The stream being written to.
     *  @param[in] head The object being written to stream.
     *  @param[in] tail The variadic parameters.
     *
     **/
    template<class Head, class... Tail>
    void print_args(std::ostream& s, Head&& head, Tail&&... tail)
    {
        s << std::forward<Head>(head);
        print_args(s, std::forward<Tail>(tail)...);
    }


    /** safe printstr
     *
     *  Safe handle for writing objects into a string.
     *
     *  @param[out] s The stream being written to.
     *  @param[in] head The object being written to stream.
     *  @param[in] tail The variadic parameters.
     *
     **/
    template<class... Args>
    std::string safe_printstr(Args&&... args) {
        std::ostringstream ss;
        print_args(ss, std::forward<Args>(args)...);

        return ss.str();
    }


    /** log
     *
     *  Safe constant format debugging logs.
     *  Dumps to console or to log file.
     *
     *  @param[in] nLevel The log level being written.
     *  @param[in] args The variadic template arguments in.
     *
     **/
    template<class... Args>
    void log(uint32_t nLevel, Args&&... args)
    {
        /* Don't write if log level is below set level. */
        if(config::GetArg("-verbose", 0) < nLevel)
            return;

        LOCK(DEBUG_MUTEX);

        /* Get the debug string. */
        std::string debug = safe_printstr(args...);

        /* Dump it to the console. */
        std::cout << debug << std::endl;

        /* Write it to the debug file. */
        std::string pathDebug = config::GetDataDir() + "debug.log";
        std::ofstream ssFile(pathDebug, std::ios::app);
        ssFile << debug << std::endl;
    }

    template<class... Args>
    bool error(Args&&... args)
    {
        log(0, args...);

        return false;
    }


    /** real_strprintf
     *
     *  Prints output into a string that is returned.
     *
     *  @param[in] format The format string specifier.
     *
     *  @param[in] ... The variable argument list to supply to each format
     *                 specifier in the format string.
     *
     *  @return the output string of the printed message
     *
     **/
    std::string real_strprintf(const char* format, ...);
    #define strprintf(format, ...) real_strprintf(format, __VA_ARGS__)


    /** error
     *
     *  Prints output with a red error caption to the console. It may also write output to a debug.log
     *  if the global fileout file is assigned.
     *
     *  @param[in] format The format string specifier.
     *
     *  @param[in] ... The variable argument list to supply to each format
     *                 specifier in the format string.
     *
     *  @return Always returns false.
     *
     **/
    bool error(const char *format, ...);



    /** LogStackTrace
     *
     *  Prints and logs the stack trace of the code execution call stack up to
     *  the point where this function is called to debug.log
     *
     **/
    void LogStackTrace();


    /** LogException
     *
     *  Prints and logs the exception with the named calling thread.
     *
     *  @param[in] pex The pointer to the exception that has been thrown.
     *
     *  @param[in] pszThread The name of the calling thread that threw the exception.
     *
     **/
    void LogException(std::exception* pex, const char* pszThread);


    /** PrintException
     *
     *  Prints the exception with the named calling thread and throws it
     *
     *  @param[in] pex The pointer to the exception that has been thrown.
     *
     *  @param[in] pszThread The name of the calling thread that threw the exception.
     *
     **/
    void PrintException(std::exception* pex, const char* pszThread);


    /** PrintExceptionContinue
     *
     *  Prints the exception with the named calling thread but does not throw it.
     *
     *  @param[in] pex The pointer to the exception that has been thrown.
     *
     *  @param[in] pszThread The name of the calling thread that threw the exception.
     *
     **/
    void PrintExceptionContinue(std::exception* pex, const char* pszThread);


    /** GetFilesize
     *
     *  Gets the size of the file in bytes.
     *
     *  @param[in] file The file pointer of the file get get the size of.
     *
     *  @return The size of the file
     *
     **/
    int GetFilesize(FILE* file);


    /** ShrinkDebugFile
     *
     *  Shrinks the size of the debug.log file if it has grown exceptionally large.
     *  It keeps some of the end of the file with most recent log history before
     *  shrinking it down.
     *
     **/
    void ShrinkDebugFile();

}
#endif
