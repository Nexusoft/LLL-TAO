/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/transaction.h>
#include <Legacy/types/legacy.h>

#include <LLC/types/bignum.h>
#include <TAO/Register/types/address.h>
#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Condition variable for private blocks. */
        std::condition_variable PRIVATE_CONDITION;


        /* Create a new transaction object from signature chain. */
        bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx)
        {
            /* Get the genesis id of the sigchain. */
            uint256_t hashGenesis = user->Genesis();

            /* Last sigchain transaction. */
            uint512_t hashLast = 0;

            /* Set default signature types. */
            tx.nNextType = SIGNATURE::FALCON;
            tx.nKeyType  = SIGNATURE::FALCON;

            /* Check for configuration options. */
            if(config::GetBoolArg("-brainpool"))
            {
                tx.nKeyType  = SIGNATURE::BRAINPOOL;
                tx.nNextType = SIGNATURE::BRAINPOOL;
            }

            /* Check mempool for other transactions. */
            TAO::Ledger::Transaction txPrev;
            if(mempool.Get(hashGenesis, txPrev))
            {
                /* Build new transaction object. */
                tx.nSequence   = txPrev.nSequence + 1;
                tx.hashGenesis = txPrev.hashGenesis;
                tx.hashPrevTx  = txPrev.GetHash();
                tx.nKeyType    = txPrev.nNextType;
            }

            /* Get the last transaction. */
            else if(LLD::Ledger->ReadLast(user->Genesis(), hashLast))
            {
                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev))
                    return debug::error(FUNCTION, "no prev tx ", hashLast.ToString(), " in ledger db");

                /* Build new transaction object. */
                tx.nSequence   = txPrev.nSequence + 1;
                tx.hashGenesis = txPrev.hashGenesis;
                tx.hashPrevTx  = hashLast;
                tx.nKeyType    = txPrev.nNextType;
            }

            /* Genesis Transaction. */
            tx.NextHash(user->Generate(tx.nSequence + 1, pin), tx.nNextType);
            tx.hashGenesis = user->Genesis();

            return true;
        }


        /* Gets a list of transactions from memory pool for current block. */
        void AddTransactions(TAO::Ledger::TritiumBlock& block)
        {
            /* Check the memory pool. */
            std::vector<uint512_t> vMempool;
            mempool.List(vMempool);

            /* Loop through the list of transactions. */
            for(const auto& hash : vMempool)
            {
                /* Check the Size limits of the Current Block. */
                if(::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 200 >= MAX_BLOCK_SIZE)
                    break;

                /* Get the transaction from the memory pool. */
                TAO::Ledger::Transaction tx;
                if(!mempool.Get(hash, tx))
                    continue;

                /* Don't add transactions that are coinbase or coinstake. */
                if(tx.IsCoinbase() || tx.IsCoinstake())
                    continue;

                /* Check for timestamp violations. */
                if(tx.nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                    continue;

                /* Add the transaction to the block. */
                block.vtx.push_back(std::make_pair(TRITIUM_TX, hash));
            }
        }


        /* Populate block header data for a new block. */
        void AddBlockData(const TAO::Ledger::BlockState& stateBest, const uint32_t nChannel, TAO::Ledger::TritiumBlock& block)
        {

            /* Modulate the Block Versions if they correspond to their proper time stamp */
            /* Normally, if condition is true and block version is NETWORK_BLOCK_CURRENT_VERSION unless an activation is pending */
            if(runtime::unifiedtimestamp() >= (config::fTestNet.load() ?
                TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] :
                NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
            {
                block.nVersion = config::fTestNet.load() ?
                    TESTNET_BLOCK_CURRENT_VERSION :
                    NETWORK_BLOCK_CURRENT_VERSION; // --> New Block Version Activation Switch
            }
            else
            {
                block.nVersion = config::fTestNet.load() ?
                    TESTNET_BLOCK_CURRENT_VERSION - 1 :
                    NETWORK_BLOCK_CURRENT_VERSION - 1;
            }

            /* Calculate the merkle root (stake minter must handle channel 0 after completing coinstake producer setup) */
            if(nChannel != 0)
            {
                /* Add the transaction hashest. */
                std::vector<uint512_t> vHashes;
                for(const auto& tx : block.vtx)
                    vHashes.push_back(tx.second);

                /* Producer transaction is last. */
                vHashes.push_back(block.producer.GetHash());

                /* Build the block's merkle root. */
                block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
            }

            /* Add remaining block data */
            block.hashPrevBlock = stateBest.GetHash();
            block.nChannel      = nChannel;
            block.nHeight       = stateBest.nHeight + 1;
            block.nBits         = GetNextTargetRequired(stateBest, nChannel, false);
            block.nNonce        = 1;
            block.nTime         = static_cast<uint32_t>(std::max(stateBest.GetBlockTime() + 1, runtime::unifiedtimestamp()));
        }


        /* Create a new block object from the chain.*/
        static memory::atomic<TAO::Ledger::TritiumBlock> blockCache[4];

        bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
            const uint32_t nChannel, TAO::Ledger::TritiumBlock& block, const uint64_t nExtraNonce)
        {

            if (nChannel < 1 || nChannel > 3)
                return debug::error(FUNCTION, "invalid channel");

            /* Set the block to null. */
            block.SetNull();

            /* Handle if the block is cached. */
            if(ChainState::stateBest.load().GetHash() == blockCache[nChannel].load().hashPrevBlock)
            {
                /* Set the block to cached block. */
                block = blockCache[nChannel].load();

                /* Clear the block transactions. */
                block.vtx.clear();

                /* Add new transactions. */
                AddTransactions(block);

                /* Check that the producer isn't going to orphan any transactions. */
                TAO::Ledger::Transaction tx;
                if(mempool.Get(block.producer.hashGenesis, tx) && block.producer.hashPrevTx != tx.GetHash())
                {
                    /* Handle for STALE producer. */
                    debug::log(0, FUNCTION, "producer is stale, rebuilding...");

                    /* Setup the producer transaction. */
                    if(!CreateTransaction(user, pin, block.producer))
                        return debug::error(FUNCTION, "failed to create producer transactions");

                    /* Store new block cache. */
                    blockCache[nChannel].store(block);
                }
                else
                {
                    /* Update the producer timestamp. */
                    block.producer.nTimestamp = runtime::unifiedtimestamp();
                }

                /* Use the extra nonce if block is coinbase. */
                if(nChannel == 1 || nChannel == 2)
                {
                    /* Create coinbase transaction. */
                    block.producer[0].Clear();
                    block.producer[0] << uint8_t(TAO::Operation::OP::COINBASE);

                    /* Add the spendable genesis. */
                    block.producer[0] << user->Genesis();

                    /* The total to be credited. */
                    uint64_t  nCredit = GetCoinbaseReward(ChainState::stateBest.load(), nChannel, 0);
                    block.producer[0] << nCredit;

                    /* The extra nonce to coinbase. */
                    block.producer[0] << nExtraNonce;
                }
                else if(nChannel == 3)
                {
                    /* Create an authorize producer. */
                    block.producer[0].Clear();
                    block.producer[0] << uint8_t(TAO::Operation::OP::AUTHORIZE);

                    /* Get the sigchain txid. */
                    block.producer[0] << block.producer.hashPrevTx;

                    /* Set the genesis operation. */
                    block.producer[0] << block.producer.hashGenesis;
                }

                /* Sign the producer transaction. */
                block.producer.Sign(user->Generate(block.producer.nSequence, pin));

                /* Rebuild the merkle tree for updated block. */
                std::vector<uint512_t> vHashes;
                for(const auto& tx : block.vtx)
                    vHashes.push_back(tx.second);

                /* Producer transaction is last. */
                vHashes.push_back(block.producer.GetHash());

                /* Build the block's merkle root. */
                block.hashMerkleRoot  = block.BuildMerkleTree(vHashes);
            }
            else //block not cached, set up new block
            {
                /* Cache the best chain before processing. */
                const TAO::Ledger::BlockState stateBest = ChainState::stateBest.load();

                /* Must add transactions first, before creating producer, so producer is sequenced last if user has tx in block */
                AddTransactions(block);

                /* Setup the producer transaction. */
                if(!CreateTransaction(user, pin, block.producer))
                    return debug::error(FUNCTION, "failed to create producer transactions");

                /* Create the Coinbase Transaction if the Channel specifies. */
                if(nChannel == 1 || nChannel == 2)
                {
                    /* Create coinbase transaction. */
                    block.producer[0] << uint8_t(TAO::Operation::OP::COINBASE);

                    /* Add the spendable genesis. */
                    block.producer[0] << user->Genesis();

                    /* The total to be credited. */
                    uint64_t nCredit = GetCoinbaseReward(stateBest, nChannel, 0);
                    block.producer[0] << nCredit;

                    /* The extra nonce to coinbase. */
                    block.producer[0] << nExtraNonce;

                    /* Get the last state block for channel. */
                    TAO::Ledger::BlockState statePrev = stateBest;
                    if(GetLastState(statePrev, nChannel))
                    {
                        /* Check for interval. */
                        if(statePrev.nChannelHeight % ABMASSADOR_PAYOUT_THRESHOLD == 0)
                        {
                            /* Get the total in reserves. */
                            int64_t nBalance = statePrev.nReleasedReserve[1] - (33 * NXS_COIN); //leave 33 coins in the reserve
                            if(nBalance > 0)
                            {
                                /* Loop through the embassy sigchains. */
                                for(auto it = AMBASSADOR.begin(); it != AMBASSADOR.end(); ++it)
                                {
                                    /* Make sure to push to end. */
                                    uint32_t nContract = block.producer.Size();

                                    /* Create coinbase transaction. */
                                    block.producer[nContract] << uint8_t(TAO::Operation::OP::COINBASE);
                                    block.producer[nContract] << it->first;

                                    /* The total to be credited. */
                                    uint64_t nCredit = (nBalance * it->second.second) / 1000;
                                    block.producer[nContract] << nCredit;
                                    block.producer[nContract] << uint64_t(0);
                                }
                            }
                        }
                    }
                }
                else if(nChannel == 3)
                {
                    /* Create an authorize producer. */
                    block.producer[0] << uint8_t(TAO::Operation::OP::AUTHORIZE);

                    /* Get the sigchain txid. */
                    block.producer[0] << block.producer.hashPrevTx;

                    /* Set the genesis operation. */
                    block.producer[0] << block.producer.hashGenesis;
                }

                /* Sign the producer transaction. */
                block.producer.Sign(user->Generate(block.producer.nSequence, pin));

                /* Populate the block metadata */
                AddBlockData(stateBest, nChannel, block);

                /* Store the cached block. */
                blockCache[nChannel].store(block);
            }

            return true;
        }


        bool CreateStakeBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                              TAO::Ledger::TritiumBlock& block, const uint64_t isGenesis)
        {

            const uint32_t nChannel = 0;

            /* Set the block to null. */
            block.SetNull();

            /* Cache the best chain before processing. */
            const TAO::Ledger::BlockState stateBest = ChainState::stateBest.load();

            /* Add the transactions to the block (no transactions for PoS Genesis block). */
            /* Must add transactions first, before creating producer, so producer is sequenced last if user has tx in block */
            if (!isGenesis)
                AddTransactions(block);

            /* Create the producer transaction. */
            if(!CreateTransaction(user, pin, block.producer))
                return debug::error(FUNCTION, "failed to create producer transactions");

            /* Set the Coinstake timestamp. */
            block.producer.nTimestamp = stateBest.GetBlockTime() + 1;

            /* The remainder of Coinstake producer not configured here. Stake minter must handle it. */

            /* Populate the block metadata */
            AddBlockData(stateBest, nChannel, block);

            return true;
        }


        /*  Creates the genesis block. */
        bool CreateGenesis()
        {
            /* Get the genesis hash. */
            uint1024_t hashGenesis = TAO::Ledger::ChainState::Genesis();

            /* Check for genesis from disk. */
            if(!LLD::Ledger->ReadBlock(hashGenesis, ChainState::stateGenesis))
            {
                /* Build the first transaction for genesis. */
                const char* pszTimestamp = "Silver Doctors [2-19-2014] BANKER CLEAN-UP: WE ARE AT THE PRECIPICE OF SOMETHING BIG";

                /* Main coinbase genesis. */
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
                block.nBits    = LLC::CBigNum(bnProofOfWorkLimit[2]).GetCompact();
                block.nNonce   = config::fTestNet.load() ? 122999499 : 2196828850;

                /* Ensure the hard coded merkle root is the same calculated merkle root. */
                assert(block.hashMerkleRoot == uint512_t("0x8a971e1cec5455809241a3f345618a32dc8cb3583e03de27e6fe1bb4dfa210c413b7e6e15f233e938674a309df5a49db362feedbf96f93fb1c6bfeaa93bd1986"));

                /* Ensure the time of transaction is the same time as the block time. */
                assert(genesis.nTime == block.nTime);

                /* Check that the genesis hash is correct. */
                LLC::CBigNum target;
                target.SetCompact(block.nBits);
                if(block.GetHash() != hashGenesis)
                    return debug::error(FUNCTION, "genesis hash does not match");

                /* Set the proper chain state variables. */
                ChainState::stateGenesis = BlockState(block);
                ChainState::stateGenesis.nChannelHeight = 1;
                ChainState::stateGenesis.hashCheckpoint = hashGenesis;

                /* Set the best block. */
                ChainState::stateBest = ChainState::stateGenesis;

                /* Write the block to disk. */
                if(!LLD::Ledger->WriteBlock(hashGenesis, ChainState::stateGenesis))
                    return debug::error(FUNCTION, "genesis didn't commit to disk");

                /* Write the best chain to the database. */
                ChainState::hashBestChain = hashGenesis;
                if(!LLD::Ledger->WriteBestChain(hashGenesis))
                    return debug::error(FUNCTION, "couldn't write best chain.");
            }

            return true;
        }


        /* Handles the creation of a private block chain. */
        void ThreadGenerator()
        {
            if(!config::GetBoolArg("-private") || !config::mapArgs.count("-generate"))
                return;

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user =
                new TAO::Ledger::SignatureChain("generate", config::GetArg("-generate", "").c_str());

            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                {
                    debug::error(FUNCTION, "No previous transaction found... closing");

                    return;
                }

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev))
                {
                    debug::error(FUNCTION, "No previous transaction found... closing");

                    return;
                }

                /* Genesis Transaction. */
                TAO::Ledger::Transaction tx;
                tx.NextHash(user->Generate(txPrev.nSequence + 1, "1234", false), txPrev.nNextType);

                /* Check for consistency. */
                if(txPrev.hashNext != tx.hashNext)
                {
                    debug::error(FUNCTION, "Invalid credentials... closing");

                    return;
                }
            }

            /* Startup Debug. */
            debug::log(0, FUNCTION, "Generator Thread Started...");

            std::mutex MUTEX;
            while(!config::fShutdown.load())
            {
                std::unique_lock<std::mutex> CONDITION_LOCK(MUTEX);
                PRIVATE_CONDITION.wait(CONDITION_LOCK, []{ return config::fShutdown.load() || mempool.Size() > 0; });

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return;

                /* Keep block production to five seconds. */
                runtime::sleep(5000);

                /* Create the block object. */
                runtime::timer TIMER;
                TIMER.Start();

                TAO::Ledger::TritiumBlock block;
                if(!TAO::Ledger::CreateBlock(user, "1234", 3, block))
                    continue;

                /* Get the secret from new key. */
                std::vector<uint8_t> vBytes = user->Generate(block.producer.nSequence, "1234").GetBytes();
                LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

                /* Switch based on signature type. */
                switch(block.producer.nKeyType)
                {
                    /* Support for the FALCON signature scheeme. */
                    case SIGNATURE::FALCON:
                    {
                        /* Create the FL Key object. */
                        LLC::FLKey key;

                        /* Set the secret parameter. */
                        if(!key.SetSecret(vchSecret, true))
                            continue;

                        /* Generate the signature. */
                        if(!block.GenerateSignature(key))
                            continue;

                        break;
                    }

                    /* Support for the BRAINPOOL signature scheme. */
                    case SIGNATURE::BRAINPOOL:
                    {
                        /* Create EC Key object. */
                        LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                        /* Set the secret parameter. */
                        if(!key.SetSecret(vchSecret, true))
                            continue;

                        /* Generate the signature. */
                        if(!block.GenerateSignature(key))
                            continue;

                        break;
                    }
                }

                /* Verify the block object. */
                if(!LLP::TritiumNode::Process(block, nullptr))
                    continue;

                /* Debug output. */
                debug::log(0, FUNCTION, "Private Block Cleared in ", TIMER.ElapsedMilliseconds(), " ms");
            }
        }

    }
}
