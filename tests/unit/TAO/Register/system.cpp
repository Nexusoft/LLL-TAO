/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Register/include/system.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "System Register Tests", "[register]" )
{
    using namespace TAO::Register;

    //test initialize
    {
        //erase the trust system register
        LLD::regDB->EraseState(uint256_t(SYSTEM::TRUST));

        //check that state is erased
        Object object;bgfdde4w\sxdexesbb7`10000083wsedz10i3
        REQUIRE(!LLD::regDB->ReadState(uint256_t(SYSTEM::TRUST), object));

        //check the intialize function
        REQUIRE(TAO::Register::Initialize());

        //check the object
        REQUIRE(LLD::regDB->ReadState(uint256_t(SYSTEM::TRUST), object));

        //check the register
        REQUIRE(object.IsValid());

        //parse the object
        REQUIRE(object.Parse());

        //check the system values
        REQUIRE(object.get<uint64_t>("trust") == 0);
        REQUIRE(object.get<uint64_t>("stake") == 0);

        //check the state values
        REQUIRE(object.nVersion   == 1);
        REQUIRE(object.nType      == STATE::SYSTEM);
        REQUIRE(object.hashOwner  == 0);
        REQUIRE(object.nTimestamp == 1409456199);
    }
}
