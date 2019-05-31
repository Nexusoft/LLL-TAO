#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "OP::REGISTER  Benchmarks", "[operation]" )
{
    using namespace TAO::Operation;

    debug::log(0, "===== Begin OP::REGISTER Benchmarks =====");

    {
        std::vector<uint8_t> vData(32, 0xff);

        std::vector<TAO::Ledger::Transaction> vTx;
        for(int i = 0; i < 10000; ++i)
        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = LLC::GetRand256();
            tx << uint8_t(OP::REGISTER) << LLC::GetRand256() << uint8_t(TAO::Register::REGISTER::READONLY) << vData;

            REQUIRE(TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE));

            vTx.push_back(tx);
        }


        //Benchmark of 10,000 executions
        runtime::timer bench;
        bench.Reset();
        {
            LLD::TxnBegin();
            for(int i = 0; i < vTx.size(); ++i)
            {
                REQUIRE(TAO::Operation::Execute(vTx[i], TAO::Ledger::FLAGS::MEMPOOL));
            }
            LLD::TxnCommit();
        }

        //time output
        uint64_t nTime = bench.ElapsedMilliseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "OP::REGISTER", ANSI_COLOR_RESET, " Processed ", 10000000.0 / nTime, " ops / second");
    }

    debug::log(0, "===== End OP::REGISTER Benchmarks =====\n");

}
