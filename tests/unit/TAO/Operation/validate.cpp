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



TEST_CASE( "Validation Script Operation Tests", "[validation]" )
{
    using namespace TAO::Operation;

    Stream ssOperation;

    ssOperation << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)7u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << (uint32_t)2u << (uint8_t) OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << (uint32_t)49u;

    runtime::timer bench;
    bench.Reset();

    runtime::timer func;
    func.Reset();

    uint256_t hashFrom = LLC::GetRand256();
    uint256_t hashTo   = LLC::GetRand256();
    uint64_t  nAmount  = 500;

    TAO::Ledger::Transaction tx;
    tx.nTimestamp  = 989798;
    tx.hashGenesis = LLC::GetRand256();
    tx << (uint8_t)OP::DEBIT << hashFrom << hashTo << nAmount;


    uint256_t hash = LLC::GetRand256();
    uint256_t hash2 = LLC::GetRand256();
    TAO::Register::State state;
    state.hashOwner = LLC::GetRand256();
    state.nType     = 2;
    state << hash2;




    LLD::regDB = new LLD::RegisterDB();
    REQUIRE(LLD::regDB->Write(hash, state));
    REQUIRE(LLD::regDB->Write(hashFrom, state));

    std::string strName = "colasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfin!!!";
    uint256_t hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::STRING << strName;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK256 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hashRegister;

    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    uint512_t hashRegister2 = LLC::SK512(strName.begin(), strName.end());

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << strName << (uint8_t)OP::CRYPTO::SK512 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT512_T << hashRegister2;


    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }





    //////////////COMPARISONS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hash;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << hash + 1;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::GREATERTHAN << (uint8_t)OP::TYPES::UINT256_T << hash - 1;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }





    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash2;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::NOTEQUALS << (uint8_t)OP::TYPES::UINT256_T << hash + 1;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }




    /////CONTAINS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("bear out");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("is");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("atomic bear out");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("atomic fox");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(!script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("atomic") <<
    (uint8_t)OP::AND << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("bear");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("fox and bear") <<
    (uint8_t)OP::OR << (uint8_t)OP::TYPES::STRING << std::string("is there an atomic bear out there?") << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::STRING << std::string("atomic bear");
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    Stream ssCompare;
    ssCompare << (uint8_t)OP::DEBIT << uint256_t(0) << uint256_t(0) << nAmount;

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::CALLER::OPERATIONS << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::BYTES << ssCompare.Bytes();
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssCompare.SetNull();
    ssCompare << (uint8_t)OP::DEBIT << uint256_t(0) << uint256_t(5) << nAmount;

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::CALLER::OPERATIONS << (uint8_t)OP::CONTAINS << (uint8_t)OP::TYPES::BYTES << ssCompare.Bytes();
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(!script.Execute());
    }




    ////SUBDATA
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::CALLER::OPERATIONS << (uint8_t)OP::SUBDATA << uint16_t(1) << uint16_t(32) << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT256_T << hashFrom;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation << (uint8_t)OP::CALLER::OPERATIONS << (uint8_t)OP::SUBDATA << uint16_t(1) << uint16_t(32) << (uint8_t)OP::REGISTER::STATE << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << hash2;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }







    /////REGISTERS
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::REGISTER::STATE << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << hash2;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::REGISTER::OWNER << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << state.hashOwner;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::REGISTER::TIMESTAMP << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 3u << (uint8_t)OP::GREATERTHAN << (uint8_t) OP::GLOBAL::UNIFIED;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT256_T << hash << (uint8_t)OP::REGISTER::TYPE << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t) 2;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }





    ///////LEDGER and CALLER
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::CALLER::TIMESTAMP << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t) 989798;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::CALLER::GENESIS << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT256_T << tx.hashGenesis;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    TAO::Ledger::BlockState block;
    block.nTime          = 947384;
    block.nHeight        = 23030;
    block.nMoneySupply   = 39239;
    block.nChannelHeight = 3;
    block.nChainTrust    = 55;

    TAO::Ledger::ChainState::stateBest.store(block);


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::LEDGER::TIMESTAMP << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)947384;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::LEDGER::HEIGHT << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)23030;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::LEDGER::SUPPLY << (uint8_t)OP::EQUALS << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)39239;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }





    ////////////COMPUTATION
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 555u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 333u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 222u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9234837u << (uint8_t) OP::SUB << (uint8_t) OP::TYPES::UINT32_T << 384728u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 8850109u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }



    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 905u << (uint8_t) OP::MOD << (uint8_t) OP::TYPES::UINT32_T << 30u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 5u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 837438372u << (uint8_t) OP::MOD << (uint8_t) OP::TYPES::UINT32_T << 128328u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 98172u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)1000000000000 << (uint8_t) OP::DIV << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)30000 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 33333333u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)5 << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)15 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)30517578125;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 2u << (uint8_t) OP::EXP << (uint8_t) OP::TYPES::UINT32_T << 8u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 256u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT64_T << (uint64_t)7878 << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }

    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT64_T << (uint64_t)9837 << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::LESSTHAN << (uint8_t)OP::TYPES::UINT256_T << uint256_t(17716);
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }


    //////////////// AND / OR
    ssOperation.SetNull();
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    ssOperation << (uint8_t)OP::AND;
    ssOperation << (uint8_t)OP::TYPES::UINT32_T << 9837u << (uint8_t) OP::ADD << (uint8_t) OP::TYPES::UINT32_T << 7878u << (uint8_t)OP::EQUALS << (uint8_t)OP::TYPES::UINT32_T << 17715u;
    {
        Validate script = Validate(ssOperation, tx);
        REQUIRE(script.Execute());
    }
}
