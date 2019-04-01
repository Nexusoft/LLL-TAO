#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>

#include <TAO/Operation/include/validate.h>

#include <TAO/Operation/include/enum.h>

#include <LLD/include/global.h>

#include <cmath>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Validation Script Operation Benchmarks", "[validation]" )
{
    using namespace TAO::Operation;

    debug::log(0, "===== Begin Validation Script Benchmarks =====");

    //random data for tx script
    uint256_t hashFrom = LLC::GetRand256();
    uint256_t hashTo   = LLC::GetRand256();
    uint64_t  nAmount  = 500;

    //tx object for validation script
    TAO::Ledger::Transaction tx;
    tx.nTimestamp  = 989798;
    tx.hashGenesis = LLC::GetRand256();
    tx << (uint8_t)OP::DEBIT << hashFrom << hashTo << nAmount;

    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(7) << uint8_t(OP::ADD) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(9) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(16);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "ADD::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }

    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(7) << uint8_t(OP::MUL) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(9) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(63);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "MUL::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(7) << uint8_t(OP::EXP) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(9) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(40353607);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "EXP::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(99) << uint8_t(OP::SUB) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(66) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(33);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "SUB::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(99) << uint8_t(OP::INC) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(100);

        //Total Operations
        uint32_t nOps = 4;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "INC::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(99) << uint8_t(OP::DEC) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(98);

        //Total Operations
        uint32_t nOps = 4;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "DEC::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(99) << uint8_t(OP::DIV) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(3) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(33);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "DIV::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::UINT32_T) << uint32_t(100) << uint8_t(OP::MOD) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(3) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::UINT32_T) << uint32_t(1);

        //Total Operations
        uint32_t nOps = 5;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "MOD::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }



    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::TYPES::STRING) << std::string("This is data to parse") << uint8_t(OP::SUBDATA) << uint16_t(5) << uint16_t(7) << uint8_t(OP::EQUALS) << uint8_t(OP::TYPES::STRING) << std::string("is data");

        //Total Operations
        uint32_t nOps = 4;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "SUBDATA::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    {
        //Operation stream
        Stream ssOperation;
        ssOperation << uint8_t(OP::GLOBAL::UNIFIED) << uint8_t(OP::EQUALS) << uint8_t(OP::GLOBAL::UNIFIED);

        //Total Operations
        uint32_t nOps = 3;

        //Benchmark of 1 million executions
        runtime::timer bench;
        bench.Reset();
        {
            Validate script = Validate(ssOperation, tx);
            for(int i = 0; i < 1000000; i++)
            {
                REQUIRE(script.Execute());
                script.Reset();
            }
        }

        //time output
        uint64_t nTime = bench.ElapsedMicroseconds();
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "UNIFIED::", ANSI_COLOR_RESET, "Processed ", nOps * 1000000.0 / nTime, " million ops / second");
    }


    debug::log(0, "===== End Validation Script Benchmarks =====\n");

}
