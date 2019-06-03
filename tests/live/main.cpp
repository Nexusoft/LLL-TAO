/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/include/random.h>

#include <LLD/cache/binary_lru.h>

#include <Util/include/debug.h>

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{

    LLD::BinaryLRU* cache = new LLD::BinaryLRU(512);

    for(int i = 0; i < 1000000; ++i)
    {

        uint256_t hashKey = LLC::GetRand256();

        uint512_t hashData = LLC::GetRand512();

        cache->Put(hashKey.GetBytes(), hashData.GetBytes());

        std::vector<uint8_t> vBytes;
        if(!cache->Get(hashKey.GetBytes(), vBytes))
            continue;

        uint512_t hashData2;
        hashData2.SetBytes(vBytes);

        debug::log(0, "data=", hashData.ToString());
        debug::log(0, "data=", hashData2.ToString());
    }

    return 0;
}
