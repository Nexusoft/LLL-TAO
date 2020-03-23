/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/create.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/legacy.h>

#include <LLC/types/bignum.h>
#include <TAO/Register/types/address.h>
#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/include/developer.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/API/include/global.h> //for CREATE_MUTEX

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

        /* Create a new block object from the chain.*/
        static memory::atomic<TAO::Ledger::TritiumBlock> blockCache[4];


        /* Create a new transaction object from signature chain. */
        bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx)
        {
            /* Get the genesis id of the sigchain. */
            uint256_t hashGenesis = user->Genesis();

            /* Last sigchain transaction. */
            uint512_t hashLast = 0;

            /* Set default signature types. */
            tx.nKeyType  = SIGNATURE::BRAINPOOL;
            tx.nNextType = SIGNATURE::BRAINPOOL;

            /* Check for configuration options. */
            if(config::GetBoolArg("-falcon"))
            {
                tx.nNextType = SIGNATURE::FALCON;
                tx.nKeyType  = SIGNATURE::FALCON;
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
                tx.hashRecovery = txPrev.hashRecovery;
                tx.nTimestamp  = std::max(runtime::unifiedtimestamp(), txPrev.nTimestamp);
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
                tx.hashRecovery = txPrev.hashRecovery;
                tx.nTimestamp  = std::max(runtime::unifiedtimestamp(), txPrev.nTimestamp);
            }

            /* Set the transaction version based on the timestamp. The transaction version is current version
               unless an activation is pending */
            uint32_t nCurrent = CurrentTransactionVersion();
            if(TransactionVersionActive(tx.nTimestamp, nCurrent))
                tx.nVersion = nCurrent;
            else
                tx.nVersion = nCurrent - 1;

            /* Genesis Transaction. */
            tx.NextHash(user->Generate(tx.nSequence + 1, pin), tx.nNextType);
            tx.hashGenesis = user->Genesis();

            return true;
        }


        /* Gets a list of transactions from memory pool for current block. */
        void AddTransactions(TAO::Ledger::TritiumBlock& block)
        {
            /* Clear the transactions. */
            block.vtx.clear();

            /* Check the memory pool. */
            std::vector<uint512_t> vMempool;
            mempool.List(vMempool);

            /* Start a ACID transaction (to be disposed). */
            LLD::TxnBegin(FLAGS::MINER);

            debug::log(3, "BEGIN-------------------------------------");

            /* Loop through the list of transactions. */
            std::set<uint512_t> setDependents;
            for(const auto& hash : vMempool)
            {
                /* Check the Size limits of the Current Block. */
                if(::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 256 >= MAX_BLOCK_SIZE)
                    break;

                /* Get the transaction from the memory pool. */
                TAO::Ledger::Transaction tx;
                if(!mempool.Get(hash, tx))
                    continue;

                /* Don't add transactions that are coinbase or coinstake. */
                if(tx.IsCoinBase() || tx.IsCoinStake())
                {
                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - tx is coinbase/coinstake");
                    continue;
                }

                /* Check for failed dependants. */
                if(setDependents.count(tx.hashPrevTx))
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - INVALID dependent");
                    continue;
                }

                /* Check for timestamp violations. */
                if(tx.nTimestamp > runtime::unifiedtimestamp() + runtime::maxdrift())
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - timesamp too far in future");
                    continue;
                }

                /* Check the pre-states and post-states. */
                if(!tx.Verify(FLAGS::MINER))
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - failed to verify");
                    continue;
                }

                /* Check to see if this transaction connects. */
                if(!tx.Connect(FLAGS::MINER))
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - failed to connect");
                    continue;
                }

                /* Check that the hashlast is on disk. If it is not, then the sig chain genesis must also be in this block.  If for
                   any reason the genesis transaction should be in this block but failed one of the above rules, then we could end
                   up with a subsequent transaction also in this block for which the genesis is not going to exist.  In which case
                   we need to omit this transaction also. The simplest solution for this is to skip any transactions that are not
                   the first in the sequence if the hash last is not currently on disk. If a sig chain transcation and subsequent
                   transaction genuinely should be in the same block, then ths will just result in the subsequent transaction being
                   left out of this block and included in the next.*/
                uint512_t hashLast = 0;
                if(!tx.IsFirst() && !LLD::Ledger->ReadLast(tx.hashGenesis, hashLast) )
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - genesis not on disk");
                    continue;
                }

                /* Dump sequence on verbose 3 levels. */
                if(config::nVerbose >= 3)
                    tx.print();

                /* Add the transaction to the block. */
                block.vtx.push_back(std::make_pair(TRANSACTION::TRITIUM, hash));
            }

            debug::log(3, "END-------------------------------------");

            /* Abort the temporary ACID transaction. */
            LLD::TxnAbort(FLAGS::MINER);

            /* Clear for legacy. */
            vMempool.clear();

            /* Retrieve list of transaction hashes from mempool. Limit list to a sane size that would typically more than fill a
             * legacy block, rather than pulling entire pool if it is very large. */
            TAO::Ledger::mempool.List(vMempool, 100, true);

            /* Loop through the list of transactions. */
            TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::stateBest.load();
            for(const auto& hash : vMempool)
            {
                /* Check the Size limits of the Current Block. */
                if(::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 256 >= MAX_BLOCK_SIZE)
                    break;

                /* Get the transaction from the memory pool. */
                Legacy::Transaction tx;
                if(!mempool.Get(hash, tx))
                {
                    debug::error(FUNCTION, "Unable to read transaction from mempool ", hash.SubString(10));
                    continue;
                }

                /* Don't add transactions that are coinbase or coinstake. */
                if(tx.IsCoinBase() || tx.IsCoinStake())
                {
                    debug::log(2, FUNCTION, "Mempool transaction is Coinbase/Coinstake ", hash.SubString(10));
                    continue;
                }

                /* Check transaction for finality. */
                if(!tx.IsFinal())
                {
                    debug::log(2, FUNCTION, "Mempool transaction is not Final ", hash.SubString(10));
                    continue;
                }

                /* Check for timestamp violations. */
                if(tx.nTime > runtime::unifiedtimestamp() + runtime::maxdrift())
                    continue;

                /* Retrieve tx inputs */
                std::map<uint512_t, std::pair<uint8_t, DataStream> > mapInputs;
                if(!tx.FetchInputs(mapInputs))
                {
                    debug::log(2, FUNCTION, "Failed to get transaction inputs ", hash.SubString(10));
                    continue;
                }

                /* Check tx fee meetes minimum fee requirement */
                int64_t nTxFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();
                if(nTxFees <  tx.GetMinFee(1000, false))
                {
                    debug::log(2, FUNCTION, "Not enough fees ", hash.SubString(10));
                    continue;
                }

                /* Check that transction can be connected. */
                if(!tx.Connect(mapInputs, stateBest, FLAGS::MINER))
                {
                    debug::log(2, FUNCTION, "Failed to connect inputs ", hash.SubString(10));
                    continue;
                }

                /* Dump sequence on verbose 3 levels. */
                if(config::nVerbose >= 3)
                    tx.print();

                /* Add the transaction to the block. */
                block.vtx.push_back(std::make_pair(TRANSACTION::LEGACY, hash));
            }
        }


        /* Populate block header data for a new block. */
        void AddBlockData(const TAO::Ledger::BlockState& stateBest, const uint32_t nChannel, TAO::Ledger::TritiumBlock& block)
        {
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


        /* Create a new block object from the chain. */
        bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
            const uint32_t nChannel, TAO::Ledger::TritiumBlock& block, const uint64_t nExtraNonce,
            Legacy::Coinbase *pCoinbaseRecipients)
        {

            /* Lock this user's sigchain. */
            LOCK(TAO::API::users->CREATE_MUTEX);

            /* Only allow prime, hash, and private channels. */
            if (nChannel < 1 || nChannel > 3)
                return debug::error(FUNCTION, "Invalid channel: ", nChannel);

            /* Set the block to null. */
            block.SetNull();

            /* Modulate the Block Versions if they correspond to their proper time stamp */
            /* Normally, if condition is true and block version is current version unless an activation is pending */
            uint32_t nCurrent = CurrentBlockVersion();
            if(BlockVersionActive(runtime::unifiedtimestamp(), nCurrent)) // --> New Block Version Activation Switch
                block.nVersion = nCurrent;
            else
                block.nVersion = nCurrent - 1;

            /* Handle if the block is cached. */
            if(ChainState::stateBest.load().GetHash() == blockCache[nChannel].load().hashPrevBlock)
            {
                /* Set the block to cached block. */
                block = blockCache[nChannel].load();

                /* Add new transactions. */
                AddTransactions(block);

                /* Check that the producer isn't going to orphan any transactions. */
                TAO::Ledger::Transaction tx;
                if(mempool.Get(block.producer.hashGenesis, tx) && block.producer.hashPrevTx != tx.GetHash())
                {
                    /* Handle for STALE producer. */
                    debug::log(0, FUNCTION, "Producer is stale, rebuilding...");

                    /* Setup the producer transaction. */
                    if(!CreateTransaction(user, pin, block.producer))
                        return debug::error(FUNCTION, "Failed to create producer transactions.");

                    /* Update the producer timestamp */
                    UpdateProducerTimestamp(block);

                    /* Store new block cache. */
                    blockCache[nChannel].store(block);
                }

                /* Use the extra nonce if block is coinbase. */
                if(nChannel == 1 || nChannel == 2)
                {
                    /* Output type 0 is mining/minting reward */
                    uint64_t nBlockReward = GetCoinbaseReward(ChainState::stateBest.load(), nChannel, 0);

                    /* Create coinbase transaction. */
                    block.producer[0].Clear();
                    block.producer[0] << uint8_t(TAO::Operation::OP::COINBASE);

                    /* Add the spendable genesis. */
                    block.producer[0] << user->Genesis();

                    /* The total to be credited. */
                    uint64_t  nCredit = nBlockReward;

                    /* If there are coinbase recipients, set the reward to the coinbase wallet reward. */
                    if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
                        nCredit = pCoinbaseRecipients->WalletReward();

                    /* Check to make sure credit is non-zero. */
                    if(nCredit == 0)
                        return debug::error(FUNCTION, "Empty block producer reward.");

                    block.producer[0] << nCredit;

                    /* The extra nonce to coinbase. */
                    block.producer[0] << nExtraNonce;

                    /* Add coinbase recipient amounts to block producer transaction if any. */
                    if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
                    {
                        /* Ensure wallet reward and recipient amounts add up to correct block reward. */
                        if(!pCoinbaseRecipients->IsValid())
                            return debug::error(FUNCTION, "Coinbase recipients contain invalid amounts.");

                        /* Get the map of outputs for this coinbase. */
                        std::map<std::string, uint64_t> mapOutputs = pCoinbaseRecipients->Outputs();
                        uint32_t nTx = 1;

                        for(const auto&entry : mapOutputs)
                        {
                            /* Build the recipient address from a hex string. */
                            uint256_t hashGenesis = uint256_t(entry.first);

                            /* Ensure the address is valid. */
                            if(!LLD::Ledger->HasGenesis(hashGenesis))
                                return debug::error(FUNCTION, "Invaild recipient address: ", entry.first, " (", nTx, ")");

                            /* Set coinbase operation. */
                            block.producer[nTx].Clear();
                            block.producer[nTx] << uint8_t(TAO::Operation::OP::COINBASE);

                            /* Set sigchain recipient. */
                            block.producer[nTx] << hashGenesis;

                            /* Set coinbase amount for associated recipent. */
                            block.producer[nTx] << entry.second;

                            /* The extra nonce to coinbase. */
                            block.producer[nTx] << nExtraNonce;

                            ++nTx;
                        }
                    }
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
                    return debug::error(FUNCTION, "Failed to create producer transactions.");

                /* Create the Coinbase Transaction if the Channel specifies. */
                if(nChannel == 1 || nChannel == 2)
                {
                    /* Output type 0 is mining/minting reward */
                    uint64_t nBlockReward = GetCoinbaseReward(stateBest, nChannel, 0);

                    /* Create coinbase transaction. */
                    block.producer[0] << uint8_t(TAO::Operation::OP::COINBASE);

                    /* Add the spendable genesis. */
                    block.producer[0] << user->Genesis();

                    /* The total to be credited. */
                    uint64_t  nCredit = nBlockReward;

                    /* If there are coinbase recipients, set the reward to the coinbase wallet reward. */
                    if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
                        nCredit = pCoinbaseRecipients->WalletReward();

                    /* Check to make sure credit is non-zero. */
                    if(nCredit == 0)
                        return debug::error(FUNCTION, "Empty block producer reward.");


                    block.producer[0] << nCredit;

                    /* The extra nonce to coinbase. */
                    block.producer[0] << nExtraNonce;

                    /* Add coinbase recipient amounts to block producer transaction if any. */
                    if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
                    {
                        /* Ensure wallet reward and recipient amounts add up to correct block reward. */
                        if(!pCoinbaseRecipients->IsValid())
                            return debug::error(FUNCTION, "Coinbase recipients contain invalid amounts.");

                        /* Get the map of outputs for this coinbase. */
                        std::map<std::string, uint64_t> mapOutputs = pCoinbaseRecipients->Outputs();
                        uint32_t nTx = 1;

                        for(const auto& entry : mapOutputs)
                        {
                            /* Build the recipient address from a hex string. */
                            uint256_t hashGenesis = uint256_t(entry.first);

                            /* Ensure the address is valid. */
                            if(!LLD::Ledger->HasGenesis(hashGenesis))
                                return debug::error(FUNCTION, "Invaild recipient address: ", entry.first, " (", nTx, ")");

                            /* Set coinbase operation. */
                            block.producer[nTx] << uint8_t(TAO::Operation::OP::COINBASE);

                            /* Set sigchain recipient. */
                            block.producer[nTx] << hashGenesis;

                            /* Set coinbase amount for associated recipent. */
                            block.producer[nTx] << entry.second;

                            /* The extra nonce to coinbase. */
                            block.producer[nTx] << nExtraNonce;

                            ++nTx;
                        }
                    }

                    /* Get the last state block for channel. */
                    TAO::Ledger::BlockState statePrev = stateBest;
                    if(GetLastState(statePrev, nChannel))
                    {
                        /* Check for interval. */
                        if(statePrev.nChannelHeight %
                            (config::fTestNet.load() ? AMBASSADOR_PAYOUT_THRESHOLD_TESTNET : AMBASSADOR_PAYOUT_THRESHOLD) == 0)
                        {
                            /* Get the total in reserves. */
                            int64_t nBalance = statePrev.nReleasedReserve[1] - (33 * NXS_COIN); //leave 33 coins in the reserve
                            if(nBalance > 0)
                            {
                                /* Loop through the embassy sigchains. */
                                for(auto it = Ambassador(block.nVersion).begin(); it != Ambassador(block.nVersion).end(); ++it)
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


                        /* Check for interval. */
                        if(statePrev.nChannelHeight %
                            (config::fTestNet.load() ? DEVELOPER_PAYOUT_THRESHOLD_TESTNET : DEVELOPER_PAYOUT_THRESHOLD) == 0)
                        {
                            /* Get the total in reserves. */
                            int64_t nBalance = statePrev.nReleasedReserve[2] - (3 * NXS_COIN); //leave 3 coins in the reserve
                            if(nBalance > 0)
                            {
                                /* Loop through the embassy sigchains. */
                                for(auto it = Developer(block.nVersion).begin(); it != Developer(block.nVersion).end(); ++it)
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

                /* Update the producer timestamp */
                UpdateProducerTimestamp(block);

                /* Sign the producer transaction. */
                block.producer.Sign(user->Generate(block.producer.nSequence, pin));

                /* Populate the block metadata */
                AddBlockData(stateBest, nChannel, block);

                /* Store the cached block. */
                blockCache[nChannel].store(block);
            }

            /* Update the time for the newly created block. */
            block.UpdateTime();

            return true;
        }


        /* Create a new Proof of Stake (channel 0) block object from the chain. */
        bool CreateStakeBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                              TAO::Ledger::TritiumBlock& block, const bool fGenesis)
        {
            /* Lock this user's sigchain. */
            LOCK(TAO::API::users->CREATE_MUTEX);

            /* Proof of stake has channel-id of 0. */
            const uint32_t nChannel = 0;

            /* Set the block to null. */
            block.SetNull();

            /* Modulate the Block Versions if they correspond to their proper time stamp */
            /* Normally, if condition is true and block version is current version unless an activation is pending */
            uint32_t nCurrent = CurrentBlockVersion();
            if(BlockVersionActive(runtime::unifiedtimestamp(), nCurrent)) // --> New Block Version Activation Switch
                block.nVersion = nCurrent;
            else
                block.nVersion = nCurrent - 1;

            /* Cache the best chain before processing. */
            const TAO::Ledger::BlockState stateBest = ChainState::stateBest.load();

            /* Add the transactions to the block (no transactions for PoS Genesis block). */
            /* Must add transactions first, before creating producer, so producer is sequenced last if user has tx in block */
            if(!fGenesis)
                AddTransactions(block);

            /* Create the producer transaction. */
            if(!CreateTransaction(user, pin, block.producer))
                return debug::error(FUNCTION, "failed to create producer transactions");

            /* Update the producer timestamp */
            UpdateProducerTimestamp(block);

            /* NOTE: The remainder of Coinstake producer not configured here. Stake minter must handle it. */

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
                genesis.nVersion = 1;
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
                        if(!key.SetSecret(vchSecret))
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

                /* Debug output. */
                debug::log(0, FUNCTION, "Private Block CREATED in ", TIMER.ElapsedMilliseconds(), " ms");

                /* Verify the block object. */
                uint8_t nStatus = 0;
                TAO::Ledger::Process(block, nStatus);

                /* Check the statues. */
                if(!(nStatus & PROCESS::ACCEPTED))
                    continue;
            }
        }

        /* Updates the producer timestamp, making sure it is not earlier than the previous block. */
        void UpdateProducerTimestamp(TAO::Ledger::TritiumBlock& block)
        {
            /* Update the producer timestamp, making sure it is not earlier than the previous block.  However we can't simply
            set the timstamp to be last block time + 1, in case there is a long gap between blocks, as there is a consensus
            rule that the producer timestamp cannot be more than 3600 seconds before the current block time. */
            if(ChainState::stateBest.load().GetBlockTime() + 1 > runtime::unifiedtimestamp())
                block.producer.nTimestamp = std::max(block.producer.nTimestamp, ChainState::stateBest.load().GetBlockTime() + 1);
            else
                block.producer.nTimestamp = std::max(block.producer.nTimestamp, runtime::unifiedtimestamp());

            /* Since we have updated the producer transaction timestamp, we now also need to set the transaction version again as
               the version is based on the transaction time. Transaction version is current version unless an activation is pending */
            uint32_t nCurrent = CurrentTransactionVersion();
            if(TransactionVersionActive(block.producer.nTimestamp, nCurrent))
                block.producer.nVersion = nCurrent;
            else
                block.producer.nVersion = nCurrent - 1;
        }
    }
}
