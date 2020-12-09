/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Common/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stakepool.h>

#include <unit/catch2/catch.hpp>


TEST_CASE( "Stake Pool Tests", "[stakepool][stake]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Generate random genesis */
    uint256_t hashGenesis = TAO::Ledger::SignatureChain::Genesis("teststakepool");
    uint512_t hashPrivKey1  = Common::GetRand512();
    uint512_t hashPrivKey2  = Common::GetRand512();

    /* Generate trust addresses */
    uint256_t hashTrust = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    uint512_t hashPrevTx;

    //Unit tests don't test proof validation, so just need to assign values
    uint256_t hashProof = Common::GetRand256();
    uint64_t nTimeBegin = runtime::timestamp();
    uint64_t nTimeEnd = nTimeBegin + 60;

    /* Create trust account register to use for remaining tests */
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object trust = CreateTrust();
            trust << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << trust.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //Save tx to disk and save hash to use as prev for pool test tx */
            hashPrevTx = tx.GetHash();
            REQUIRE(LLD::Ledger->WriteTx(hashPrevTx, tx));
        }

        {
            //check the trust register
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashTrust, trust));

            //parse
            REQUIRE(trust.Parse());

            //check standards
            REQUIRE(trust.Standard() == OBJECTS::TRUST);
            REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //add some balance to the trust register
            trust.Write("balance", uint64_t(5000));
            REQUIRE(trust.get<uint64_t>("balance") == 5000);
            trust.SetChecksum();

            REQUIRE(LLD::Register->WriteState(hashTrust, trust));
        }

        {
            //verify update
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashTrust, trust));

            //parse
            REQUIRE(trust.Parse());

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 5000);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);
        }
    }

    /* Test failure case - tx must be pooled trust of pooled genesis */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //payload: Genesis op, coinstake reward
        tx[0] << uint8_t(OP::GENESIS) << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //Accept should fail because it is invalid operation (not pool trust or pool genesis)
        REQUIRE_FALSE(TAO::Ledger::stakepool.Accept(tx));
    }

    /* Test failure case - tx not signed fails tx.Check() */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //payload: Genesis op, proof, time start, time end, coinstake reward
        tx[0] << uint8_t(OP::GENESISPOOL) << hashProof << nTimeBegin << nTimeEnd << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //Accept should fail because it is invalid operation (not pool trust or pool genesis)
        REQUIRE_FALSE(TAO::Ledger::stakepool.Accept(tx));
    }

    /* Test failure case - missing previous transaction/bad hashLastTx */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.hashPrevTx  = 1;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //payload: Genesis op, proof, time start, time end, coinstake reward
        tx[0] << uint8_t(OP::GENESISPOOL) << hashProof << nTimeBegin << nTimeEnd << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //Accept should fail because it is invalid operation (not pool trust or pool genesis)
        REQUIRE_FALSE(TAO::Ledger::stakepool.Accept(tx));
    }

    /* Valid case - won't be accepted into pool because stake minter not running. As far as these tests can go. */
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //payload: Genesis op, proof, time start, time end, coinstake reward
        tx[0] << uint8_t(OP::GENESISPOOL) << hashProof << nTimeBegin << nTimeEnd << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //sign
        tx.Sign(hashPrivKey1);

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //Accept returns true after initial checks successful, but not added to pool b/c stake minter not running.
        //This is as far as basic unit tests can go.
        REQUIRE(TAO::Ledger::stakepool.Accept(tx));
    }
}
