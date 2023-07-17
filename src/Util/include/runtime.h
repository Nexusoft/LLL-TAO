/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_RUNTIME_H
#define NEXUS_UTIL_INCLUDE_RUNTIME_H

#include <cstdint>
#include <thread>
#include <chrono>
#include <locale>
#include <atomic>

#include <TAO/Ledger/include/timelocks.h>

#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

/** The location of the unified time seed. To enable a Unified Time System push data to this variable. **/
extern std::atomic<int32_t> UNIFIED_AVERAGE_OFFSET;


namespace runtime
{
    /** strptime
     *
     *  Parse a time string into given time structure for generating unix timestamp from rfc1123
     *
     *  @param[in] s The time string we are parsing.
     *  @param[out] tm The time structure we are outputting to.
     *
     *  @return true if at least one date item was parsed.
     *
     **/
    inline bool strptime(const char* s, struct tm& tm)
    {
        /* Disable daylight savings. */
        tm.tm_isdst = -1;
        tm.tm_year  = 0;
        tm.tm_mon   = 0;
        tm.tm_mday  = 1;
        tm.tm_hour  = 0;
        tm.tm_min   = 0;
        tm.tm_sec   = 0;

        /* Scan up our date format into our time structure. */
        const int nRet =
        sscanf
        (
            s, "%i-%i-%i %i:%i:%i",
            &tm.tm_year, &tm.tm_mon,
            &tm.tm_mday, &tm.tm_hour,
            &tm.tm_min, &tm.tm_sec
        );

        /* Adjust our internal values for mktime. */
        if(tm.tm_year >= 1900)
            tm.tm_year -= 1900;
        else
            return false;

        /* Decrease if not using default values. */
        if(tm.tm_mon)
            tm.tm_mon  -= 1;

        /* Check for a valid total. */
        if(nRet >= 1)
            return true;

        return false;
    }


    /** timestamp
     *
     *  Return the Current UNIX timestamp.
     *
     *  @param[in] fMilliseconds Flag indicating if timestamp should be in milliseconds.
     *
     *  @return The Current UNIX timestamp.
     *
     **/
    inline uint64_t timestamp(bool fMilliseconds = false)
    {
        return fMilliseconds ?  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() :
                                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }


    /** unifiedtimestamp
     *
     *  Return the Current UNIX timestamp with average unified offset
     *
     *  @param[in] fMilliseconds Flag indicating if timestamp should be in milliseconds.
     *
     *  @return The Current UNIX timestamp with average unified offset
     *
     **/
    inline uint64_t unifiedtimestamp(bool fMilliseconds = false)
    {
        return fMilliseconds ? timestamp(true) + (UNIFIED_AVERAGE_OFFSET.load() * 1000) : timestamp() + UNIFIED_AVERAGE_OFFSET.load();
    }


    /** max drift
     *
     *  The maximum time clocks can differ from one another.
     *
     *  @param[in] nVersion The version to check switch for.
     *
     **/
    inline uint32_t maxdrift()
    {
        #ifdef UNIT_TESTS
        return 1000000;
        #endif

        /* Check if before version 8 time-lock. */
        if(unifiedtimestamp() < TAO::Ledger::StartBlockTimelock(8))
            return 10;

        //max drift post v8 is 1 second.
        return 1;
    }


    /** sleep
     *
     *  sleep for a duration in Milliseconds.
     *
     *  @param[in] nTime The amount time to sleep for.
     *
     *  @param[in] fMicroseconds Flag indicating if time is in microseconds.
     *
     **/
    inline void sleep(uint32_t nTime, bool fMicroseconds = false)
    {
        fMicroseconds ? std::this_thread::sleep_for(std::chrono::microseconds(nTime)) :
                        std::this_thread::sleep_for(std::chrono::milliseconds(nTime));
    }


    /** timer
     *
     *  Class the tracks the duration of time elapsed in seconds or milliseconds.
     *  Used for socket timers to determine time outs.
     *
     **/
    class timer
    {
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        bool fStopped;

    public:

        /** timer
         *
         *  Constructs timer class with stopped set to false.
         *
         **/
        timer() : fStopped(false) {}


        /** Running
         *
         *  Returns the current running state of this timer.
         *
         **/
        bool Running()
        {
            return !fStopped;
        }


        /** Start
         *
         *  Capture the start time with a high resolution clock. Sets stopped to false.
         *
         **/
        void Start()
        {
            start_time = std::chrono::high_resolution_clock::now();

            fStopped = false;
        }


        /** Reset
         *
         *  Used as another alias for calling Start()
         *
         **/
        void Reset()
        {
            Start();
        }


        /** Stop
         *
         *  Capture the end time with a high resolution clock. Sets stopped to true.
         *
         **/
        void Stop()
        {
            end_time = std::chrono::high_resolution_clock::now();

            fStopped = true;
        }


        /** Elapsed
         *
         *  Return the Total Seconds Elapsed Since timer Started.
         *
         *  @return The Total Seconds Elapsed Since timer Started.
         *
         **/
        uint32_t Elapsed() const
        {
            if(fStopped)
                return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count());

            return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count());
        }


        /** ElapsedMilliseconds
         *
         *  Return the Total Milliseconds Elapsed Since timer Started.
         *
         *  @return The Total Milliseconds Elapsed Since timer Started.
         *
         **/
        uint32_t ElapsedMilliseconds() const
        {
            if(fStopped)
                return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

            return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count());
        }


        /** ElapsedMicroseconds
         *
         *  Return the Total Microseconds Elapsed since Time Started.
         *
         *  @return The Total Microseconds Elapsed since Time Started.
         *
         **/
        uint64_t ElapsedMicroseconds() const
        {
            if(fStopped)
                return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

            return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        }


        /* Return the Total Nanoseconds Elapsed since Time Started. */
        uint64_t ElapsedNanoseconds() const
        {
            if(fStopped)
                return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

            return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        }
    };


    /** Stopwatch class
     *
     *  Keeps track of total time with starts and stops in between.
     *
     **/
    class stopwatch
    {
        /** The total time elapsed. **/
        uint64_t nElapsed;

        /** The time when last started. **/
        std::chrono::high_resolution_clock::time_point tStart;

        /** Flag to determine when started. **/
        bool fStarted;

    public:

        /** Default Constructor. **/
        stopwatch()
        : nElapsed (0)
        , tStart   ( )
        , fStarted (false)
        {
        }


        void start()
        {
            tStart = std::chrono::high_resolution_clock::now();
            fStarted = true;
        }


        void stop()
        {
            /* Can't stop twice. */
            if(!fStarted)
                return;

            /* Grab the current elapsed time. */
            uint64_t nTime =
                std::chrono::duration_cast<std::chrono::microseconds>
                (
                    std::chrono::high_resolution_clock::now() - tStart
                ).count();

            /* Add this to elapsed state. */
            nElapsed += nTime;
            fStarted  = false;
        }


        void reset()
        {
            nElapsed = 0;
            fStarted = false;
        }


        uint64_t ElapsedMicroseconds()
        {
            return nElapsed;
        }


        uint64_t ElapsedSeconds() const
        {
            return nElapsed / 1000000;
        }

        uint64_t ElapsedMilliseconds() const
        {
            return nElapsed / 1000;
        }
    };


    /** Command
     *
     *  Runs a command to the commandline.
     *
     **/
    inline int command(const std::string& strCommand)
    {
        #ifdef IPHONE
            // Not allowed on iOS
            return 0;
        #else
            try { return std::system(strCommand.c_str()); }
            catch(const std::exception& e){ }
            return 0;
        #endif

    }
}

#endif
