#include <Util/include/runtime.h>

#include <LLC/include/random.h>

#include <LLD/cache/binary_lru.h>

#include <LLD/include/global.h>

#include <Util/templates/datastream.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "LLD Benchamrks", "[LLD]")
{
    debug::log(0, "===== Begin Ledger Random Read / Write Benchmarks =====");

    //benchmarks
    {
        runtime::timer timer;
        timer.Start();

        TAO::Ledger::BlockState state;
        for(int i = 0; i < 100000; i++)
        {
            state.nHeight = i;
            state.hashMerkleRoot = LLC::GetRand512();
            REQUIRE(LLD::Ledger->WriteBlock(uint1024_t(i), state));
        }

        uint64_t nTime = timer.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Write::", ANSI_COLOR_RESET, "100k records in ", nTime, " ms (", (100000000) / nTime, ") per/s");
    }


    {
        runtime::timer timer;
        timer.Start();

        TAO::Ledger::BlockState state;
        for(int i = 0; i < 100000; i++)
        {
            if(!LLD::Ledger->ReadBlock(uint1024_t(i), state))
                continue; //debug::error("Failed to read ", i);
        }

        uint64_t nTime = timer.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Read::", ANSI_COLOR_RESET, "100k records in ", nTime, " ms (", (100000000) / nTime, ") per/s");
    }


    debug::log(0, "===== End Ledger Random Read / Write Benchmarks =====\n");


    debug::log(0, "===== Begin Ledger Sequential Read Benchmarks =====");


    {
        runtime::timer timer;
        timer.Start();

        std::vector<TAO::Ledger::BlockState> vStates;
        LLD::Ledger->BatchRead("block", vStates, 100000);

        uint64_t nTime = timer.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Get::", ANSI_COLOR_RESET, vStates.size(), " records in ", nTime, " ms (", (vStates.size() * 1000) / nTime, ") per/s");
    }


    debug::log(0, "===== End Ledger Sequential Read Benchmarks =====\n");
}
