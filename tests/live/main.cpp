/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/include/random.h>
#include <LLC/prime/fermat.h>

#include <TAO/Ledger/include/prime.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <limits>

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    const uint32_t nHash = LLC::GetRandInt(std::numeric_limits<uint32_t>::max());

    debug::log(0, "Hash is ", nHash);

    uint32_t nBase = 4;


    uint32_t nBuckets = nBase;

    uint32_t nMod = nHash % nBuckets;
    for(int i = 0; i < 16; )
    {
        if(nHash % nBuckets != nMod)
            continue;

        debug::log(0, "Buckets ", nBuckets, " modulus ", nHash % nBuckets);
        nBuckets += nBase;

        i++;
    }


    return 0;

    uint1024_t hashBlock = uint1024_t("0x9e804d2d1e1d3f64629939c6f405f15bdcf8cd18688e140a43beb2ac049333a230d409a1c4172465b6642710ba31852111abbd81e554b4ecb122bdfeac9f73d4f1570b6b976aa517da3c1ff753218e1ba940a5225b7366b0623e4200b8ea97ba09cb93be7d473b47b5aa75b593ff4b8ec83ed7f3d1b642b9bba9e6eda653ead9");

    uint64_t nTimestamp = runtime::timestamp(true);

    for(int i = 0; i < 1000; i++)
        uint1024_t hashFermat = fermat_prime(hashBlock);

    uint64_t nTime = runtime::timestamp(true) - nTimestamp;
    debug::log(0, "monty time is ", nTime);


    nTimestamp = runtime::timestamp(true);

    for(int i = 0; i < 1000; i++)
        uint1024_t hashFermat = TAO::Ledger::FermatTest(hashBlock);

    nTime = runtime::timestamp(true) - nTimestamp;
    debug::log(0, "openssl time is ", nTime);

    return 0;
}
