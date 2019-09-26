#include <Util/include/runtime.h>

#include <LLC/include/random.h>

#include <LLD/cache/binary_key.h>

#include <LLD/include/version.h>

#include <Util/templates/datastream.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "Binary Key LRU Benchmarks", "[LLD]")
{
    debug::log(0, "===== Begin Binary Key LRU Benchmarks =====");

    //benchmarks
    LLD::KeyLRU* cache = new LLD::KeyLRU();
    uint256_t hash = LLC::GetRand256();
    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << std::make_pair(std::string("data"), hash + i);

            cache->Put(ssKey.Bytes());
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

            cache->Has(ssKey.Bytes());
        }

        uint64_t nTime = timer.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Has::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million records / second");
    }


    debug::log(0, "===== End Binary Key LRU Benchmarks =====\n");
}
