/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/legacy.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Condition variable for private blocks. */
        std::condition_variable PRIVATE_CONDITION;


        /* Create a new transaction object from signature chain. */
        bool CreateTransaction(TAO::Ledger::SignatureChain* user, SecureString pin, TAO::Ledger::Transaction& tx)
        {
            /* Get the last transaction. */
            uint512_t hashLast;
            if(LLD::legDB->ReadLast(user->Genesis(), hashLast))
            {
                /* Get previous transaction */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::legDB->ReadTx(hashLast, txPrev))
                {
                    return debug::error(FUNCTION, "no prev tx ", hashLast.ToString(), " in ledger db");
                }

                /* Build new transaction object. */
                tx.nSequence   = txPrev.nSequence + 1;
                tx.hashGenesis = txPrev.hashGenesis;
                tx.hashPrevTx  = hashLast;
                tx.NextHash(user->Generate(tx.nSequence + 1, pin));

                return true;
            }

            /* Genesis Transaction. */
            tx.NextHash(user->Generate(tx.nSequence + 1, pin));
            tx.hashGenesis = user->Genesis();

            return true;
        }


        /* Create a new block object from the chain.*/
        bool CreateBlock(TAO::Ledger::SignatureChain* user, SecureString pin, uint32_t nChannel, TAO::Ledger::TritiumBlock& block)
        {
            /* Set the block to null. */
            block.SetNull();


            /* Modulate the Block Versions if they correspond to their proper time stamp */
            if(runtime::unifiedtimestamp() >= (config::fTestNet ?
                TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] :
                NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
                block.nVersion = config::fTestNet ?
                TESTNET_BLOCK_CURRENT_VERSION :
                NETWORK_BLOCK_CURRENT_VERSION; // --> New Block Versin Activation Switch
            else
                block.nVersion = config::fTestNet ?
                TESTNET_BLOCK_CURRENT_VERSION - 1 :
                NETWORK_BLOCK_CURRENT_VERSION - 1;


            /* Setup the producer transaction. */
            if(!CreateTransaction(user, pin, block.producer))
                return debug::error(FUNCTION, "failed to create producer transactions");


            /* Handle the coinstake */
            if (nChannel == 0)
            {
                if(block.producer.IsGenesis())
                {
                    /* Create trust transaction. */
                    block.producer << (uint8_t) TAO::Operation::OP::TRUST;

                    /* The account that is being staked. */
                    uint256_t hashAccount;
                    block.producer << hashAccount;

                    /* The previous trust block. */
                    uint1024_t hashLastTrust = 0;
                    block.producer << hashLastTrust;

                    /* Previous trust sequence number. */
                    uint32_t nSequence = 0;
                    block.producer << nSequence;

                    /* The previous trust calculated. */
                    uint64_t nLastTrust = 0;
                    block.producer << nLastTrust;

                    /* The total to be staked. */
                    uint64_t  nStake = 0; //account balance
                    block.producer << nStake;
                }
                else
                {
                    /* Create trust transaction. */
                    block.producer << (uint8_t) TAO::Operation::OP::TRUST;

                    /* The account that is being staked. */
                    uint256_t hashAccount;
                    block.producer << hashAccount;

                    /* The previous trust block. */
                    uint1024_t hashLastTrust = 0; //GET LAST TRUST BLOCK FROM LOCAL DB
                    block.producer << hashLastTrust;

                    /* Previous trust sequence number. */
                    uint32_t nSequence = 0; //GET LAST SEQUENCE FROM LOCAL DB
                    block.producer << nSequence;

                    /* The previous trust calculated. */
                    uint64_t nLastTrust = 0; //GET LAST TRUST FROM LOCAL DB
                    block.producer << nLastTrust;

                    /* The total to be staked. */
                    uint64_t  nStake = 0; //BALANCE IS PREVIOUS BALANCE + INTEREST RATE
                    block.producer << nStake;
                }
            }

            /* Create the Coinbase Transaction if the Channel specifies. */
            else
            {
                /* Create coinbase transaction. */
                block.producer << (uint8_t) TAO::Operation::OP::COINBASE;

                /* The total to be credited. */
                uint64_t  nCredit = GetCoinbaseReward(ChainState::stateBest, nChannel, 0);
                block.producer << nCredit;

            }

            /* Sign the producer transaction. */
            block.producer.Sign(user->Generate(block.producer.nSequence, pin));

            /* Add the transactions. */
            std::vector<uint512_t> vHashes;
            vHashes.push_back(block.producer.GetHash());

            /* Check the memory pool. */
            mempool.List(vHashes);

            /* Add each transaction. */
            for(const auto& hash : vHashes)
            {
                /* Check the Size limits of the Current Block. */
                if (::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 193 >= MAX_BLOCK_SIZE)
                    break;

                /* Skip the producer hash if included. */
                if(hash == block.producer.GetHash())
                    continue;

                /* Add the transaction to the block. */
                block.vtx.push_back(std::make_pair(TRITIUM_TX, hash));
            }

            /* Erase the remaining hashes that didn't get onto a block. */
            vHashes.erase(vHashes.begin() + block.vtx.size() + 1, vHashes.end());

            /** Populate the Block Data. **/
            block.hashPrevBlock   = ChainState::stateBest.GetHash();
            block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
            block.nChannel       = nChannel;
            block.nHeight        = ChainState::stateBest.nHeight + 1;
            block.nBits          = GetNextTargetRequired(ChainState::stateBest, nChannel, false);
            block.nNonce         = 1;
            block.nTime          = runtime::unifiedtimestamp();

            return true;
        }


        /** Create Genesis
         *
         *  Creates the genesis block
         *
         *
         **/
        bool CreateGenesis()
        {
            uint1024_t genesisHash = TAO::Ledger::ChainState::Genesis();

            if(!LLD::legDB->ReadBlock(genesisHash, ChainState::stateGenesis))
            {
                /* Build the first transaction for genesis. */
                const char* pszTimestamp = "Silver Doctors [2-19-2014] BANKER CLEAN-UP: WE ARE AT THE PRECIPICE OF SOMETHING BIG";
                Legacy::Transaction genesis;
                genesis.nTime = 1409456199;
                genesis.vin.resize(1);
                genesis.vout.resize(1);
                genesis.vin[0].scriptSig = Legacy::Script() << std::vector<uint8_t>((const uint8_t*)pszTimestamp,
                    (const uint8_t*)pszTimestamp + strlen(pszTimestamp));
                genesis.vout[0].SetEmpty();

                /* Build the hashes to calculate the merkle root. */
                std::vector<uint512_t> vHashes;
                vHashes.push_back(genesis.GetHash());

                /* Create the genesis block. */
                Legacy::LegacyBlock block;
                block.vtx.push_back(genesis);
                block.hashPrevBlock = 0;
                block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
                block.nVersion = 1;
                block.nHeight  = 0;
                block.nChannel = 2;
                block.nTime    = 1409456199;
                block.nBits    = bnProofOfWorkLimit[2].GetCompact();
                block.nNonce   = config::fTestNet ? 122999499 : 2196828850;

                /* Ensure the hard coded merkle root is the same calculated merkle root. */
                assert(block.hashMerkleRoot == uint512_t("0x8a971e1cec5455809241a3f345618a32dc8cb3583e03de27e6fe1bb4dfa210c413b7e6e15f233e938674a309df5a49db362feedbf96f93fb1c6bfeaa93bd1986"));

                /* Ensure the time of transaction is the same time as the block time. */
                assert(genesis.nTime == block.nTime);

                /* Check that the genesis hash is correct. */

                LLC::CBigNum target;
                target.SetCompact(block.nBits);
                if(block.GetHash() != genesisHash)
                    return debug::error(FUNCTION, "genesis hash does not match");

                /* Check that the block passes basic validation. */
                if(!block.Check())
                    return debug::error(FUNCTION, "genesis block check failed");

                /* Set the proper chain state variables. */
                ChainState::stateGenesis = BlockState(block);
                ChainState::stateGenesis.nChannelHeight = 1;
                ChainState::stateGenesis.hashCheckpoint = genesisHash;
                ChainState::stateGenesis.print();

                /* Set the best block. */
                ChainState::stateBest = ChainState::stateGenesis;

                /* Write the block to disk. */
                if(!LLD::legDB->WriteBlock(genesisHash, ChainState::stateGenesis))
                    return debug::error(FUNCTION, "genesis didn't commit to disk");

                /* Write the best chain to the database. */
                ChainState::hashBestChain = genesisHash;
                if(!LLD::legDB->WriteBestChain(genesisHash))
                    return debug::error(FUNCTION, "couldn't write best chain.");
            }

            return true;
        }


        /* Handles the creation of a private block chain. */
        void ThreadGenerator()
        {
            if(!config::GetBoolArg("-private"))
                return;

            /* Startup Debug. */
            debug::log(0, FUNCTION, "Generator Thread Started...");

            /* Get the account. */
            TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("user", "pass");

            std::mutex MUTEX;
            while(!config::fShutdown.load())
            {
                std::unique_lock<std::mutex> CONDITION_LOCK(MUTEX);
                PRIVATE_CONDITION.wait(CONDITION_LOCK, []{ return config::fShutdown.load() || mempool.Size() > 0; });

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return;

                /* Create the block object. */
                runtime::timer TIMER;
                TIMER.Start();
                TAO::Ledger::TritiumBlock block;
                if(!TAO::Ledger::CreateBlock(user, std::string("1234").c_str(), 2, block))
                    continue;

                /* Get the secret from new key. */
                std::vector<uint8_t> vBytes = user->Generate(block.producer.nSequence, "1234").GetBytes();
                LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

                /* Generate the EC Key. */
                LLC::ECKey key(NID_brainpoolP512t1, 64);
                if(!key.SetSecret(vchSecret, true))
                    continue;

                /* Generate new block signature. */
                block.GenerateSignature(key);

                /* Add the producer into the memory pool. */
                mempool.AddUnchecked(block.producer);

                /* Verify the block object. */
                if(!block.Check())
                    continue;

                /* Create the state object. */
                if(!block.Accept())
                    continue;

                /* Debug output. */
                debug::log(0, FUNCTION, "Private Block Cleared in ", TIMER.ElapsedMilliseconds(), " ms");
            }
        }
    }
}
