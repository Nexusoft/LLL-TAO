/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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

#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

/** The location of the unified time seed. To enable a Unified Time System push data to this variable. **/
extern std::atomic<int32_t> UNIFIED_AVERAGE_OFFSET;

/** The maximum time in the future clock can be. **/
const uint32_t MAX_UNIFIED_DRIFT = 1;


namespace runtime
{


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
         *  Contructs timer class with stopped set to false.
         *
         **/
        timer() : fStopped(false) {}


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


    /** Command
     *
     *  Runs a command to the commandline.
     *
     **/
    inline int command(std::string strCommand)
    {
        return ::system(strCommand.c_str());
    }
}

#endif
