/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_MUTEX_H
#define NEXUS_UTIL_INCLUDE_MUTEX_H

#include <mutex>

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
#endif


/** CMutexLock
 *
 *  Wrapper around std::unique_lock
 *
 **/
template<typename Mutex>
class CMutexLock
{
private:
    std::unique_lock<Mutex> lock;
public:

    /** Enter
     *
     *  Attempt to obtain lock and critical section code.
     *
     *  @param[in] pszName Label of function or method
     *
     *  @param[in] pszFile File name to log to.
     *
     *  @param[in] nLine Line number of where this code is called
     *
     **/
    void Enter(const char* pszName, const char* pszFile, int nLine)
    {
        if (!lock.owns_lock())
        {
            EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()));
        #ifdef DEBUG_LOCKCONTENTION
            if (!lock.try_lock())
            {
                debug::log(0, "LOCKCONTENTION: ", pszName);
                debug::log(0, "Locker: ", pszFile,  ":", nLine);
        #endif
            lock.lock();
        #ifdef DEBUG_LOCKCONTENTION
            }
        #endif
        }
    }


    /** Leave
     *
     *  Attempt to unlock and leave the critical section code.
     *
     **/
    void Leave()
    {
        if (lock.owns_lock())
        {
            lock.unlock();
            LeaveCritical();
        }
    }


    /** TryEnter
     *
     *  Attempt to obtain lock and critical section code.
     *
     *  @param[in] pszName Label of function or method
     *
     *  @param[in] pszFile File name to log to.
     *
     *  @param[in] nLine Line number of where this code is called
     *
     *  @return True if the lock is owned, false otherwise.
     *
     **/
    bool TryEnter(const char* pszName, const char* pszFile, int nLine)
    {
        if (!lock.owns_lock())
        {
            EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()), true);
            lock.try_lock();
            if (!lock.owns_lock())
                LeaveCritical();
        }
        return lock.owns_lock();
    }


    /** CMutexLock
     *
     *  Constructor for unique_lock wrapper class CMutexLock.
     *
     *  @param[in] mutexIn The mutex being passed in.
     *
     *  @param[in] pszName Label of function or method
     *
     *  @param[in] pszFile File name to log to.
     *
     *  @param[in] nLine Line number of where this code is called
     *
     *  @param[in] fTry Flag if lock should force enter, or try to enter
     *
     **/
    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) : lock(mutexIn)
    {
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }


    /** ~CMutexLock
     *
     *  Destructor for unique_lock wrapper class CMutexLock that releases the
     *  lock and leaves critical section if it has the lock.
     *
     *
     **/
    ~CMutexLock()
    {
        if (lock.owns_lock())
            LeaveCritical();
    }


    /** operator bool()
     *
     *  Overload () operator for if the lock is owned or not.
     *
     **/
    operator bool()
    {
        return lock.owns_lock();
    }


    /** GetLock
     *
     *  Returns a reference to the unique_lock object.
     *
     *  @return A reference to the unique_lock object.
     *
     **/
    std::unique_lock<Mutex> &GetLock()
    {
        return lock;
    }
};

typedef CMutexLock<std::recursive_mutex> CCriticalBlock;

/* Macro preprocessor definitions for debug purposes. */
#define LOCK(cs) std::lock_guard<std::recursive_mutex> lock(cs)

//#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1,cs2) CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__),criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs,name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, true)

#define ENTER_CRITICAL_SECTION(cs) \
    { \
        EnterCritical(#cs, __FILE__, __LINE__, (void*)(&cs)); \
        (cs).lock(); \
    }

#define LEAVE_CRITICAL_SECTION(cs) \
    { \
        (cs).unlock(); \
        LeaveCritical(); \
    }

#endif
