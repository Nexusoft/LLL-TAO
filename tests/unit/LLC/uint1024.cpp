/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/types/bignum.h>
#include <LLC/include/random.h>
#include <unit/catch2/catch.hpp>


using namespace LLC;

TEST_CASE( "Base Uint Compact Tests", "[LLC]" )
{
    for(int i = 0; i < 10000; ++i)
    {
        uint1024_t bn1 = GetRand1024();
        CBigNum bn2(bn1);

        uint32_t c1 = bn1.GetCompact();
        uint32_t c2 = bn2.GetCompact();

        /* Test GetCompact */
        REQUIRE(c1 == c2);

        /* Test SetCompact */
        bn1.SetCompact(c1);
        bn2.SetCompact(c2);

        REQUIRE( bn1 == bn2.getuint1024());

        /* Test Random SetCompact */
        c1 = GetRand();

        bn1.SetCompact(c1);
        bn2.SetCompact(c1);

        REQUIRE(bn1 == bn2.getuint1024());

    }
}
