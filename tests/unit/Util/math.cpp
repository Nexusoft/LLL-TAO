/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/math.h>
#include <unit/catch2/catch.hpp>

TEST_CASE("Math Unit Tests", "[math]")
{
    {
        //pow test case
        {
            uint64_t b = 1000;
            uint64_t p = 3;

            REQUIRE(math::pow(b, p) == 1000000000);
        }

        //pow test case
        {
            uint64_t b = 10;
            uint64_t p = 3;

            REQUIRE(math::pow(b, p) == 1000);
        }

        //pow test case
        {
            uint64_t b = 3;
            uint64_t p = 3;

            REQUIRE(math::pow(b, p) == 27);
        }

        //pow test case
        {
            uint64_t b = 7;
            uint64_t p = 4;

            REQUIRE(math::pow(b, p) == 2401);
        }

        //pow test case
        {
            uint64_t b = 55;
            uint64_t p = 4;

            REQUIRE(math::pow(b, p) == 9150625);
        }

        //pow test case
        {
            uint64_t b = 7;
            uint64_t p = 7;

            REQUIRE(math::pow(b, p) == 823543);
        }

        //pow test case
        {
            uint64_t b = 8;
            uint64_t p = 4;

            REQUIRE(math::pow(b, p) == 4096);
        }


        //pow test case
        {
            uint64_t b = 10;
            uint64_t p = 18;

            REQUIRE(math::pow(b, p) == 1000000000000000000);
        }


        //pow test case
        {
            uint64_t b = 8;
            uint64_t p = 18;

            REQUIRE(math::pow(b, p) == 0x40000000000000);
        }


        //pow test case
        {
            uint64_t b = 3;
            uint64_t p = 13;

            REQUIRE(math::pow(b, p) == 0x1853d3);
        }
    }


    {
        //log test case
        {
            uint64_t b = 10;
            uint64_t v = 100;

            REQUIRE(math::log(b, v) == 2);
        }


        //log test case
        {
            uint64_t b = 5;
            uint64_t v = 55;

            REQUIRE(math::log(b, v) == 2);
        }


        //log test case
        {
            uint64_t b = 3;
            uint64_t v = 33;

            REQUIRE(math::log(b, v) == 3);
        }


        //log test case
        {
            uint64_t b = 23;
            uint64_t v = 1000000;

            REQUIRE(math::log(b, v) == 4);
        }


        //log test case
        {
            uint64_t b = 3;
            uint64_t v = 234938;

            REQUIRE(math::log(b, v) == 11);
        }


        //log test case
        {
            uint64_t b = 10;
            uint64_t v = 8484839;

            REQUIRE(math::log(b, v) == 6);
        }
        

        //log test case
        {
            uint64_t b = 12;
            uint64_t v = 144;

            REQUIRE(math::log(b, v) == 2);
        }


        //log test case
        {
            uint64_t b = 4;
            uint64_t v = 99;

            REQUIRE(math::log(b, v) == 3);
        }


        //log test case
        {
            uint64_t b = 9;
            uint64_t v = 888;

            REQUIRE(math::log(b, v) == 3);
        }


        //log test case
        {
            uint64_t b = 44;
            uint64_t v = 159265;

            REQUIRE(math::log(b, v) == 3);
        }
    }
}
