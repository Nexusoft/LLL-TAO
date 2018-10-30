/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_INCLUDE_RANDOM_H
#define NEXUS_LLC_INCLUDE_RANDOM_H

#include <openssl/rand.h>

#include <LLC/types/uint1024.h>

#ifndef WIN32
#include <sys/time.h>
#endif

namespace LLC
{

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
    uint256_t GetRand256();

    /* Get a random 512 bit number. */
    uint512_t GetRand512();

    /* Get a random 1024 bit number. */
    uint1024_t GetRand1024();

}

#endif
