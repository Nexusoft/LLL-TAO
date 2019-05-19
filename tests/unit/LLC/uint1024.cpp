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

TEST_CASE( "Base Uint Get Uint Tests", "[LLC]")
{
    uint1024_t a = std::numeric_limits<uint64_t>::max(); //GetRand();

    CBigNum b(a);

    REQUIRE(a.getuint32() == b.getuint32());
    REQUIRE((a+1).getuint32() == (b+1).getuint32());
    REQUIRE((a-1).getuint32() == (b-1).getuint32());

    REQUIRE((a+2).getuint32() == (b+2).getuint32());

}

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


TEST_CASE( "Base Uint Constants Tests", "[LLC]")
{
    /** Minimum channels difficulty. **/
    const CBigNum bnProofOfWorkLimit[] =
    {
        CBigNum(~uint1024_t(0) >> 5),
        CBigNum(20000000),
        CBigNum(~uint1024_t(0) >> 17)
    };


    /** Starting channels difficulty. **/
    const LLC::CBigNum bnProofOfWorkStart[] =
    {
        CBigNum(~uint1024_t(0) >> 7),
        CBigNum(25000000),
        CBigNum(~uint1024_t(0) >> 22)
    };


    /** Minimum prime zero bits (1016-bits). **/
    const CBigNum bnPrimeMinOrigins = CBigNum(~uint1024_t(0) >> 8);

////////////////////////////////////////////////////////////////////////////////

    /** Minimum channels difficulty. **/
    const uint1024_t nProofOfWorkLimit[] =
    {
        uint1024_t(~uint1024_t(0) >> 5),
        uint1024_t(20000000),
        uint1024_t(~uint1024_t(0) >> 17)
    };


    /** Starting channels difficulty. **/
    const uint1024_t nProofOfWorkStart[] =
    {
        uint1024_t(~uint1024_t(0) >> 7),
        uint1024_t(25000000),
        uint1024_t(~uint1024_t(0) >> 22)
    };


    /** Minimum prime zero bits (1016-bits). **/
    const uint1024_t nPrimeMinOrigins = uint1024_t(~uint1024_t(0) >> 8);

////////////////////////////////////////////////////////////////////////////////

    REQUIRE(bnProofOfWorkLimit[0].getuint1024() == nProofOfWorkLimit[0]);
    REQUIRE(bnProofOfWorkLimit[1].getuint1024() == nProofOfWorkLimit[1]);
    REQUIRE(bnProofOfWorkLimit[2].getuint1024() == nProofOfWorkLimit[2]);

    REQUIRE(bnProofOfWorkStart[0].getuint1024() == nProofOfWorkStart[0]);
    REQUIRE(bnProofOfWorkStart[1].getuint1024() == nProofOfWorkStart[1]);
    REQUIRE(bnProofOfWorkStart[2].getuint1024() == nProofOfWorkStart[2]);

    REQUIRE(bnPrimeMinOrigins.getuint1024() == nPrimeMinOrigins);
}


TEST_CASE( "Base Uint Math Tests", "[LLC]")
{
    for(uint32_t i = 0; i < 10000; ++i)
    {
        uint1024_t a1 = GetRand1024();
        uint1024_t b1 = GetRand1024();

        CBigNum a2(a1);
        CBigNum b2(b1);

        uint32_t r32 = GetRand(1024);
        uint64_t r64 = GetRand();

        /* Addition */
        REQUIRE( (a1 + b1) == (a2 + b2).getuint1024());
        REQUIRE( (a1 + r64) == (a2 + r64).getuint1024());
        REQUIRE( (r64 + b1) == (r64 + b2).getuint1024());

        REQUIRE( (a1 + b1).getuint32() == (a2 + b2).getuint32());

        /* Subtraction (no overflow) */
        if(b1 <= a1)
            REQUIRE( (a1 - b1) == (a2 - b2).getuint1024());

        if(r64 <= a1)
            REQUIRE( (a1 - r64) == (a2 - r64).getuint1024());

        uint1024_t t = b1;

        b1 = GetRand(r64);
        b2.setuint1024(b1);

        if(b1 <= r64)
            REQUIRE( (r64 - b1) == (r64 - b2).getuint1024());

        b1 = t;
        b2.setuint1024(b1);

        /* Multiply */
        REQUIRE( (a1 * b1) == (a2 * b2).getuint1024());
        REQUIRE( (a1 * r64) == (a2 * r64).getuint1024());
        REQUIRE( (r64 * b1) == (r64 * b2).getuint1024());

        /* Divide */
        REQUIRE( (a1 / b1) == (a2 / b2).getuint1024());
        REQUIRE( (a1 / r64) == (a2 / r64).getuint1024());

        /* Left shift. */
        REQUIRE( (a1 << r32) == (a2 << r32).getuint1024());
        REQUIRE( (b1 << r32) == (b2 << r32).getuint1024());

        /* Right shift. */
        REQUIRE( (a1 >> r32) == (a2 >> r32).getuint1024());
        REQUIRE( (b1 >> r32) == (b2 >> r32).getuint1024());


        a2.setuint1024(a1);

        uint32_t r16 = GetRand(1 << 16);

        /* Modulo 16-bit */
        if(r16 != 0)
            REQUIRE( (a1 % r16) == (a2 % r16).getuint32());


    }

}
