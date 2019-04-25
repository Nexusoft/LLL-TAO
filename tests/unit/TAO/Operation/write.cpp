
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Write Primitive Tests", "[operation]" )
{
    using namespace TAO::Operation;

    //test object register writes
    {
        //create the mock transaction object
        TAO::Ledger::Transaction tx;
        tx.nTimestamp  = 989798;
        tx.hashGenesis = LLC::GetRand256();

        TAO::Register::Object object;
        object.hashOwner = tx.hashGenesis;
        object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
               << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
               << std::string("bytes") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
               << std::string("balance") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(55)
               << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT32_T) << uint32_t(0);

        //write the object register for testing
        uint256_t hash = LLC::GetRand256();
        LLD::regDB = new LLD::RegisterDB();
        REQUIRE(LLD::regDB->WriteState(hash, object));

        //create an operation stream to set values.
        {
            Stream stream;
            stream << std::string("byte") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
            REQUIRE(Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::WRITE, tx));
        }

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
        REQUIRE(object.get<uint32_t>("identifier") == uint32_t(0));


        //create an operation stream to set values.
        {
            Stream stream;
            stream << std::string("test") << uint8_t(OP::TYPES::STRING) << std::string("stRInGISNew");

            //run the write operation.
            REQUIRE(Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
            REQUIRE(Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::WRITE, tx));
        }

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
        REQUIRE(object2.get<uint32_t>("identifier") == uint32_t(0));

        //make sure reserved values fail
        {
            Stream stream;
            stream << std::string("balance") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("identifier") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("require") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("supply") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("digits") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("trust") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            Stream stream;
            stream << std::string("stake") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(99);

            //run the write operation.
            REQUIRE(!Write(hash, stream.Bytes(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }


        //check failure on system values
        {
            //run the write operation.
            REQUIRE(!Write(0, std::vector<uint8_t>(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            //run the write operation.
            REQUIRE(!Write(1, std::vector<uint8_t>(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            //run the write operation.
            REQUIRE(!Write(55, std::vector<uint8_t>(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }

        {
            //run the write operation.
            REQUIRE(!Write(255, std::vector<uint8_t>(), tx.hashGenesis, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE, tx));
        }


    }
}
