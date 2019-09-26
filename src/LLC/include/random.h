/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_RANDOM_H
#define NEXUS_LLC_INCLUDE_RANDOM_H

#include <limits>
#include <LLC/types/uint1024.h>

namespace LLC
{

    /** GetPerformanceCounter
     *
     *  Performance counter wrapper for Random Seed Generating.
     *
     **/
    int64_t GetPerformanceCounter();


    /** RandAddSeed
     *
     *  Add a Seed to Random Functions.
     *
     **/
    void RandAddSeed();


    /** RandAddSeedPerfmon
     *
     *  Add a Seed to Random Functions.
     *
     **/
    void RandAddSeedPerfmon();


    /** GetRandInt
     *
     *  Generate Random Number.
     *
     **/
    int GetRandInt(int nMax);


    /** GetRandInt
     *
     *  Generate Random Number. *
     *
     **/
    int GetRandInt(int nMax);


    /** GetRand
     *
     *  Get random 64 bit number.
     *
     **/
    uint64_t GetRand(uint64_t nMax = std::numeric_limits<uint64_t>::max());


    /** GetRand256
     *
     *  Get random 256 bit number.
     *
     **/
    uint256_t GetRand256();


    /** GetRand512
     *
     *  Get a random 512 bit number.
     *
     **/
    uint512_t GetRand512();


    /** GetRand1024
     *
     *  Get a random 1024 bit number.
     *
     **/
    uint1024_t GetRand1024();


}

#endif
