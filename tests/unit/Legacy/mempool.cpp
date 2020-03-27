/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/types/reservekey.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/sigchain.h>

#include <unit/catch2/catch.hpp>

#include <algorithm>

TEST_CASE( "Legacy mempool and memory sequencing tests", "[legacy]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Need to clear mempool in case other unit tests have added transactions.  This allows us to test the sequencing without
       having our tests affected by other transactions outside of this test. */
    std::vector<uint512_t> vExistingHashes;
    TAO::Ledger::mempool.List(vExistingHashes);

    /* Iterate existing mempool tx list and remove them all */
    for(auto& hash : vExistingHashes)
    {
        REQUIRE(TAO::Ledger::mempool.Remove(hash));
    }

    //handle out of order transactions
    {

        //vector to shuffle
        std::vector<TAO::Ledger::Transaction> vTX;

        //create object
        uint256_t hashGenesis     = TAO::Ledger::SignatureChain::Genesis("testuser");
        uint512_t hashCoinbaseTx  = 0;

        uint512_t hashPrivKey1    = LLC::GetRand512();
        uint512_t hashPrivKey2    = LLC::GetRand512();

        uint512_t hashPrevTx;

        TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
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
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis << uint64_t(1000 * TAO::Ledger::NXS_COIN) << (uint64_t)0;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(tx.Connect(TAO::Ledger::FLAGS::BLOCK));

            //set previous
            hashPrevTx = tx.GetHash();

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::stateGenesis.GetHash()));

            //set the hash
            hashCoinbaseTx = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(tx.Connect(TAO::Ledger::FLAGS::BLOCK));

            //set previous
            hashPrevTx = tx.GetHash();

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::stateGenesis.GetHash()));

        }


        //credit to account
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashCoinbaseTx << uint32_t(0) << hashAccount << hashGenesis << uint64_t(1000 * TAO::Ledger::NXS_COIN);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        {
            //random shuffle the list for sequencing
            std::random_shuffle(vTX.begin(), vTX.end());

            //accept all transactions in random ordering
            for(auto& tx : vTX)
            {
                REQUIRE(TAO::Ledger::mempool.Accept(tx));
            }

            //check mempool list sequencing
            std::vector<uint512_t> vHashes;
            REQUIRE(TAO::Ledger::mempool.List(vHashes));

            //commit memory pool transactions to disk
            for(auto& hash : vHashes)
            {
                TAO::Ledger::Transaction tx;
                REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

                LLD::Ledger->WriteTx(hash, tx);
                REQUIRE(tx.Verify());
                REQUIRE(tx.Connect(TAO::Ledger::FLAGS::BLOCK));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }
        }


        {
            //check states
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount, object));

            //parse
            REQUIRE(object.Parse());

            //verify balance
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(1000 * TAO::Ledger::NXS_COIN));
        }


        //start a memory transaction
        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL);


        //test success for legacy opeation
        uint512_t hashDependant1 = 0;
        TAO::Ledger::Transaction txDependant1;
        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(10 * TAO::Ledger::NXS_COIN);

            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //legacy payload
            Legacy::Script script;
            script.SetNexusAddress(address);

            //add to tx
            tx[0] << script;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(tx.Connect(TAO::Ledger::FLAGS::MEMPOOL));

            //set previous
            hashPrevTx = tx.GetHash();

            //set dependant
            hashDependant1 = tx.GetHash();
            txDependant1   = tx;

            //add to wallet
            REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));
        }


        {
            //check states
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object.Parse());

            //verify balance
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(990 * TAO::Ledger::NXS_COIN));
        }


        {
            //check states
            TAO::Register::Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::BLOCK));

            //parse
            REQUIRE(object.Parse());

            //verify balance
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(1000 * TAO::Ledger::NXS_COIN));
        }


        //create another output
        uint512_t hashDependant2 = 0;
        TAO::Ledger::Transaction txDependant2;
        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(10 * TAO::Ledger::NXS_COIN);

            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //legacy payload
            Legacy::Script script;
            script.SetNexusAddress(address);

            //add to tx
            tx[0] << script;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //set dependant
            hashDependant2 = tx.GetHash();
            txDependant2   = tx;

            //add to wallet
            REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));

        }


        //create the orphan
        Legacy::WalletTx wtx;
        {
            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //create a transaction
            Legacy::Script scriptPubKey;
            scriptPubKey.SetNexusAddress(address);

            //create sending vectorsss
            std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
            vecSend.push_back(make_pair(scriptPubKey, 9 * TAO::Ledger::NXS_COIN));

            //for change
            Legacy::ReserveKey changeKey(Legacy::Wallet::GetInstance());

            //create transaction
            int64_t nFees;
            REQUIRE(Legacy::Wallet::GetInstance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

            //check the inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE_FALSE(wtx.FetchInputs(inputs));

            //accept to mempool
            REQUIRE_FALSE(TAO::Ledger::mempool.Accept(wtx));
        }


        //abort the memory transaction
        LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL);


        //connect a dependant
        REQUIRE(TAO::Ledger::mempool.Accept(txDependant1));
        REQUIRE(TAO::Ledger::mempool.Accept(txDependant2));


        //check mempool list sequencing
        std::vector<uint512_t> vHashes;
        REQUIRE(TAO::Ledger::mempool.List(vHashes));

        //commit memory pool transactions to disk
        for(auto& hash : vHashes)
        {
            TAO::Ledger::Transaction tx;
            REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

            //write to disk
            LLD::Ledger->WriteTx(hash, tx);

            //verify states
            REQUIRE(tx.Verify());

            //connect states
            REQUIRE(tx.Connect(TAO::Ledger::FLAGS::BLOCK));

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::stateGenesis.GetHash()));

            //remove from pool
            REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
        }

        //check that exists
        REQUIRE(TAO::Ledger::mempool.Accept(wtx));
        REQUIRE(TAO::Ledger::mempool.Has(wtx.GetHash()));
    }
}
