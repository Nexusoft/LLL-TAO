#include <TAO/Ledger/types/sigchain.h>

#include <unit/catch2/catch.hpp>

#include <Util/include/debug.h>

TEST_CASE( "Signature Chain Benchmarks", "[sigchain]" )
{

    debug::log(0, "===== Begin Signature Chain Benchmarks =====");

    runtime::timer bench;
    bench.Reset();
    TAO::Ledger::SignatureChain user = TAO::Ledger::SignatureChain("user", "password");

    //time output
    uint64_t nTime = bench.ElapsedMilliseconds();
    debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Genesis::", ANSI_COLOR_RESET, "Created in ", nTime, " ms");

    REQUIRE(user.Genesis() == uint256_t("0xb5254d24183a77625e2dbe0c63570194aca6fb7156cb84edf3e238f706b51019"));

    bench.Reset();
    REQUIRE(user.Generate(0, "pin") == uint512_t("0x2986e8254e45ce87484feb0cbb9a961588dfe040bf109662f1235d97e57745fdcfae12ed46ba8a523bf490caf9461c9ef8dbc68bbbe8c62ea484ec0fc519f00c"));

    //time output
    nTime = bench.ElapsedMilliseconds();
    debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Generate::", ANSI_COLOR_RESET, "Created in ", nTime, " ms");

    debug::log(0, "===== End Signature Chain Benchmarks =====\n");
}
