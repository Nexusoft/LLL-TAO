/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <stdexcept>

#include <Util/include/debug.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <LLD/keychain/hashmap.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/templates/sector.h>

#include <Util/include/hex.h>


#include <TAO/Register/types/stream.h>
#include <TAO/Operation/types/stream.h>


namespace LLD
{
    class TestDB : public SectorDatabase<BinaryHashTree, BinaryLRU>
    {
    public:

        TestDB()
        : SectorDatabase("testdb", FLAGS::CREATE | FLAGS::FORCE | FLAGS::WRITE)
        {

        }

        bool WriteHash(const uint1024_t& a, const uint1024_t& b)
        {
            return Write(a, b);
        }

        bool ReadHash(const uint1024_t& a, uint1024_t &b)
        {
            return Read(a, b);
        }
    };
}


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{


/*
    {
        std::fstream ssFile = std::fstream("/home/viz/Downloads/image.png", std::ios::app | std::ios::binary);
        if(!ssFile.is_open())
            return debug::error("Failed to read file");

        //ssFile.ignore(std::numeric_limits<std::streamsize>::max());
        //uint32_t nSize = static_cast<int32_t>(ssFile.gcount());

        DataStream ssData(SER_LLD, LLD::DATABASE_VERSION);
        ssData << hashAddress;

        //ssFile.seekp(nSize, std::ios::beg);

        debug::log(0, "Writing at ", ssFile.tellp());


        //ssFile.write((char*)&ssData.Bytes(), ssData.Bytes().size());
        //ssFile.flush();

        const std::vector<uint8_t>& vBytes = ssData.Bytes();
        ssFile.write((char*)&vBytes[0], vBytes.size());
        if(ssFile.eof())
            return debug::error("Failed to write to file");

        debug::log(0, "Wrote Address ", hashAddress.ToString(), " Size ", vBytes.size());

        PrintHex(ssData.Bytes().begin(), ssData.Bytes().end());

        debug::log(0, "Writing at ", ssFile.tellp());

        ssFile.close();
    }
*/


    //return 0;

    {

        std::fstream ssFile = std::fstream("/home/viz/Downloads/image.png", std::ios::in | std::ios::out | std::ios::binary);
        if(!ssFile.is_open())
            return debug::error("Failed to read file");

        ///* Get the Binary Size. */
        ssFile.ignore(std::numeric_limits<std::streamsize>::max());
        uint32_t nSize = static_cast<int32_t>(ssFile.gcount());

        //debug::log(0, "File size is ", nSize);

        /* Read the keychain file. */
        ssFile.seekg (nSize - 32, std::ios::beg);

        debug::log(0, "Reading at ", ssFile.tellg());
        std::vector<uint8_t> vBinary(32, 0);
        ssFile.read((char*) &vBinary[0], vBinary.size());
        if(!ssFile)
            return debug::error("Failed to read from file");

        PrintHex(vBinary.begin(), vBinary.end());

        DataStream ssData(vBinary, SER_LLD, LLD::DATABASE_VERSION);
        uint256_t hashAddress2;
        ssData >> hashAddress2;

        debug::log(0, "Address Read ", hashAddress2.ToString());

        ssFile.close();

    }

    //uint256_t hashAddress2;
    //std::copy((uint8_t*)&vBinary[nSize - 33], (uint8_t*)&vBinary[nSize - 33] + 32, (uint8_t*)&hashAddress2);





    //ssFile.seekp (0, std::ios::beg);
    //ssFile.write((char*)&vBinary[0], vBinary.size());





    return 0;

    LLD::TestDB db;

    runtime::timer timer;
    timer.Start();

    uint1024_t hashRand = LLC::GetRand1024();

    uint32_t nCounter = 0;
    while(++nCounter)
    {
        if(nCounter % 100000 == 0)
        {
            debug::log(0, "100k written in ", timer.ElapsedMilliseconds(), " ms");

            nCounter = 0;
            timer.Reset();
        }


        db.WriteHash(hashRand, hashRand);

        ++hashRand;
    }


    return 0;

    /* This is the body for UNIT TESTS for prototyped code. */

    uint64_t nBase = 58384;

    uint32_t nBase2 = 0;

    memory::copy((uint8_t*)&nBase, (uint8_t*)&nBase + 4, (uint8_t*)&nBase2, (uint8_t*)&nBase2 + 4);

    debug::log(0, nBase2);


    return 0;
}
