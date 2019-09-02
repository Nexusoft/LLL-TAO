
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Register Primitive Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //test account register
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

            //build the tx
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

            //reset the streams
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance") == 0);
            REQUIRE(object.get<uint256_t>("token") == 0);
        }
    }


    //test trust register
    {
        /* random genesis */
        uint256_t hashGenesis = LLC::GetRand256();

        //object register address
        TAO::Register::Address hashTrust = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

               //build the tx
               tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << object.GetState();

               //run tests
               REQUIRE(tx.Build());
               REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

               //reset the streams
               REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashTrust, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::TRUST);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance") == 0);
            REQUIRE(object.get<uint64_t>("trust")   == 0);
            REQUIRE(object.get<uint256_t>("token") == 0);
        }
    }


    //test token register
    {
        //erase identifier
        LLD::Register->EraseIdentifier(55);

        //object register address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::TOKEN);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)   << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("token")      << uint8_t(TYPES::UINT256_T) << hashAddress
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T)  << uint64_t(5555)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T)  << uint8_t(10);

            //build the tx
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

            //reset the streams
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::TOKEN);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance")    == 5555);
            REQUIRE(object.get<uint64_t>("supply")     == 5555);
            REQUIRE(object.get<uint256_t>("token")     == hashAddress);
            REQUIRE(object.get<uint8_t>("decimals")     == 10);
        }

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("token")      << uint8_t(TYPES::UINT256_T) << hashAddress
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T) << uint8_t(10);

            //run tests
            REQUIRE(tx.Build());
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(55);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(55);

           //run tests
           REQUIRE(tx.Build());
           REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(55)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T) << uint8_t(10);

            //run tests
            REQUIRE(tx.Build());
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(0)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T) << uint8_t(10);

            //run tests
            REQUIRE(tx.Build());
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }
    }
}
