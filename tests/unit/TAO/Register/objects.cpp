/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <TAO/Register/types/object.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Object Register Tests", "[register]" )
{
    using namespace TAO::Register;

    {
        Object object;
        object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
               << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
               << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
               << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

        //parse object
        REQUIRE(object.Parse());

        //read test
        uint8_t nByte;
        REQUIRE(object.Read("byte", nByte));

        //check
        REQUIRE(nByte == uint8_t(55));

        //write test
        REQUIRE(object.Write("byte", uint8_t(98)));

        //check
        REQUIRE(object.Read("byte", nByte));

        //check
        REQUIRE(nByte == uint8_t(98));


        //string test
        std::string strTest;
        REQUIRE(object.Read("test", strTest));

        //check
        REQUIRE(strTest == std::string("this string"));

        //test fail
        REQUIRE(!object.Write("test", std::string("fail")));

        //test type fail
        REQUIRE(!object.Write("test", "fail"));

        //test write
        REQUIRE(object.Write("test", std::string("THIS string")));

        //test read
        REQUIRE(object.Read("test", strTest));

        //check
        REQUIRE(strTest == std::string("THIS string"));


        //vector test
        std::vector<uint8_t> vBytes;
        REQUIRE(object.Read("bytes", vBytes));

        //write test
        vBytes[0] = 0x00;
        REQUIRE(object.Write("bytes", vBytes));

        //read test
        std::vector<uint8_t> vCheck;
        REQUIRE(object.Read("bytes", vCheck));

        //check
        REQUIRE(vBytes == vCheck);

        //identifier
        uint32_t nIdentifier;
        REQUIRE(object.Read("identifier", nIdentifier));

        //check
        REQUIRE(nIdentifier == uint64_t(0));

        //check standards
        REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
               << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);


        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::TRUST);
        REQUIRE(object.Standard() != OBJECTS::TOKEN);
        REQUIRE(object.Base()     == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("current") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("testing") << uint8_t(TYPES::UINT64_T) << uint64_t(888888);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::NONSTANDARD);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888)
               << std::string("digits") << uint8_t(TYPES::UINT64_T) << uint64_t(100);

        //parse object
        REQUIRE(object.Parse());

        //check standards
        REQUIRE(object.Standard() == OBJECTS::TOKEN);
        REQUIRE(object.Base()     == OBJECTS::ACCOUNT);
    }


    {
        Object object;
        object << std::string("balance") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint64_t(0)
               << std::string("supply") << uint8_t(TYPES::UINT64_T) << uint64_t(888888)
               << std::string("digits") << uint8_t(TYPES::UINT64_T) << uint64_t(100);

        //parse object
        REQUIRE(!object.Parse());
    }


    {
        uint256_t hash256   = LLC::GetRand256();
        uint512_t hash512   = LLC::GetRand512();
        uint1024_t hash1024 = LLC::GetRand1024();

        std::vector<uint8_t> vBytes(15);
        RAND_bytes((uint8_t*)&vBytes[0], vBytes.size());

        Object object;
        object << std::string("uint8_t")    << uint8_t(TYPES::UINT8_T)    << uint8_t(55)
               << std::string("uint16_t")   << uint8_t(TYPES::UINT16_T)   << uint16_t(9383)
               << std::string("uint32_t")   << uint8_t(TYPES::UINT32_T)   << uint32_t(82384293823)
               << std::string("uint64_t")   << uint8_t(TYPES::UINT64_T)   << uint64_t(239482349023843984)
               << std::string("uint256_t")  << uint8_t(TYPES::UINT256_T)  << hash256
               << std::string("uint512_t")  << uint8_t(TYPES::UINT512_T)  << hash512
               << std::string("uint1024_t") << uint8_t(TYPES::UINT1024_T) << hash1024
               << std::string("string")     << uint8_t(TYPES::STRING)     << std::string("this is a string to test long forms")
               << std::string("bytes")      << uint8_t(TYPES::BYTES)      << vBytes;

        //parse object
        REQUIRE(object.Parse());

        //read byte
        uint8_t nByte;
        REQUIRE(object.Read("uint8_t", nByte));

        //check
        REQUIRE(nByte == uint8_t(55));

        //read short
        uint16_t nShort;
        REQUIRE(object.Read("uint16_t", nShort));

        //check
        REQUIRE(nShort == uint16_t(9383));

        //read int
        uint32_t nInt;
        REQUIRE(object.Read("uint32_t", nInt));

        //check
        REQUIRE(nInt == uint32_t(82384293823));

        //read 64-bit int
        uint64_t n64;
        REQUIRE(object.Read("uint64_t", n64));

        //check
        REQUIRE(n64 ==uint64_t(239482349023843984));

        //read 256
        uint256_t hashRead256;
        REQUIRE(object.Read("uint256_t", hashRead256));

        //check
        REQUIRE(hashRead256 == hash256);

        //read 256
        uint512_t hashRead512;
        REQUIRE(object.Read("uint512_t", hashRead512));

        //check
        REQUIRE(hashRead512 == hash512);

        //read 1024
        uint1024_t hashRead1024;
        REQUIRE(object.Read("uint1024_t", hashRead1024));

        //check
        REQUIRE(hashRead1024 == hash1024);

        //read string
        std::string strRead;
        REQUIRE(object.Read("string", strRead));

        //check
        REQUIRE(strRead == std::string("this is a string to test long forms"));

        //read bytes
        std::vector<uint8_t> vRead;
        REQUIRE(object.Read("bytes", vRead));

        //check
        REQUIRE(vRead == vBytes);
    }
}
