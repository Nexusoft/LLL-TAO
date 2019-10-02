#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>


#include <unit/catch2/catch.hpp>

/* Test that contracts are calculating fees correctly */
TEST_CASE( "Transaction fee Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    SecureString strUsername = LLC::GetRand256().ToString().c_str();
    SecureString strPassword = "password";
    SecureString strPin = "1234";

    /* Create a sig chain to use for these tests */
    memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword);

    /* store the genesis hash for later use */
    uint256_t hashGenesis = user->Genesis();

    /* Generate a random hash for default account register address */
    TAO::Register::Address hashDefaultAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);


    /* Temporarily disable private mode so that fees are applicable */
    config::mapArgs["-private"] = "0";

    /* Keep a track of the transaction timestamps so that we can avoid paying tx fees for this test */
    uint64_t nTimestamp = runtime::unifiedtimestamp();

    /* Set the timestamp on this first transaction to be 600s in the past so that we can create some transactions for free 
           without hitting the tx fee limit*/
    nTimestamp -= 600;

    /* Create a default NXS account so that we can load it funds for the future tests */
    {
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        tx.nTimestamp = nTimestamp;

        /* Add a Name record for the trust account */
        TAO::Register::Address hashNameAddress = TAO::Register::Address("default", hashGenesis, TAO::Register::Address::NAME);

        //create name object
        Object name = CreateName("", "default", hashDefaultAccount);

        //payload
        tx[0] << uint8_t(OP::CREATE) << hashNameAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

        /* Add the default account register operation to the transaction */
        Object account = TAO::Register::CreateAccount(0);
        tx[1] << uint8_t(TAO::Operation::OP::CREATE) << hashDefaultAccount
                << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

        REQUIRE(tx.Build());
        REQUIRE(tx.Sign(user->Generate(tx.nSequence, strPin)));
        //REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        REQUIRE(TAO::Ledger::mempool.Accept(tx));

        //add balance to seed remaining tests
        REQUIRE(LLD::Register->ReadState(hashDefaultAccount, account, TAO::Ledger::FLAGS::MEMPOOL));
        account.Parse();
        account.Write("balance", uint64_t(50000000000));
        REQUIRE(account.get<uint64_t>("balance") == 50000000000);
        account.SetChecksum();

        REQUIRE(LLD::Register->WriteState(hashDefaultAccount, account, TAO::Ledger::FLAGS::MEMPOOL));
    }


    /* Test account fee*/
    {
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        nTimestamp += TAO::Ledger::TX_FEE_INTERVAL + 1;
        tx.nTimestamp = nTimestamp;

        TAO::Register::Object object = TAO::Register::CreateAccount(0);

        //build the tx
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

        /* run tests */

        /* Check the fee */
        REQUIRE(tx.Cost() == TAO::Ledger::ACCOUNT_FEE);

        REQUIRE(tx.Build());
        REQUIRE(tx.Sign(user->Generate(tx.nSequence, strPin)));
        //REQUIRE(tx.Verify());
        //REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        REQUIRE(TAO::Ledger::mempool.Accept(tx));

    }

    /* Test object fee */
    {
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        /* create object so that a fee is required */
        Object asset = CreateAsset();

        /* add some data */
        asset << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << std::string("somedata");

        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << asset.GetState();

        /* Check the fee */
        REQUIRE(tx.Cost() == TAO::Ledger::OBJECT_FEE);

    }

    /* Test name fee */
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        /* Generate Random name */
        std::string strName = LLC::GetRand256().ToString();

        /* create name so that a fee is required */
        TAO::Register::Address hashAddress = TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);
        Object name = CreateName("", strName, hashAddress);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

        /* Check the fee */
        REQUIRE(tx.Cost() == TAO::Ledger::NAME_FEE);

    }

    /* Test global name fee */
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        /* Generate Random name */
        std::string strName = LLC::GetRand256().ToString();

        /* create name so that a fee is required */
        TAO::Register::Address hashAddress = TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);
        Object name = CreateName(TAO::Register::NAMESPACE::GLOBAL, strName, hashAddress);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

        /* Check the fee */
        REQUIRE(tx.Cost() == TAO::Ledger::GLOBAL_NAME_FEE);
    }

    /* Test namespace fee */
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        /* Generate Random name */
        std::string strNamespace = LLC::GetRand256().ToString();

        /* create name so that a fee is required */
        TAO::Register::Address hashAddress = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);
        Object name = CreateNamespace(strNamespace);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

        /* Check the fee */
        REQUIRE(tx.Cost() == TAO::Ledger::NAMESPACE_FEE);
    }

    /* Test token fee - free for 100 units*/
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::TOKEN);
        /* Create token with 100 units */
        Object token = CreateToken(hashAddress, 100, 0);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << token.GetState();

        /* Token with 100 units - should be 1 NXS */
        REQUIRE(tx.Cost() == 1 * TAO::Ledger::NXS_COIN);
    }

    /* Test token fee - for 1000 units*/
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::TOKEN);

        /* Create token with 1000 units  */
        Object token = CreateToken(hashAddress, 1000, 0);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << token.GetState();

        /* Token with 1000 units - should be 100 NXS */
        REQUIRE(tx.Cost() == (100 * TAO::Ledger::NXS_COIN));
    }

    /* Test token fee - for 1000000 units*/
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::TOKEN);

        /* Create token with 1000000 units */
        Object token = CreateToken(hashAddress, 1000000, 0);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << token.GetState();

        /* Token with 1000 units - should be 200 NXS */
        REQUIRE(tx.Cost() == (400 * TAO::Ledger::NXS_COIN));
    }

    /* Test token fee - for 100000000000000 units*/
    {
        /* Create transaction  */
        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        tx.nTimestamp = nTimestamp + TAO::Ledger::TX_FEE_INTERVAL + 1;

        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::TOKEN);

        /* Create token with 100000000000000 units*/
        Object token = CreateToken(hashAddress, 100000000000000, 0);
        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << token.GetState();

        /* Token with 1000 units - should be 1200 NXS */
        REQUIRE(tx.Cost() == (1200 * TAO::Ledger::NXS_COIN));
    }

    /* Test transaction fee for creating transactions too fast*/
    {
        uint256_t hashAddress = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        tx.nTimestamp = nTimestamp;

        /* Create an account object */
        TAO::Register::Object object = TAO::Register::CreateAccount(0);

        /* Create 3 account registers, which should be free themselves */
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << object.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << object.GetState();
        tx[2] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << object.GetState();

        /* run tests */

        /* Check the fee.  This shuld be 3 lots of account fee (free) and 3 x TX_FEE (0.03) */
        REQUIRE(tx.Cost() == (3 *TAO::Ledger::ACCOUNT_FEE) + 3 * (TAO::Ledger::TX_FEE));

    }


    /* Test failure with not enough fee */
    {
        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 1s after the last so that there is a tx fee*/
        tx.nTimestamp = nTimestamp + 1;
        
        /* create object so that a fee is required */
        Object asset = CreateAsset();

        /* add some data */
        asset << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << std::string("somedata");

        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << asset.GetState();

        REQUIRE(tx.Build());
        REQUIRE(tx.Sign(user->Generate(tx.nSequence, strPin)));
        /* Check fail as we have provided no fees */
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));
        /* check for error */
        std::string error = debug::GetLastError();
        REQUIRE(error.find("not enough fees supplied") != std::string::npos);
    }

    /* Test Success case with correct fees applied */
    {
        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

        TAO::Ledger::Transaction tx;
        TAO::Ledger::CreateTransaction(user, strPin, tx);

        /* Force the timestamp to be 11s after the last so that there is no tx fee*/
        nTimestamp += TAO::Ledger::TX_FEE_INTERVAL + 1;
        tx.nTimestamp = nTimestamp;

        /* create object so that a fee is required */
        Object asset = CreateAsset();

        /* add some data */
        asset << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << std::string("somedata");

        /* Add the operations payload */
        tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << asset.GetState();

        /* Add a name so that a further fee is required */
        std::string strName = LLC::GetRand256().ToString();
        hashAddress = TAO::Register::Address(strName, hashGenesis, TAO::Register::Address::NAME);
        Object name = CreateName("", strName, hashAddress);
        /* Add the operations payload */
        tx[1] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << name.GetState();

        /* Add the fee */
        asset.Parse();
        name.Parse();
        uint64_t nRequiredFee = asset.Cost() + name.Cost();
        tx[2] << uint8_t(TAO::Operation::OP::FEE) << hashDefaultAccount << nRequiredFee;

        REQUIRE(tx.Build());
        REQUIRE(tx.Sign(user->Generate(tx.nSequence, strPin)));

        /* Check fail as we have provided no fees */
        REQUIRE(TAO::Ledger::mempool.Accept(tx));

    }




    /* Finally reinstate private mode so the rest of the tests can continue */
    config::mapArgs["-private"] = "1";

}
