/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <TAO/Register/include/basevm.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Base VM allocation tests", "[basevm]" )
{
    TAO::Register::BaseVM registers;


    TAO::Register::Value test;
    uint256_t hashing = LLC::GetRand256();
    registers.allocate(hashing, test);
    REQUIRE(registers.available() == 480);


    uint256_t random = LLC::GetRand256();
    TAO::Register::Value test2;
    registers.allocate(random, test2);
    REQUIRE(registers.available() == 448);


    TAO::Register::Value test3;
    uint64_t nTest = 48384839483948;
    registers.allocate(nTest, test3);
    REQUIRE(registers.available() == 440);


    uint64_t nTest33;
    registers.deallocate(nTest33, test3);
    REQUIRE(nTest == nTest33);


    uint256_t hash333;
    registers.deallocate(hash333, test2);
    REQUIRE(hash333 == random);


    uint256_t hashing2;
    registers.deallocate(hashing2, test);
    REQUIRE(hashing == hashing2);
    REQUIRE(registers.available() == 512);


    std::string str = "this is a long form test string to make sure no corruption";
    registers.allocate(str, test);


    std::string str2(test.size() * 8, 0);
    registers.deallocate(str2, test);
    REQUIRE(str2.find(str) != std::string::npos);


    registers.allocate(str2, test);
    std::string str3(test.size() * 8, 0);
    registers.deallocate(str3, test);


    REQUIRE(str2 == str3);
    REQUIRE(registers.available() == 512);


    std::vector<uint8_t> vData = hashing.GetBytes();
    registers.allocate(vData, test);

    std::vector<uint8_t> vData2(test.size() * 8, 0);
    registers.deallocate(vData2, test);


    REQUIRE(vData == vData2);
}
