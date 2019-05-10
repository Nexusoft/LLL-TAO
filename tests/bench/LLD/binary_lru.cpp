#include <Util/include/runtime.h>

#include <LLC/include/random.h>

#include <LLD/cache/binary_lru.h>

#include <LLD/include/version.h>

#include <Util/templates/datastream.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "Binary LRU Benchmarks", "[LLD]" )
{
    debug::log(0, "===== Begin Binary LRU Benchmarks =====");

    //benchmarks
    LLD::BinaryLRU* cache = new LLD::BinaryLRU();
    uint256_t hash = LLC::GetRand256();
    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << std::make_pair(std::string("data"), hash + i);

            DataStream ssData(SER_LLD, LLD::DATABASE_VERSION);
            ssData << uint1024_t(4934943);

            cache->Put(ssKey.Bytes(), ssData.Bytes());
        }

        uint64_t nTime = timer.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Put::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million records / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << std::make_pair(std::string("data"), hash + i);

            std::vector<uint8_t> vBytes;

            cache->Get(ssKey.Bytes(), vBytes);
        }

        uint64_t nTime = timer.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Get::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million records / second");
    }


    debug::log(0, "===== End Binary LRU Benchmarks =====\n");
}
