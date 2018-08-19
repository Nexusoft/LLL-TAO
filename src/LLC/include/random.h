/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_INCLUDE_RANDOM_H
#define NEXUS_LLC_INCLUDE_RANDOM_H

#include <openssl/rand.h>

#include "../types/uint1024.h"

#ifndef WIN32
#include <sys/time.h>
#endif


/** Performance counter wrapper for Random Seed Generating. **/
inline int64_t GetPerformanceCounter()
{
    int64_t nCounter = 0;
#ifdef WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&nCounter);
#else
    timeval t;
    gettimeofday(&t, NULL);
    nCounter = t.tv_sec * 1000000 + t.tv_usec;
#endif
    return nCounter;
}

/** Add a Seed to Random Functions. */
void RandAddSeed();
void RandAddSeedPerfmon();

/* Generate Random Number. */
int GetRandInt(int nMax);

/* Generate Random Number. */
int GetRandInt(int nMax);

/* Get random 64 bit number. */
uint64_t GetRand(uint64_t nMax);

/* Get random 256 bit number. */
uint256 GetRand256();

/* Get a random 512 bit number. */
uint512 GetRand512();

/* Get a random 1024 bit number. */
uint1024 GetRand1024();

#endif
