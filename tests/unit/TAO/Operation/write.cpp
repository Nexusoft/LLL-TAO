
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Write Primitive Tests", "[operation]" )
{
    using namespace TAO::Operation;

    //test object register writes
    {

        uint256_t hashGenesis = LLC::GetRand256();

        TAO::Register::Object object;
        object.hashOwner = hashGenesis;
        object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
               << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
               << std::string("bytes") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
               << std::string("balance") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

        //write the object register for testing
        uint256_t hash = LLC::GetRand256();
        REQUIRE(LLD::regDB->WriteState(hash, object));


        //create an operation stream to set values.
        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("byte") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

            //reset the streams
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check values all match
            REQUIRE(LLD::regDB->ReadState(hash, object));

            //should fail
            uint8_t nByte;
            REQUIRE(!object.Read("byte", nByte));

            //parse
            REQUIRE(object.Parse());

            //check values
            REQUIRE(object.get<uint8_t>("byte") == uint8_t(99));
            REQUIRE(object.get<std::string>("test") == std::string("this string"));
            REQUIRE(object.get<std::vector<uint8_t>>("bytes") == std::vector<uint8_t>(10, 0xff));
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(55));
            REQUIRE(object.get<uint256_t>("identifier") == uint256_t(0));

        }


        //create an operation stream to set values.
        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            Stream stream;
            stream << std::string("test") << uint8_t(OP::TYPES::STRING) << std::string("stRInGISNew");

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

            //reset the streams
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hash, object2));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(99));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));
            REQUIRE(object2.get<std::vector<uint8_t>>("bytes") == std::vector<uint8_t>(10, 0xff));
            REQUIRE(object2.get<uint64_t>("balance") == uint64_t(55));
            REQUIRE(object2.get<uint256_t>("identifier") == uint256_t(0));
        }

        //make sure reserved values fail
        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            Stream stream;
            stream << std::string("balance") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("identifier") << uint8_t(OP::TYPES::UINT256_T) << uint256_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));

        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("require") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("supply") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("digits") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("trust") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << hash << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }


        //check failure on system values
        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << uint256_t(0) << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << uint256_t(1) << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << uint256_t(55) << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }

        {
            //create the mock transaction object
            TAO::Ledger::Transaction tx;
            tx.nTimestamp  = 989798;
            tx.hashGenesis = hashGenesis;

            //build stream
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //build transaction object
            tx[0] << uint8_t(OP::WRITE) << uint256_t(255) << stream.Bytes();

            //run tests
            REQUIRE(tx.Build());
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
        }


    }
}
