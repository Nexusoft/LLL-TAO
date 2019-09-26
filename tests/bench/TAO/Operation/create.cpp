#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "OP::CREATE  Benchmarks", "[operation]")
{
    using namespace TAO::Operation;

    debug::log(0, "===== Begin OP::CREATE Benchmarks =====");

    {
        std::vector<uint8_t> vData(32, 0xff);

        std::vector<TAO::Ledger::Transaction> vTx;
        for(int i = 0; i < 10000; ++i)
        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = LLC::GetRand256();
            tx[0] << uint8_t(OP::CREATE) << LLC::GetRand256() << uint8_t(TAO::Register::REGISTER::READONLY) << vData;

            REQUIRE(tx.Build());

            vTx.push_back(tx);
        }


        //Benchmark of 10,000 executions
        runtime::timer bench;
        bench.Reset();
        {
            LLD::TxnBegin();
            for(int i = 0; i < vTx.size(); ++i)
            {
                REQUIRE(vTx[i].Connect(TAO::Ledger::FLAGS::MEMPOOL));
            }
            LLD::TxnCommit();
        }

        //time output
        uint64_t nTime = bench.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "OP::CREATE", ANSI_COLOR_RESET, " Processed ", 10000000.0 / nTime, " ops / second");
    }

    debug::log(0, "===== End OP::CREATE Benchmarks =====\n");

}
