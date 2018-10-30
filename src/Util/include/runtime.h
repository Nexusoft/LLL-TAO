/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_RUNTIME_H
#define NEXUS_UTIL_INCLUDE_RUNTIME_H

#define PAIRTYPE(t1, t2)    std::pair<t1, t2> // This is needed because the foreach macro can't get over the comma in pair<t1, t2>

#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

#include <inttypes.h>
#include <thread>
#include <chrono>

/* The location of the unified time seed. To enable a Unified Time System push data to this variable. */
static int UNIFIED_AVERAGE_OFFSET = 0;


/* Return the Current UNIX Timestamp. */
inline int64_t Timestamp(bool fMilliseconds = false)
{
    return fMilliseconds ?  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() :
                            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


inline int64_t UnifiedTimestamp(bool fMilliseconds = false)
{
    return fMilliseconds ? Timestamp(true) + (UNIFIED_AVERAGE_OFFSET * 1000) : Timestamp() + UNIFIED_AVERAGE_OFFSET;
}


/* Sleep for a duration in Milliseconds. */
inline void Sleep(uint32_t nTime, bool fMicroseconds = false)
{
    fMicroseconds ? std::this_thread::sleep_for(std::chrono::microseconds(nTime)) :
                    std::this_thread::sleep_for(std::chrono::milliseconds(nTime));
}


/* Class the tracks the duration of time elapsed in seconds or milliseconds.
    Used for socket timers to determine time outs. */
class Timer
{
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    bool fStopped;

public:
    Timer() : fStopped(false) {}

    void Start()
    {
        start_time = std::chrono::high_resolution_clock::now();

        fStopped = false;
    }

    void Reset()
    {
        Start();
    }

    void Stop()
    {
        end_time = std::chrono::high_resolution_clock::now();

        fStopped = true;
    }

    /* Return the Total Seconds Elapsed Since Timer Started. */
    uint32_t Elapsed()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    /* Return the Total Milliseconds Elapsed Since Timer Started. */
    uint32_t ElapsedMilliseconds()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    /* Return the Total Microseconds Elapsed since Time Started. */
    uint64_t ElapsedMicroseconds()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }
};


inline uint32_t ByteReverse(uint32_t value)
{
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return (value<<16) | (value>>16);
}

#endif
