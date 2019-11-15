/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLC/include/random.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/types/address.h>

#include <unit/catch2/catch.hpp>

#include <Util/include/debug.h>

TEST_CASE( "Signature Chain Generation", "[ledger]")
{
    /* TODO Test needs updating */
    // TAO::Ledger::SignatureChain user = TAO::Ledger::SignatureChain("user", "password");

    // REQUIRE(user.Genesis() == uint256_t("0xb5254d24183a77625e2dbe0c63570194aca6fb7156cb84edf3e238f706b51019"));

    // REQUIRE(user.Generate(0, "pin") == uint512_t("0x2986e8254e45ce87484feb0cbb9a961588dfe040bf109662f1235d97e57745fdcfae12ed46ba8a523bf490caf9461c9ef8dbc68bbbe8c62ea484ec0fc519f00c"));
}


TEST_CASE( "Signature Chain Genesis Transaction checks", "[sigchain]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    uint512_t hashPrivKey1  = LLC::GetRand512();
    uint512_t hashPrivKey2  = LLC::GetRand512();

    /* NOTE: We need ensure private mode is off for these tests so that the genesis transaction fees and default contracts
       can be correctly tested */
    config::mapArgs["-private"] = "0";

    /* Failure case adding invalid contracts to genesis, tokens are not allowed */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());
        TAO::Register::Address hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create object
        Object token = CreateToken(hashToken, 1000, 100);

        //payload
        tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("genesis transaction contains invalid contracts") != std::string::npos);

    }

    /* Failure case adding invalid contracts to genesis - too many names */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());
        TAO::Register::Address hashAccount     = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        TAO::Register::Address hashNameAddress = TAO::Register::Address("test", hashGenesis, TAO::Register::Address::NAME);
        //create name object pointing to account address
        Object name = TAO::Register::CreateName("", "test", hashAccount);

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address("test", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address("test2", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();
        tx[2] << uint8_t(OP::CREATE) << TAO::Register::Address("test3", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("genesis transaction contains invalid contracts") != std::string::npos);

    }

    /* Failure case adding invalid contracts to genesis - too many trust accounts */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create trust account object
        Object trust = TAO::Register::CreateTrust();

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address("trust1", hashGenesis, TAO::Register::Address::TRUST) << uint8_t(REGISTER::OBJECT) << trust.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address("trust2", hashGenesis, TAO::Register::Address::TRUST) << uint8_t(REGISTER::OBJECT) << trust.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("genesis transaction contains invalid contracts") != std::string::npos);

    }

    /* Failure case adding invalid contracts to genesis - too many accounts */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create account object
        Object account = TAO::Register::CreateAccount(0);

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT)<< uint8_t(REGISTER::OBJECT) << account.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << account.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("genesis transaction contains invalid contracts") != std::string::npos);

    }

    /* Failure case adding invalid contracts to genesis - too many crypto objects */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create crypto object
        Object crypto = TAO::Register::CreateCrypto(0,0,0,0,0,0,0,0,0);

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(std::string("crypto1"), hashGenesis, TAO::Register::Address::CRYPTO) << uint8_t(REGISTER::OBJECT) << crypto.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address(std::string("crypto2"), hashGenesis, TAO::Register::Address::CRYPTO) << uint8_t(REGISTER::OBJECT) << crypto.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("genesis transaction contains invalid contracts") != std::string::npos);

    }

    /* Failure case correct number and type of contracts but with a global name which causes a high fee */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);


        /* create account object */
        Object account = TAO::Register::CreateAccount(0);

        /* Create trust object */
        Object trust = TAO::Register::CreateTrust();

        /* Create Crypto */
        Object crypto = TAO::Register::CreateCrypto(0,0,0,0,0,0,0,0,0);

        //create name object pointing to account address
        Object name = TAO::Register::CreateName(TAO::Register::NAMESPACE::GLOBAL, "global name", TAO::Register::Address(TAO::Register::Address::ACCOUNT));
        TAO::Register::Address hashGlobalNamespace = TAO::Register::Address(TAO::Register::NAMESPACE::GLOBAL, TAO::Register::Address::NAMESPACE);

        Object name2 = TAO::Register::CreateName("", "test2", TAO::Register::Address(TAO::Register::Address::ACCOUNT));

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << account.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address("trust", hashGenesis, TAO::Register::Address::TRUST) << uint8_t(REGISTER::OBJECT) << trust.GetState();
        tx[2] << uint8_t(OP::CREATE) << TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO) << uint8_t(REGISTER::OBJECT) << crypto.GetState();
        tx[3] << uint8_t(OP::CREATE) << TAO::Register::Address("global name", hashGlobalNamespace, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();
        tx[4] << uint8_t(OP::CREATE) << TAO::Register::Address("test2", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name2.GetState();



        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

        //check for error
        std::string error = debug::GetLastError();
        REQUIRE(error.find("not enough fees supplied") != std::string::npos);

    }

    /* Success case - 1 account, 1 trust, 1 crypto 2 names */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("testuser" +std::to_string(LLC::GetRand())).c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);


        /* create account object */
        Object account = TAO::Register::CreateAccount(0);

        /* Create trust object */
        Object trust = TAO::Register::CreateTrust();

        /* Create Crypto */
        Object crypto = TAO::Register::CreateCrypto(0,0,0,0,0,0,0,0,0);

        //create name object pointing to account address
        Object name = TAO::Register::CreateName("", "test", TAO::Register::Address(TAO::Register::Address::ACCOUNT));
        Object name2 = TAO::Register::CreateName("", "test2", TAO::Register::Address(TAO::Register::Address::ACCOUNT));

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << account.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address("trust", hashGenesis, TAO::Register::Address::TRUST) << uint8_t(REGISTER::OBJECT) << trust.GetState();
        tx[2] << uint8_t(OP::CREATE) << TAO::Register::Address("crypto", hashGenesis, TAO::Register::Address::CRYPTO) << uint8_t(REGISTER::OBJECT) << crypto.GetState();
        tx[3] << uint8_t(OP::CREATE) << TAO::Register::Address("test", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();
        tx[4] << uint8_t(OP::CREATE) << TAO::Register::Address("test2", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name2.GetState();



        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE(TAO::Ledger::mempool.Accept(tx));

    }


    /* Success case - 1 account, 1 trust, 1 crypto 2 names */
    {
        uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis(std::string("recovery").c_str());

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);


        /* create account object */
        Object account = TAO::Register::CreateAccount(0);

        /* Create trust object */
        Object trust = TAO::Register::CreateTrust();

        /* Create Crypto */
        Object crypto = TAO::Register::CreateCrypto(0,0,0,0,0,0,0,0,0);

        //create name object pointing to account address
        Object name = TAO::Register::CreateName("", "test", TAO::Register::Address(TAO::Register::Address::ACCOUNT));
        Object name2 = TAO::Register::CreateName("", "test2", TAO::Register::Address(TAO::Register::Address::ACCOUNT));

        //payload
        tx[0] << uint8_t(OP::CREATE) << TAO::Register::Address(TAO::Register::Address::ACCOUNT) << uint8_t(REGISTER::OBJECT) << account.GetState();
        tx[1] << uint8_t(OP::CREATE) << TAO::Register::Address("trust", hashGenesis, TAO::Register::Address::TRUST) << uint8_t(REGISTER::OBJECT) << trust.GetState();
        tx[2] << uint8_t(OP::CREATE) << TAO::Register::Address("crypto", hashGenesis, TAO::Register::Address::CRYPTO) << uint8_t(REGISTER::OBJECT) << crypto.GetState();
        tx[3] << uint8_t(OP::CREATE) << TAO::Register::Address("test", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name.GetState();
        tx[4] << uint8_t(OP::CREATE) << TAO::Register::Address("test2", hashGenesis, TAO::Register::Address::NAME) << uint8_t(REGISTER::OBJECT) << name2.GetState();



        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        REQUIRE(TAO::Ledger::mempool.Accept(tx));

    }


    /* Finally reinstate private mode so the rest of the tests can continue */
    config::mapArgs["-private"] = "1";
}
