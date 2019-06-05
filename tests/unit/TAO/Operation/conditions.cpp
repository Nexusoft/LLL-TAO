#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <Util/include/runtime.h>

#include <LLC/include/flkey.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/memory.h>

#include <LLC/include/random.h>

#include <openssl/rand.h>

#include <LLC/aes/aes.h>

#include <TAO/Operation/types/condition.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Operation/include/enum.h>

#include <LLD/include/global.h>

#include <TAO/Register/types/object.h>

#include <cmath>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Validation Script Tests", "[operation]" )
{
    using namespace TAO::Operation;

    uint256_t hashFrom = LLC::GetRand256();
    uint256_t hashTo   = LLC::GetRand256();
    uint64_t  nAmount  = 500;

    TAO::Ledger::Transaction tx;
    tx.nTimestamp  = 989798;
    tx.hashGenesis = LLC::GetRand256();
    tx[0] << (uint8_t)OP::DEBIT << hashFrom << hashTo << nAmount;

    const Contract& caller = tx[0];

    Contract contract = Contract();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= (uint32_t)7u <= (uint8_t) OP::MUL <= (uint8_t) OP::TYPES::UINT32_T <= (uint32_t)9u <= (uint8_t) OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= (uint32_t)63u;


    uint256_t hash = LLC::GetRand256();
    uint256_t hash2 = LLC::GetRand256();
    TAO::Register::State state;
    state.hashOwner = LLC::GetRand256();
    state.nType     = 2;
    state << hash2;


    REQUIRE(LLD::Register->Write(hash, state));
    REQUIRE(LLD::Register->Write(hashFrom, state));

    std::string strName = "colasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfin!!!";
    uint256_t hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));





    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= strName <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::STRING <= strName;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= strName <= (uint8_t)OP::CRYPTO::SK256 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT256_T <= hashRegister;

    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    uint512_t hashRegister2 = LLC::SK512(strName.begin(), strName.end());

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= strName <= (uint8_t)OP::CRYPTO::SK512 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT512_T <= hashRegister2;


    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }




    ///////////////////EXCEPTIONS
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(0) <= (uint8_t) OP::SUB <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(100) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::SUB 64-bit value overflow"));
        }
    }


    ///////////////////EXCEPTIONS
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(555) <= (uint8_t) OP::SUB <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(std::numeric_limits<uint64_t>::max()) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::SUB 64-bit value overflow"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= std::numeric_limits<uint64_t>::max() <= (uint8_t) OP::ADD <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(100) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::ADD 64-bit value overflow"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= std::numeric_limits<uint64_t>::max() <= (uint8_t) OP::DIV <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(0) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::DIV cannot divide by zero"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= std::numeric_limits<uint64_t>::max() <= (uint8_t) OP::MOD <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(0) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::MOD cannot divide by zero"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(std::numeric_limits<uint64_t>::max()) <= (uint8_t) OP::INC <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= uint32_t(222);
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::INC 64-bit value overflow"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(0) <= (uint8_t) OP::DEC <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= uint32_t(222);
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::DEC 64-bit value overflow"));
        }
    }



    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(555) <= (uint8_t) OP::EXP <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(9999) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::EXP 64-bit value overflow"));
        }
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(555323423434433443) <= (uint8_t) OP::MUL <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(2387438283734234423) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        std::string strException = "";
        Condition script = Condition(contract, caller);
        try
        {
            if(script.Execute())
                REQUIRE(false);
        }
        catch(const std::runtime_error& e)
        {
            REQUIRE(e.what() == std::string("OP::MUL 64-bit value overflow"));
        }
    }





    //////////////COMPARISONS
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT256_T <= hash;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::LESSTHAN <= (uint8_t)OP::TYPES::UINT256_T <= hash + 1;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::GREATERTHAN <= (uint8_t)OP::TYPES::UINT256_T <= hash - 1;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }





    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::NOTEQUALS <= (uint8_t)OP::TYPES::UINT256_T <= hash2;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::NOTEQUALS <= (uint8_t)OP::TYPES::UINT256_T <= hash + 1;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }




    /////CONTAINS
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("bear out");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("is");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("atomic bear out");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("atomic fox");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(!script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("atomic") <=
    (uint8_t)OP::AND <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("bear");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("fox and bear") <=
    (uint8_t)OP::OR <= (uint8_t)OP::TYPES::STRING <= std::string("is there an atomic bear out there?") <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::STRING <= std::string("atomic bear");
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    Stream ssCompare;
    ssCompare << (uint8_t)OP::DEBIT << uint256_t(0) << uint256_t(0) << nAmount;

    contract.Clear();
    contract <= (uint8_t)OP::CALLER::OPERATIONS <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::BYTES <= ssCompare.Bytes();
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    ssCompare.SetNull();
    ssCompare << (uint8_t)OP::DEBIT << uint256_t(0) << uint256_t(5) << nAmount;

    contract.Clear();
    contract <= (uint8_t)OP::CALLER::OPERATIONS <= (uint8_t)OP::CONTAINS <= (uint8_t)OP::TYPES::BYTES <= ssCompare.Bytes();
    {
        Condition script = Condition(contract, caller);
        REQUIRE(!script.Execute());
    }




    ////SUBDATA
    contract.Clear();
    contract <= (uint8_t)OP::CALLER::OPERATIONS <= (uint8_t)OP::SUBDATA <= uint16_t(1) <= uint16_t(32) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT256_T <= hashFrom;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
        REQUIRE(script.available() == 512);
    }



    contract.Clear();
    contract <= (uint8_t)OP::CALLER::OPERATIONS <= (uint8_t)OP::SUBDATA <= uint16_t(1) <= uint16_t(32) <= (uint8_t)OP::REGISTER::STATE <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT256_T <= hash2;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }







    ////////////////////OBJECT REGISTERS
    {
        using namespace TAO::Register;

        Object object;
        object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
               << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
               << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
               << std::string("balance") << uint8_t(TYPES::UINT64_T) << uint64_t(55)
               << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

       object.hashOwner = LLC::GetRand256();
       object.nType     = TAO::Register::REGISTER::OBJECT;


       std::string strObject = "register-vanity";

       uint256_t hashObject = LLC::SK256(std::vector<uint8_t>(strObject.begin(), strObject.end()));
       REQUIRE(LLD::Register->Write(hashObject, object));


      contract.Clear();
      contract <= uint8_t(OP::TYPES::STRING) <= strObject <= uint8_t(OP::CRYPTO::SK256)
                  <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= hashObject;

      {
          Condition script = Condition(contract, caller);
          REQUIRE(script.Execute());
      }

       contract.Clear();
       contract <= uint8_t(OP::TYPES::UINT256_T) <= hashObject <= uint8_t(OP::REGISTER::VALUE) <= std::string("byte")
                   <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT8_T) <= uint8_t(55);
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }


       contract.Clear();
       contract <= uint8_t(OP::TYPES::UINT256_T) <= hashObject <= uint8_t(OP::REGISTER::VALUE) <= std::string("test")
                   <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::STRING) <= std::string("this string");
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }


       contract.Clear();
       contract <= uint8_t(OP::TYPES::UINT256_T) <= hashObject <= uint8_t(OP::REGISTER::VALUE) <= std::string("test")
                   <= uint8_t(OP::CONTAINS) <= uint8_t(OP::TYPES::STRING) <= std::string("this");
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }


       contract.Clear();
       contract <= uint8_t(OP::TYPES::UINT256_T) <= hashObject <= uint8_t(OP::REGISTER::VALUE) <= std::string("balance")
                   <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(55);
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }


       contract.Clear();
       contract <= uint8_t(OP::TYPES::UINT256_T) <= hashObject <= uint8_t(OP::REGISTER::VALUE) <= std::string("token")
                   <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= uint256_t(0);
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }


       contract.Clear();
       contract <= uint8_t(OP::TYPES::STRING) <= strObject <= uint8_t(OP::CRYPTO::SK256) <= uint8_t(OP::REGISTER::VALUE) <= std::string("token")
                   <= uint8_t(OP::EQUALS) <= uint8_t(OP::TYPES::UINT256_T) <= uint256_t(0);
       {
           Condition script = Condition(contract, caller);
           REQUIRE(script.Execute());
       }
    }









    /////REGISTERS
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::REGISTER::STATE <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT256_T <= hash2;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::REGISTER::OWNER <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT256_T <= state.hashOwner;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::REGISTER::MODIFIED <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 3u <= (uint8_t)OP::GREATERTHAN <= (uint8_t) OP::GLOBAL::UNIFIED;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT256_T <= hash <= (uint8_t)OP::REGISTER::TYPE <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t) 2;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }





    ///////LEDGER and CALLER
    contract.Clear();
    contract <= (uint8_t)OP::CALLER::TIMESTAMP <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t) 989798;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::CALLER::GENESIS <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT256_T <= tx.hashGenesis;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    TAO::Ledger::BlockState block;
    block.nTime          = 947384;
    block.nHeight        = 23030;
    block.nMoneySupply   = 39239;
    block.nChannelHeight = 3;
    block.nChainTrust    = 55;

    TAO::Ledger::ChainState::stateBest.store(block);


    contract.Clear();
    contract <= (uint8_t)OP::LEDGER::TIMESTAMP <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)947384;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::LEDGER::HEIGHT <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)23030;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::LEDGER::SUPPLY <= (uint8_t)OP::EQUALS <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)39239;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }





    ////////////COMPUTATION
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 555u <= (uint8_t) OP::SUB <= (uint8_t) OP::TYPES::UINT32_T <= 333u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9234837u <= (uint8_t) OP::SUB <= (uint8_t) OP::TYPES::UINT32_T <= 384728u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 8850109u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 92382983u <= (uint8_t) OP::SUB <= (uint8_t) OP::TYPES::UINT32_T <= 1727272u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 90655711u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }




    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 905u <= (uint8_t) OP::MOD <= (uint8_t) OP::TYPES::UINT32_T <= 30u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 5u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 837438372u <= (uint8_t) OP::MOD <= (uint8_t) OP::TYPES::UINT32_T <= 128328u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 98172u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 98247293u <= (uint8_t) OP::MOD <= (uint8_t) OP::TYPES::UINT32_T <= 2394839u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 58894u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= (uint64_t)1000000000000 <= (uint8_t) OP::DIV <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)30000 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 33333333u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= (uint64_t)23984729837429387 <= (uint8_t) OP::DIV <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)9238493893 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 2596173u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= (uint64_t)23948392849238 <= (uint8_t) OP::DIV <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)923239232 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 25939u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }





    contract.Clear();
    contract <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5) <= uint8_t(OP::EXP) <= (uint8_t) OP::TYPES::UINT64_T <= uint64_t(15) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(30517578125);
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 2u <= (uint8_t) OP::EXP <= (uint8_t) OP::TYPES::UINT32_T <= 8u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 256u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 3u <= (uint8_t) OP::EXP <= (uint8_t) OP::TYPES::UINT32_T <= 10u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 59049u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }






    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= (uint64_t)9837 <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT64_T <= (uint64_t)7878 <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }

    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT64_T <= (uint64_t)9837 <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::LESSTHAN <= (uint8_t)OP::TYPES::UINT256_T <= uint256_t(17716);
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }


    //////////////// AND / OR
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;
    contract <= (uint8_t)OP::AND;

    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;
    contract <= (uint8_t)OP::AND;

    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;
    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }



    //////////////// AND / OR
    contract.Clear();
    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;

    contract <= (uint8_t)OP::OR;

    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;

    contract <= (uint8_t)OP::AND;

    contract <= (uint8_t)OP::TYPES::UINT32_T <= 9837u <= (uint8_t) OP::ADD <= (uint8_t) OP::TYPES::UINT32_T <= 7878u <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 17715u;

    {
        Condition script = Condition(contract, caller);
        REQUIRE(!script.Execute());
    }




    //GROUPING
    contract.Clear();

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(9837)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(7878)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(17715);
    contract <= uint8_t(OP::UNGROUP);
    //)


    contract <= uint8_t(OP::AND);

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(556);

    contract <= uint8_t(OP::OR);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);
    contract <= uint8_t(OP::UNGROUP);
    //)


    contract <= uint8_t(OP::AND);


    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);

    contract <= uint8_t(OP::OR);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(588);
    contract <= uint8_t(OP::UNGROUP);
    //)

    {
        Condition script = Condition(contract, caller);
        REQUIRE(script.Execute());
    }







    contract.Clear();

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(9837)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(7878)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(17715);
    //)


    contract <= uint8_t(OP::AND);

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(556);

    contract <= uint8_t(OP::AND);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);
    contract <= uint8_t(OP::UNGROUP);
    //)


    contract <= uint8_t(OP::AND);


    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);

    contract <= uint8_t(OP::AND);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(588);
    contract <= uint8_t(OP::UNGROUP);
    //)

    {
        Condition script = Condition(contract, caller);
        REQUIRE(!script.Execute());

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("evaluate groups count incomplete") != std::string::npos);
    }




    contract.Clear();

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(9837)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(7878)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(17715);
    contract <= uint8_t(OP::UNGROUP);
    //)


    contract <= uint8_t(OP::AND);

    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(556);

    contract <= uint8_t(OP::OR);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);
    contract <= uint8_t(OP::UNGROUP);
    //)


    contract <= uint8_t(OP::OR);


    //(
    contract <= uint8_t(OP::GROUP);
    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(555);

    contract <= uint8_t(OP::AND);

    contract <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(333)
             <= uint8_t(OP::ADD)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(222)
             <= uint8_t(OP::EQUALS)
             <= uint8_t(OP::TYPES::UINT32_T) <= uint32_t(588);
    contract <= uint8_t(OP::UNGROUP);
    //)

    {
        Condition script = Condition(contract, caller);
        REQUIRE(!script.Execute());

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("cannot evaluate OP::OR with previous OP::AND") != std::string::npos);
    }



}
