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

TEST_CASE( "Compact", "[bignum]" )
{
    uint1024_t bn1 = GetRand1024();
    CBigNum bn2(bn1);

    std::vector<uint8_t> v1 = bn1.GetBytes();
    std::vector<uint8_t> v2 = bn2.getvch();


    REQUIRE(bn1.GetCompact() == bn2.GetCompact());


}
