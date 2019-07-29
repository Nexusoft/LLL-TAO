#include <Util/include/runtime.h>

#include <LLC/include/random.h>

#include <LLD/cache/binary_lru.h>

#include <LLD/include/global.h>

#include <Util/templates/datastream.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "LLD Benchamrks", "[LLD]")
{
    debug::log(0, "===== Begin Register Random Read / Write Benchmarks =====");

    //benchmarks
    uint256_t hash = LLC::GetRand256();
    {
        runtime::timer timer;
        timer.Start();

        TAO::Ledger::BlockState state;
        for(int i = 0; i < 100000; i++)
            LLD::Ledger->WriteBlock(uint1024_t(i), state);

        uint64_t nTime = timer.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Put::", ANSI_COLOR_RESET, "10k records in ", nTime, "ms (", (100000000) / nTime, ") per/s");
    }


    {
        runtime::timer timer;
        timer.Start();

        TAO::Ledger::BlockState state;
        for(int i = 0; i < 100000; i++)
            LLD::Ledger->ReadBlock(uint1024_t(i), state);

        uint64_t nTime = timer.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Get::", ANSI_COLOR_RESET, "10k records in ", nTime, "ms (", (100000000) / nTime, ") per/s");
    }


    debug::log(0, "===== End Ledger Random Read / Write Benchmarks =====\n");
}
