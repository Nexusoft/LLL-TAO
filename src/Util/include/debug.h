/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_DEBUG_H
#define NEXUS_UTIL_INCLUDE_DEBUG_H

#include <string>
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/runtime.h>
#include <Util/include/mutex.h>

//if you don't want to see ANSI colors in the logs, build with NO_ANSI=1
#ifdef NO_ANSI
#define ANSI_COLOR_RED     ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_COLOR_RESET   ""

#define ANSI_COLOR_BRIGHT_RED     ""
#define ANSI_COLOR_BRIGHT_GREEN   ""
#define ANSI_COLOR_BRIGHT_YELLOW  ""
#define ANSI_COLOR_BRIGHT_BLUE    ""
#define ANSI_COLOR_BRIGHT_MAGENTA ""
#define ANSI_COLOR_BRIGHT_CYAN    ""
#define ANSI_COLOR_BRIGHT_WHITE   ""

#define ANSI_COLOR_FUNCTION ""
#else
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

#define ANSI_COLOR_FUNCTION "\u001b[1m"
#endif

//this macro is for creating nice formatting on console logs
#define VALUE(data) data

//this macro will dump a variable name to a string for use in debugging
#define VAR_NAME(a) \
    debug::safe_printstr(#a)

//this macro is used for dumping data structures
#define VARIABLE(a) \
    ANSI_COLOR_FUNCTION, VAR_NAME(a), ANSI_COLOR_RESET, " = ", a

//this macro will dump node related information to the console
#define NODE debug::print_node(this)

//ANSI_COLOR_FUNCTION, " Node", ANSI_COLOR_RESET " : ", "\u001b[1m", GetAddress().ToStringIP(), ANSI_COLOR_RESET, " "

/* Support for Windows */
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define FUNCTION ANSI_COLOR_FUNCTION, __PRETTY_FUNCTION__, ANSI_COLOR_RESET, " : "

namespace debug
{

    extern std::mutex DEBUG_MUTEX;
    extern std::ofstream ssFile;
    extern thread_local std::string strLastError;
    extern thread_local std::string strLastException;

    /* Flag indicating that errors should be logged for this thread (defaults to true). This allows calling code to temporarily
       disable error logging for a given thread. */
    extern thread_local bool fLogError;

    /** Block debug output flags. **/
    struct flags
    {
        enum
        {
            header        = (1 << 0),
            tx            = (1 << 1),
            chain         = (1 << 2)
        };
    };


    /** Initialize
     *
     *  Write startup information into the log file.
     *
     **/
    void Initialize();


    /** Shutdown
     *
     *  Close the debug log file.
     *
     **/
    void Shutdown();


    /** LogStartup
     *
     *  Log startup information.
     *  This will read/log the config file separately from the startup arguments passed as parameters.
     *
     *  @param argc The argc value from main()
     *  @param argv The argv value from main()
     *
     **/
    void LogStartup(int argc, char** argv);


    /** safe_printstr
     *
     *  Safe handle for writing objects into a string.
     *
     *  @param[out] s The stream being written to.
     *  @param[in] head The object being written to stream.
     *  @param[in] tail The variadic parameters.
     *
     **/
    template<class... Args>
    std::string safe_printstr(Args&&... args)
    {
        std::ostringstream ss;
        ((ss << args), ...);

        return ss.str();
    }


    /** log_
     *
     *  Writes log output to console and debug file with timestamps.
     *  Encapsulated log for improved compile time. Not thread safe.
     *
     **/
     void _log(const time_t& nTimestamp, const std::string& strDebug);


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
    void log(const uint32_t nLevel, Args&&... args)
    {
        /* Don't write if log level is below set level. */
        if(config::nVerbose < nLevel)
            return;

        /* We catch execption here to prevent crashes on shutdown for iOS. */
        try
        {
            /* Lock the mutex. */
            LOCK(DEBUG_MUTEX);

            /* Get the debug string and log file. */
            std::string strDebug = safe_printstr(args...);

            /* Get the timestamp. */
            time_t nTimestamp = std::time(nullptr);
            _log(nTimestamp, strDebug);
        }
        catch(const std::exception& e){}
    }


    /** error
     *
     *  Safe constant format debugging error logs.
     *  Dumps to console or to log file.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class... Args>
    bool error(Args&&... args)
    {
        if(fLogError)
        {
            strLastError = safe_printstr(args...);

            debug::log(0, ANSI_COLOR_BRIGHT_RED, "ERROR: ", ANSI_COLOR_RESET, args...);
        }
        return false;
    }


    /** acid_handler
     *
     *  We need this so we can declare in source file and not need forward declaration of LLD::TxnAbort.
     *
     **/
    void acid_handler(const uint8_t nFlags, const uint8_t nInstances);


    /** abort
     *
     *  Safe constant format debugging failure. This function aborts a transaction if failed.
     *
     *  @param[in] nFlags The transaction flags we are aborting
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class... Args>
    bool abort(const uint8_t nFlags, const uint8_t nInstances, Args&&... args)
    {
        if(fLogError)
        {
            strLastError = safe_printstr(args...);

            debug::log(0, ANSI_COLOR_BRIGHT_RED, "ABORT: ", ANSI_COLOR_RESET, args...);
        }

        /* Abort our transaction here. */
        acid_handler(nFlags, nInstances);

        return false;
    }


    /** warning
     *
     *  Safe constant format debugging warning logs.
     *  Dumps to console or to log file.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class... Args>
    void warning(Args&&... args)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "WARNING: ", ANSI_COLOR_RESET, args...);
    }


    /** notice
     *
     *  Safe constant format debugging notice logs.
     *  Dumps to console or to log file.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class... Args>
    void notice(Args&&... args)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "NOTICE: ", ANSI_COLOR_RESET, args...);
    }


    /** drop
     *
     *  Safe constant format debugging network drror logs.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class... Args>
    bool drop(Args&&... args)
    {
        log(0, ANSI_COLOR_BRIGHT_YELLOW, "DROPPED: ", ANSI_COLOR_RESET, args...);

        return false;
    }


    /** ban
     *
     *  Safe constant format debugging network drror logs.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns false always. (Assumed to return an error.)
     *
     **/
    template<class Node, class... Args>
    bool ban(Node* pNode, Args&&... args)
    {
        /* Build a string for message. */
        const std::string strMessage = safe_printstr(args...);

        /* Ban the node and track it's new score. */
        if(pNode && pNode->DDOS)
            pNode->DDOS->Ban(strMessage);

        log(0, ANSI_COLOR_BRIGHT_YELLOW, "BANNED: ", ANSI_COLOR_RESET, strMessage);

        return false;
    }


    /** success
     *
     *  Safe constant format debugging success logs.
     *  Dumps to console or to log file.
     *
     *  @param[in] args The variadic template arguments in.
     *
     *  @return Returns true always. (Assumed to return successful.)
     *
     **/
    template<class... Args>
    bool success(const uint32_t nLevel, Args&&... args)
    {
        /* Don't write if log level is below set level. */
        if(config::nVerbose < nLevel)
            return true;

        log(nLevel, ANSI_COLOR_BRIGHT_GREEN, "SUCCESS: ", ANSI_COLOR_RESET, args...);

        return true;
    }


    /** @class error
     *
     *  Handle exceptions with variadic template constructor for logging support.
     *
     **/
    class exception : public std::runtime_error
    {
    public:

        /** Constructor
         *
         *  @param[in] e The exception object to create error from.
         *
         **/
        exception(const std::runtime_error& e)
        : std::runtime_error(e)
        {
        }


        /** Constructor
         *
         *  @param[in] args The variadic template for initialization.
         *
         **/
        template<class... Args>
        exception(Args&&... args)
        : std::runtime_error(safe_printstr(args...))
        {
        }
    };


    /** print_node
     *
     *  Print information about a node including current address and name.
     *  Uses a template to grab the static class type and force it to the proper polymorphic class.
     *
     *  @param[in] pnode A pointer to the node getting stats from.
     *
     *  @return A string containing the node formatted text.
     *
     **/
    template<typename ProtocolType>
    const std::string print_node(const ProtocolType* pnode)
    {
        return debug::safe_printstr(
            ANSI_COLOR_FUNCTION, ProtocolType::Name(), " Node",
            ANSI_COLOR_RESET, " : ", ANSI_COLOR_FUNCTION, pnode->GetAddress().ToStringIP(), ANSI_COLOR_RESET, " ");
    }


    /** rfc1123Time
     *
     *  Special Specification for HTTP Protocol.
     *  TODO: This could be cleaned up I'd say.
     *
     **/
    std::string rfc1123Time();


    /** InitializeLog
      *
      *  Write startup information into the log file
      *
      *  @param argc The argc value from main()
      *  @param argv The argv value from main()
      *
      */
    void InitializeLog(int argc, char** argv);


    /** GetLastError
     *
     *  Get the last error denoted by the thread local variable strLastError
     *
     **/
    std::string GetLastError();


    /** check_log_archive
     *
     *  Checks if the current debug log should be closed and archived. This
     *  function will close the current file if the max file size is exceeded,
     *  rename it, and open a new file. It will delete the oldest file if it
     *  exceeds the max number of files.
     *
     *  @param[in] outFile The output file stream used to update debug files
     *
     **/
    void check_log_archive(std::ofstream &outFile);


    /** debug_filecount
     *
     *  Returns the number of debug files present in the debug directory.
     *
     **/
    uint32_t debug_filecount();


    /** log_path
     *
     *  Builds an indexed debug log path for a file.
     *
     *  @param[in] nIndex The index for the debug log path.
     *
     *  @return Returns the absolute path to the log file.
     *
     **/
    std::string log_path(uint32_t nIndex);
}
#endif
