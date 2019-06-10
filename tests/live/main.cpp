/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/cache/binary_lfu.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <list>

template <unsigned N>
double approxRollingAverage(double avg, double input)
{
    avg -= avg/N;
    avg += input/N;
    return avg;
}



std::list<double> listDeltaMA;

double getDeltaMovingAverage(double delta)
{
    listDeltaMA.push_back(delta);
    if (listDeltaMA.size() > 1440) listDeltaMA.pop_front();
    double sum = 0;
    for (std::list<double>::iterator p = listDeltaMA.begin(); p != listDeltaMA.end(); ++p)
        sum += (double)*p;
    return sum / listDeltaMA.size();
}


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{

    uint256_t hashDiff = (~uint256_t(0) >> 16);

    std::vector<uint8_t> vBytes(16, 0);

    uint256_t hash;

    double approx = 0;
    double accumulator = 0;
    double alpha = 0.00012314;
    while(true)
    {
        RAND_bytes((uint8_t*)&vBytes[0], 16);

        hash = LLC::SK1024(vBytes);

        if(hash < hashDiff)
        {
            uint256_t test = (~hash / (hash + 1)) + 1;
            printf("Bits %s\n", hash.ToString().c_str());
            printf("Test %lu\n", test.Get64());

            uint64_t nValue = test.Get64();
            double delta = getDeltaMovingAverage(nValue);

            accumulator = (alpha * nValue) + (1.0 - alpha) * accumulator;

            approx = approxRollingAverage<1440>(approx, nValue);
            printf("Delta %f Approx %f Accumulator %f\n", delta, approx, accumulator);
        }
    }


    return 0;
}
