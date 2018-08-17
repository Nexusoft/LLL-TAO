/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_RUNTIME_H
#define NEXUS_UTIL_INCLUDE_RUNTIME_H

#define PAIRTYPE(t1, t2)    std::pair<t1, t2> // This is needed because the foreach macro can't get over the comma in pair<t1, t2>

#define loop                for (;;)
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

#include <inttypes.h>

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


/* Thread Type for easy reference. */
typedef boost::thread                                        Thread_t;


/* The location of the unified time seed. To enable a Unified Time System push data to this variable. */
static int UNIFIED_AVERAGE_OFFSET = 0;


/* Return the Current UNIX Timestamp. */
inline int64_t Timestamp(bool fMilliseconds = false)
{
    return fMilliseconds ? ((boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
            boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_milliseconds()) : ((boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
            boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_seconds());
}


inline int64_t UnifiedTimestamp(bool fMilliseconds = false)
{
    return fMilliseconds ? Timestamp(true) + (UNIFIED_AVERAGE_OFFSET * 1000) : Timestamp() + UNIFIED_AVERAGE_OFFSET;
}


/* Sleep for a duration in Milliseconds. */
inline void Sleep(unsigned int nTime, bool fMicroseconds = false){ fMicroseconds ? boost::this_thread::sleep(boost::posix_time::microseconds(nTime)) : boost::this_thread::sleep(boost::posix_time::milliseconds(nTime)); }


/* Class the tracks the duration of time elapsed in seconds or milliseconds.
    Used for socket timers to determine time outs. */
class Timer
{
private:
    boost::posix_time::ptime TIMER_START, TIMER_END;
    bool fStopped = false;

public:
    void Start() { TIMER_START = boost::posix_time::microsec_clock::local_time(); fStopped = false; }
    void Reset() { Start(); }
    void Stop()  { TIMER_END = boost::posix_time::microsec_clock::local_time(); fStopped = true; }

    /* Return the Total Seconds Elapsed Since Timer Started. */
    unsigned int Elapsed()
    {
        if(fStopped)
            return (TIMER_END - TIMER_START).total_seconds();

        return (boost::posix_time::microsec_clock::local_time() - TIMER_START).total_seconds();
    }

    /* Return the Total Milliseconds Elapsed Since Timer Started. */
    unsigned int ElapsedMilliseconds()
    {
        if(fStopped)
            return (TIMER_END - TIMER_START).total_milliseconds();

        return (boost::posix_time::microsec_clock::local_time() - TIMER_START).total_milliseconds();
    }

    /* Return the Total Microseconds Elapsed since Time Started. */
    uint64_t ElapsedMicroseconds()
    {
        if(fStopped)
            return (TIMER_END - TIMER_START).total_microseconds();

        return (boost::posix_time::microsec_clock::local_time() - TIMER_START).total_microseconds();
    }
};



/*
// NOTE: It turns out we might have been able to use boost::thread
// by using TerminateThread(boost::thread.native_handle(), 0); */
#ifdef WIN32
typedef HANDLE bitcoin_pthread_t;

inline bitcoin_pthread_t CreateThread(void(*pfn)(void*), void* parg, bool fWantHandle=false)
{
    DWORD nUnused = 0;
    HANDLE hthread =
        CreateThread(
            NULL,                        // default security
            0,                           // inherit stack size from parent
            (LPTHREAD_START_ROUTINE)pfn, // function pointer
            parg,                        // argument
            0,                           // creation option, start immediately
            &nUnused);                   // thread identifier
    if (hthread == NULL)
    {
        printf("Error: CreateThread() returned %d\n", GetLastError());
        return (bitcoin_pthread_t)0;
    }
    if (!fWantHandle)
    {
        CloseHandle(hthread);
        return (bitcoin_pthread_t)-1;
    }
    return hthread;
}
#else
inline pthread_t CreateThread(void(*pfn)(void*), void* parg, bool fWantHandle=false)
{
    pthread_t hthread = 0;
    int ret = pthread_create(&hthread, NULL, (void*(*)(void*))pfn, parg);
    if (ret != 0)
    {
        printf("Error: pthread_create() returned %d\n", ret);
        return (pthread_t)0;
    }
    if (!fWantHandle)
    {
        pthread_detach(hthread);
        return (pthread_t)-1;
    }
    return hthread;
}

inline void ExitThread(size_t nExitCode)
{
    pthread_exit((void*)nExitCode);
}
#endif


inline uint32_t ByteReverse(uint32_t value)
{
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return (value<<16) | (value>>16);
}

#endif
