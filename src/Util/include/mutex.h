/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_MUTEX_H
#define NEXUS_UTIL_INCLUDE_MUTEX_H

#include <mutex>

/* Macro preprocessor definitions for debug purposes. */
#define LOCK(mut) std::unique_lock<std::mutex> lk(mut)
#define LOCK2(mut) std::unique_lock<std::mutex> lk2(mut)
#define RLOCK(mut) std::lock_guard<std::recursive_mutex> lk(mut)
#define RLOCK2(mut) std::lock_guard<std::recursive_mutex> lk2(mut)

#endif
