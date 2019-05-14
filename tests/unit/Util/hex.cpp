/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/hex.h>
#include <unit/catch2/catch.hpp>

TEST_CASE("Util hex tests", "[hex]")
{
    std::string isAllHex = "0123456789ABCDEF";
    std::string isNotAllHex = "!@#$%^&*()this is not all hex, but hey at least i tried";

    /* Test the IsHex boolean function. */
    REQUIRE(IsHex(isAllHex) == true);
    REQUIRE(IsHex(isNotAllHex) == false);



}
