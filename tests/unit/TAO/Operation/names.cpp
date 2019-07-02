/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Names / Namespaces Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Test creating a name  */
    {
        uint256_t hashAddress   = LLC::GetRand256();
        uint256_t hashGenesis   = LLC::GetRand256();
        TAO::Register::Address hashNameAddress = 0;
        std::string strName = "somename";
        std::string strNamespace = "somenamespace";

        /* Test successfully creating a name in user local namespace */
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            TAO::Register::GetNameAddress(hashGenesis, strName, hashNameAddress);

            //create name object
            Object name = CreateName("", strName, hashAddress);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNameAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        /* Test that we are not allowed to create a name in another sig chain's namespace */
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            /* Generate an address for a random sig chain genesis */
            TAO::Register::GetNameAddress(LLC::GetRand256(), strName, hashNameAddress);

            //create name object
            Object name = CreateName("", strName, hashAddress);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNameAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            /* commit to disk.  This should fail as the register address of the name is not based on the tx.hashGenesis */
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        /* Test successfully creating a namespace in user local namespace */
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            /* Generate register address for namespace, which must be a hash of the name */
            uint256_t hashNamespace = LLC::SK256(strNamespace);

            //create name object
            Object namespaceObject = CreateNamespace(strNamespace);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNamespace << uint8_t(REGISTER::OBJECT) << namespaceObject.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        /* Test that the namespace hash must be a hash of the namespace name */
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            /* Generate register address for namespace based on a different name. */
            uint256_t hashNamespace = LLC::SK256("wrongnamespace");

            //create name object
            Object namespaceObject = CreateNamespace(strNamespace);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNamespace << uint8_t(REGISTER::OBJECT) << namespaceObject.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            /* commit to disk.  This should fail as the register address of the namespace is not based on the namespace name */
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        /* Test successfully creating a name in a namespace */
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.nTimestamp  = runtime::timestamp();

            /* full name is somename.somenamespace */

            /* Generate register address for namespace, which must be a hash of the name */
            uint256_t hashNamespace = LLC::SK256(strNamespace);

            /* Generate  */
            TAO::Register::GetNameAddress(hashNamespace, strName, hashNameAddress);

            //create name object
            Object name = CreateName(strNamespace, strName, hashAddress);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNameAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        /* Test that you are not allowed to create a name in a namespace you do not own*/
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = LLC::GetRand256(); // use random genesis here so that it is not the namespace owner
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            /* full name is somename.somenamespace */

            /* Generate register address for namespace, which must be a hash of the name */
            uint256_t hashNamespace = LLC::SK256(strNamespace);

            /* Generate  */
            TAO::Register::GetNameAddress(hashNamespace, strName, hashNameAddress);

            //create name object
            Object name = CreateName(strNamespace, strName, hashAddress);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNameAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            /* commit to disk.  This should fail as the sig chain is not the owner of the namespace */
            REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }
    }



}
