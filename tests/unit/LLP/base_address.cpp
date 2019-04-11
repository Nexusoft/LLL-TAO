/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/base_address.h>

TEST_CASE( "LLP Tests", "[base_address]")
{
    LLP::BaseAddress a1, a2;

    a1.SetIP(std::string("192.168.0.1"));
    a1.SetPort(9325);

    a2.SetIP(std::string("192.168.0.2"));
    a2.SetPort(8325);

    /* Testing Logical Not */
    REQUIRE(a1 != a2);

    a1 = a2;

    /* Testing assignment and relational equivalance */
    REQUIRE(a1 == a2);
}
