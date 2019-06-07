#include <Util/include/runtime.h>

#include <LLC/include/random.h>

#include <LLD/cache/template_lru.h>

#include <LLD/include/version.h>

#include <Util/templates/datastream.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "Template LRU Benchmarks", "[LLD]")
{
    debug::log(0, "===== Begin Template LRU Benchmarks =====");

    //benchmarks
    LLD::TemplateLRU<uint256_t, std::string>* cache = new LLD::TemplateLRU<uint256_t, std::string>(1024);

    uint256_t hash = LLC::GetRand256();
    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            cache->Put(hash + i, std::string("data"));
        }

        uint64_t nTime = timer.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Put::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million records / second");
    }


    {
        runtime::timer timer;
        timer.Start();

        for(int i = 0; i < 1000000; i++)
        {
            std::string str;
            cache->Get(hash + i, str);
        }

        uint64_t nTime = timer.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Get::", ANSI_COLOR_RESET, 1000000.0 / nTime, " million records / second");
    }


    debug::log(0, "===== End Template LRU Benchmarks =====\n");
}
