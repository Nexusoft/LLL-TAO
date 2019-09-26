#include <TAO/Ledger/types/sigchain.h>

#include <unit/catch2/catch.hpp>

#include <Util/include/debug.h>

TEST_CASE( "Signature Chain Benchmarks", "[ledger]")
{

    debug::log(0, "===== Begin Signature Chain Benchmarks =====");

    runtime::timer bench;
    bench.Reset();
    TAO::Ledger::SignatureChain user = TAO::Ledger::SignatureChain("user", "password");

    //time output
    uint64_t nTime = bench.ElapsedMilliseconds();
    debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Genesis::", ANSI_COLOR_RESET, "Created in ", nTime, " ms");

    user.Genesis();

    bench.Reset();
    user.Generate(0, "pin");

    //time output
    nTime = bench.ElapsedMilliseconds();
    debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Generate::", ANSI_COLOR_RESET, "Created in ", nTime, " ms");

    debug::log(0, "===== End Signature Chain Benchmarks =====\n");
}
