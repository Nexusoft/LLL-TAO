/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_MUTEX_H
#define NEXUS_UTIL_INCLUDE_MUTEX_H

#include <mutex>
#include <shared_mutex>

/* Macro preprocessor definitions for debug purposes. */
#define LOCK(mut) std::unique_lock<std::mutex> lk(mut)
#define LOCK2(mut) std::unique_lock<std::mutex> lk2(mut)

/* Reader-writer lock macros for std::shared_mutex.
 * SHARED_LOCK: concurrent read access (multiple readers allowed).
 * WRITE_LOCK:  exclusive write access (blocks all readers and writers). */
#define SHARED_LOCK(mut)  std::shared_lock<std::shared_mutex> slk(mut)
#define SHARED_LOCK2(mut) std::shared_lock<std::shared_mutex> slk2(mut)
#define WRITE_LOCK(mut)   std::unique_lock<std::shared_mutex> ulk(mut)
#define WRITE_LOCK2(mut)  std::unique_lock<std::shared_mutex> ulk2(mut)

/* Variadic macro to support multiple locks in same macro. */
#define CRITICAL(...) std::scoped_lock<std::mutex>            __LOCK(__VA_ARGS__)
#define RECURSIVE(...) std::scoped_lock<std::recursive_mutex> __LOCK(__VA_ARGS__)

#endif
