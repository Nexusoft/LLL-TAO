
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/enum.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Register Primitive Tests", "[operation]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //test account register
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::MEMPOOL, tx));

            //reset the streams
            tx.ssRegister.seek(0, STREAM::BEGIN);
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::WRITE, tx));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::ACCOUNT);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance") == 0);
            REQUIRE(object.get<uint32_t>("identifier") == 0);
        }
    }


    //test trust register
    {
        //object register address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::MEMPOOL, tx));

            //reset the streams
            tx.ssRegister.seek(0, STREAM::BEGIN);
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::WRITE, tx));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::TRUST);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance") == 0);
            REQUIRE(object.get<uint64_t>("trust")   == 0);
            REQUIRE(object.get<uint32_t>("identifier") == 0);
        }
    }


    //test token register
    {
        //erase identifier
        LLD::regDB->EraseIdentifier(55);

        //object register address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(55)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(10);

            //run tests
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::MEMPOOL, tx));

            //reset the streams
            tx.ssRegister.seek(0, STREAM::BEGIN);
            REQUIRE(Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::WRITE, tx));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::TOKEN);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance")    == 5555);
            REQUIRE(object.get<uint64_t>("supply")     == 5555);
            REQUIRE(object.get<uint32_t>("identifier") == 55);
            REQUIRE(object.get<uint64_t>("digits")     == 10);
        }

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(55)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(10);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(55);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("trust")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                   << std::string("stake")      << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(55)
                   << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(55);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(0)
                    << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(55)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(10);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }


    //check for incorrect register values
    {
        //object account address
        uint256_t hashAddress = LLC::GetRand256();

        {
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = LLC::GetRand256();

            TAO::Register::Object object;
            object  << std::string("balance")    << uint8_t(TYPES::MUTABLE)  << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("identifier") << uint8_t(TYPES::UINT32_T) << uint32_t(0)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(5555)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(10);

            //run tests
            REQUIRE(!Register(hashAddress, REGISTER::OBJECT, object.GetState(), FLAGS::PRESTATE | FLAGS::POSTSTATE, tx));
        }
    }
}
