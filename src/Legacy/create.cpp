/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <map>
#include <utility>

#include <Legacy/include/ambassador.h>
#include <Legacy/include/create.h>
#include <Legacy/include/enum.h>
#include <Legacy/include/evaluate.h>
#include <Legacy/types/address.h>
#include <Legacy/types/txin.h>

#include <LLC/hash/SK.h>
#include <LLC/include/key.h>
#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>
#include <LLP/types/legacy.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>

/* Global Legacy namespace. */
namespace Legacy
{

    /** Constructs a new legacy block **/
    bool CreateLegacyBlock(Legacy::CReserveKey& coinbaseKey, const uint32_t nChannel, const uint32_t nID, LegacyBlock& newBlock)
    {
        newBlock.SetNull();

        /* Previous block state is current best state on chain */
        TAO::Ledger::BlockState& prevBlockState = TAO::Ledger::ChainState::stateBest;

        /* Modulate the Block Versions if they correspond to their proper time stamp */
        if (runtime::unifiedtimestamp() >= (config::fTestNet
                                            ? TAO::Ledger::TESTNET_VERSION_TIMELOCK[TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 2] 
                                            : TAO::Ledger::NETWORK_VERSION_TIMELOCK[TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 2]))
        {
            newBlock.nVersion = config::fTestNet 
                                ? TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION 
                                : TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION; // --> New Block Version Activation Switch
        }
        else
        {
            newBlock.nVersion = config::fTestNet 
                                ? TAO::Ledger::TESTNET_BLOCK_CURRENT_VERSION - 1 
                                : TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION - 1;
        }

        /* Coinbase / Coinstake Transaction. **/
        Transaction txNew;

        if (nChannel == 0)
        {
            /* Create the Coinstake transaction for Proof of Stake Channel. */
            if (!CreateCoinstakeTransaction(txNew))
                return debug::error(FUNCTION, "Error creating coinstake transaction");

            /* Add coinstake to block. */
            newBlock.vtx.push_back(txNew);

            /* Proof of Stake does not add mempool transactions at this time */
        }

        else
        {
            /* Create the Coinbase transaction for Prime/Hash Channels. */
            if (!CreateCoinbaseTransaction(coinbaseKey, nChannel, nID, newBlock.nVersion, txNew))
                return debug::error(FUNCTION, "Error creating coinbase transaction");

            /* Add coinbase to block. */
            newBlock.vtx.push_back(txNew);

            /* Add transactions from mempool */
            AddTransactions(newBlock.vtx);
        }


        /* Populate the Block Data. */
        std::vector<uint512_t> vMerkleTree;
        newBlock.hashPrevBlock  = prevBlockState.GetHash();
        newBlock.hashMerkleRoot = newBlock.BuildMerkleTree(vMerkleTree);
        newBlock.nChannel       = nChannel;
        newBlock.nHeight        = prevBlockState.nHeight + 1;
        newBlock.nBits          = GetNextTargetRequired(prevBlockState, nChannel, false);
        newBlock.nNonce         = 1;

        newBlock.UpdateTime();

        return true;
    }


    /* Create the coinstake transaction for a legacy block. */
    bool CreateCoinstakeTransaction(Transaction& coinstakeTx)
    {
        /* Initialize vin */
        coinstakeTx.vin.resize(1);
        coinstakeTx.vin[0].prevout.SetNull();

        /* Use scriptsig to mark the Coinstake transaction with Fibonacci byte signature on first input */
        coinstakeTx.vin[0].scriptSig.resize(8);
        coinstakeTx.vin[0].scriptSig[0] = 1;
        coinstakeTx.vin[0].scriptSig[1] = 2;
        coinstakeTx.vin[0].scriptSig[2] = 3;
        coinstakeTx.vin[0].scriptSig[3] = 5;
        coinstakeTx.vin[0].scriptSig[4] = 8;
        coinstakeTx.vin[0].scriptSig[5] = 13;
        coinstakeTx.vin[0].scriptSig[6] = 21;
        coinstakeTx.vin[0].scriptSig[7] = 34;

        /* Remaining Coinstake inputs will be generated by stake minter */

        /* Coinstake output also generated by stake minter */

        /* Set the Coinstake timestamp. */
        coinstakeTx.nTime = TAO::Ledger::ChainState::stateBest.GetBlockTime() + 1;

        return true;
    }


    /* Create the Coinbase transaction for a legacy block. */
    bool CreateCoinbaseTransaction(Legacy::CReserveKey& coinbaseKey, const uint32_t nChannel, 
                                   const uint32_t nID, const uint32_t nNewBlockVersion, Transaction& coinbaseTx)
    {
        /* Previous block state is current best state on chain */
        TAO::Ledger::BlockState& prevBlockState = TAO::Ledger::ChainState::stateBest;

        /* Initialize vin */
        coinbaseTx.vin.resize(1);
        coinbaseTx.vin[0].prevout.SetNull();

        /* Set the Proof of Work Script Signature. */
        coinbaseTx.vin[0].scriptSig = (Legacy::CScript() << nID * 513513512151);

        /* Set the first output to pay to the coinbaseKey. */
        coinbaseTx.vout.resize(1);
        coinbaseTx.vout[0].scriptPubKey << coinbaseKey.GetReservedKey() << Legacy::OP_CHECKSIG;

//TODO -- Add pool support for coinbase
        /* Customized Coinbase Transaction - not currently supported, this is old code */
        // if (pCoinbase)
        // {

        //     /* Dummy Transaction to Allow the Block to be Signed by Pool Wallet. [For Now] */
        //     coinbaseTx.vout[0].nValue = pCoinbase->nPoolFee;

        //     unsigned int nTx = 1;
        //     coinbaseTx.vout.resize(pCoinbase->vOutputs.size() + 1);
        //     for(std::map<std::string, uint64>::iterator nIterator = pCoinbase->vOutputs.begin(); nIterator != pCoinbase->vOutputs.end(); nIterator++)
        //     {
        //         /* Set the Appropriate Outputs. */
        //         coinbaseTx.vout[nTx].scriptPubKey.SetNexusAddress(nIterator->first);
        //         coinbaseTx.vout[nTx].nValue = nIterator->second;

        //         nTx++;
        //     }

        //     int64 nMiningReward = 0;
        //     for(int nIndex = 0; nIndex < coinbaseTx.vout.size(); nIndex++)
        //         nMiningReward += coinbaseTx.vout[nIndex].nValue;

        //     /* Double Check the Coinbase Transaction Fits in the Maximum Value. */
        //     if(nMiningReward != TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 0))
        //         return false;

        // }
        // else
        {
            /* When no customized coinbase, pay full miner reward to coinbaseKey */
            coinbaseTx.vout[0].nValue = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 0); // Output type 0 is minting of miner reward
        }

        /* Make coinbase counter mod 13 of height. */
        int nCoinbaseCounter = TAO::Ledger::ChainState::stateBest.nHeight % 13;

        /* Create the ambassador and developer outputs for Coinbase transaction */
        coinbaseTx.vout.resize(coinbaseTx.vout.size() + 2);
        NexusAddress ambassadorKeyAddress(config::fTestNet 
                                            ? (nNewBlockVersion < 5 ? TESTNET_DUMMY_ADDRESS 
                                                                    : TESTNET_DUMMY_AMBASSADOR_RECYCLED) 
                                            : (nNewBlockVersion < 5 ? AMBASSADOR_ADDRESSES[nCoinbaseCounter] 
                                                                    : AMBASSADOR_ADDRESSES_RECYCLED[nCoinbaseCounter]));

        NexusAddress devKeyAddress(config::fTestNet 
                                    ? (nNewBlockVersion < 5 ? TESTNET_DUMMY_ADDRESS 
                                                            : TESTNET_DUMMY_DEVELOPER_RECYCLED) 
                                    : (nNewBlockVersion < 5 ? DEVELOPER_ADDRESSES[nCoinbaseCounter] 
                                                            : DEVELOPER_ADDRESSES_RECYCLED[nCoinbaseCounter]));

        coinbaseTx.vout[coinbaseTx.vout.size() - 2].scriptPubKey.SetNexusAddress(ambassadorKeyAddress);
        coinbaseTx.vout[coinbaseTx.vout.size() - 1].scriptPubKey.SetNexusAddress(devKeyAddress);

        coinbaseTx.vout[coinbaseTx.vout.size() - 2].nValue = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 1); // Output type 1 is ambassador mint
        coinbaseTx.vout[coinbaseTx.vout.size() - 1].nValue = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 2); // Output type 2 is dev mint

        return true;
    }


    void AddTransactions(std::vector<Transaction>& vtx)
    {
        /* Previous block state is current best state on chain */
        TAO::Ledger::BlockState& prevBlockState = TAO::Ledger::ChainState::stateBest;

        std::vector<uint512_t> vMemPoolHashes;           // legacy tx hashes currently in mempool
        std::vector<uint512_t> vRemoveFromPool;          // invalid tx to remove from mempool
        std::multimap<double, Transaction*> mapPriority; // processing priority for mempool tx
        uint64_t nFees = 0;

        /* Retrieve list of transaction hashes from mempool.
         * Limit list to a sane size that would typically more than fill a legacy block, rather than pulling entire pool if it is very large
         */
        if (TAO::Ledger::mempool.ListLegacy(vMemPoolHashes, 1000))
        {
            /* Mempool was empty */
            return;
        }

        /* Process the mempool transactions and load them into mapPriority for processing */
        for (const uint512_t& txHash : vMemPoolHashes)
        {
            Transaction tx;

            /* Retrieve next transaction from the mempool */
            if (!TAO::Ledger::mempool.Get(txHash, tx))
            {
                /* Should never have free floating Coinbase or Coinstake transaction in the mempool */
                debug::error(FUNCTION, "Unable to read transaction from mempool %s", txHash.ToString().substr(0, 10).c_str());
                continue;
            }

            if (tx.IsCoinBase() || tx.IsCoinStake())
            {
                /* Should never have free floating Coinbase or Coinstake transaction in the mempool */
                debug::log(2, FUNCTION, "Mempool transaction is Coinbase/Coinstake %s", txHash.ToString().substr(0, 10).c_str());
                continue;
            }

            if (!tx.IsFinal())
            {
                debug::log(2, FUNCTION, "Mempool transaction is not Final %s", txHash.ToString().substr(0, 10).c_str());
                continue;
            }

            /* Calculate priority using transaction inputs */
            double dPriority = 0;
            for (const CTxIn& txin : tx.vin)
            {
                /* Check if we have previous transaction */
                Transaction txPrev;
                if (!LLD::legacyDB->ReadTx(txin.prevout.hash, txPrev))
                {
                    /* Skip any orphan transactions in the mempool until the prevout tx is confirmed */
                    debug::log(2, FUNCTION, "Found orphan transaction in mempool %s", txHash.ToString().substr(0, 10).c_str());
                    continue;
                }

                int64_t nValueIn = txPrev.vout[txin.prevout.n].nValue;

                /* Read block header */
                // This is old code replaced by use of transaction age in minutes as a proxy for nConf
                // int nConf = txindex.GetDepthInMainChain();

                /* Calculate tx age in minutes */
                uint64_t txAge = (runtime::timestamp() - txPrev.nTime) / 60;

                /* Update priority from current input */
                dPriority += (double)nValueIn * (double)txAge;

                debug::log(3, FUNCTION, "Updated transaction priority: nValueIn=%-12" PRI64d " txAge=%-5" PRI64d ", new dPriority=%-20.1f\n", nValueIn, txAge, dPriority);
            }

            /* Priority is sum (valuein * age) / txsize */
            dPriority /= ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);

            /* Map will be ordered with SMALLEST value first, so add using -dPriority as key so highest priority is first */
            mapPriority.emplace(std::make_pair(-dPriority, &tx));

            debug::log(3, FUNCTION, "Final priority %-20.1f %s\n%s", dPriority, txHash.ToString().substr(0,10).c_str(), tx.ToString().c_str());

        }

        /* Now have all the tx in mapPriority. Collect them into block vtx */
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;

        while (!mapPriority.empty())
        {
            /* Extract first entry from the priority map  */
            auto firstMapEntry = mapPriority.begin();
            Transaction& tx = *((*firstMapEntry).second); //dereferences the iterator to get map entry, then dereferences the tx pointer
            mapPriority.erase(firstMapEntry);

            uint32_t nTxSize = ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);
            uint32_t nTxSigOps = 0;
            uint64_t nTxFees = 0;
            uint64_t nMinFee = tx.GetMinFee(nBlockSize, false, Legacy::GMF_BLOCK);
            std::map<uint512_t, Transaction> mapInputs;

            /* Check block size limits */
            if ((nBlockSize + nTxSize) >= TAO::Ledger::MAX_BLOCK_SIZE_GEN)
            {
                debug::log(2, FUNCTION, "Block size limits reached on transaction %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
                continue;
            }

            /* Legacy limits on sigOps */
            nTxSigOps = tx.GetLegacySigOpCount();
            if (nBlockSigOps + nTxSigOps >= TAO::Ledger::MAX_BLOCK_SIGOPS)
            {
                debug::log(2, FUNCTION, "Too many legacy signature operations %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
                continue;
            }

            /* Timestamp limit */
            if (tx.nTime > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
            {
                debug::log(2, FUNCTION, "Transaction time too far in future %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
                continue;
            }

            /* Retrieve tx inputs */
            if (!tx.FetchInputs(mapInputs))
            {
                debug::log(2, FUNCTION, "Failed to get transaction inputs %s\n", tx.GetHash().ToString().substr(0, 10).c_str());

                vRemoveFromPool.push_back(tx.GetHash());
                continue;
            }

            /* Check tx fee meetes minimum fee requirement */ 
            nTxFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();
            if (nTxFees < nMinFee)
            {
                debug::log(2, FUNCTION, "Not enough fees %s\n", tx.GetHash().ToString().substr(0, 10).c_str());

                vRemoveFromPool.push_back(tx.GetHash());
                continue;
            }

            /* Check legacy limits on sigOps with inputs included */
            nTxSigOps += tx.TotalSigOps(mapInputs);
            if ((nBlockSigOps + nTxSigOps) >= TAO::Ledger::MAX_BLOCK_SIGOPS)
            {
                debug::log(2, FUNCTION, "Too many P2SH signature operations %s\n", tx.GetHash().ToString().substr(0, 10).c_str());

                vRemoveFromPool.push_back(tx.GetHash());
                continue;
            }

            if (!tx.Connect(mapInputs, prevBlockState))
            {
                debug::log(2, FUNCTION, "Failed to connect inputs %s\n", tx.GetHash().ToString().substr(0, 10).c_str());

                vRemoveFromPool.push_back(tx.GetHash());
                continue;
            }

            /* Add the current transaction to the block */
            vtx.push_back(tx);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;
        }

        // these were in old Core::global scope and used in RPC getmininginfo
        //nLastBlockTx = nBlockTx;
        //nLastBlockSize = nBlockSize;

        /* Remove any invalid transactions from the mempool */
        for (const uint512_t& txHashToRemove : vRemoveFromPool)
        {
            debug::log(0, FUNCTION, "Removed invalid tx %s from mempool\n", txHashToRemove.ToString().substr(0, 10).c_str());
            TAO::Ledger::mempool.RemoveLegacy(txHashToRemove);
        }
 
    }


    /* Sign the block with the key that found the block. */
    bool SignBlock(LegacyBlock& block, const CKeyStore& keystore)
    {
        std::vector<std::vector<uint8_t>> vSolutions;
        Legacy::TransactionType whichType;

        /* Retrieve the Coinbase/Coinstake transaction from the block */
        const CTxOut& txout = block.vtx[0].vout[0];

        /* Extract the public key from the Coinbase/Coinstake script */
        if (!Solver(txout.scriptPubKey, whichType, vSolutions))
            return false;

        if (whichType == Legacy::TX_PUBKEY)
        {
            /* Retrieve the public key returned by Solver */
            const std::vector<uint8_t>& vchPubKey = vSolutions[0];

            /* Use public key to get private key from keystore */
            LLC::ECKey key;
            if (!keystore.GetKey(LLC::SK256(vchPubKey), key))
            {
                debug::error(FUNCTION, "Key not found attempting to sign new block");
                return false;
            }

            /* Validate the key returned from keystore */
            if (key.GetPubKey() != vchPubKey)
            {
                debug::error(FUNCTION, "Invalid key attempting to sign new block");
                return false;
            }

            /* Sign the block */
            return block.GenerateSignature(key);
        }

        debug::error(FUNCTION, "Wrong key type attempting to sign new block");

        return false;
    }


    /* Work Check Before Submit. 
     *
     * This method no longer takes a ReserveKey or calls KeepKey. This change allows the stake minter
     * issue the call, then only call KeepKey on its ReserveKey when it is using a new key. 
     * The old CheckWork always called it, using up keys from key pool unnecessarily.
     *
     * Prime and hash mining code should also call ProcessBlock when the mining server is 
     * implemented to use this method. They would, of course, also need to call KeepKey after every
     * mined block, using a separate key for each new block as before.
     */
    bool CheckWork(const LegacyBlock& block, Legacy::Wallet& wallet)
    {
        uint32_t nChannel = block.GetChannel();
        uint1024_t blockHash = (block.nVersion < 5 ? block.GetHash() : nChannel == 0 ? block.StakeHash() : block.ProofHash());
        uint1024_t hashTarget = LLC::CBigNum().SetCompact(block.nBits).getuint1024();

        if(nChannel > 0 && !block.VerifyWork())
            return debug::error(FUNCTION, "Nexus Miner: Proof of work not meeting target.");

        if(nChannel == 0)
        {
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(block.nBits);

            if((block.nVersion < 5 ? block.GetHash() : block.StakeHash()) > bnTarget.getuint1024())
                return debug::error(FUNCTION, "Nexus Stake Minter: Proof of stake not meeting target");
        }

        std::string timestampString(DateTimeStrFormat(runtime::unifiedtimestamp()));
        if (nChannel == 0)
        {
            debug::log(1, FUNCTION, "Nexus Stake Minter: new nPoS channel block found at unified time %s", timestampString.c_str());
            debug::log(1, " blockHash: %s block height: ", blockHash.ToString().substr(0, 30).c_str(), block.nHeight);
        }
        else if (nChannel == 1)
        {
            debug::log(1, FUNCTION, "Nexus Miner: new Prime channel block found at unified time %s", timestampString.c_str());
            debug::log(1, "  blockHash: %s block height: ", blockHash.ToString().substr(0, 30).c_str(), block.nHeight);
            debug::log(1, "  prime cluster verified of size %f", TAO::Ledger::GetDifficulty(block.nBits, 1));
        }
        else if (nChannel == 2)
        {
            debug::log(1, FUNCTION, "Nexus Miner: new Hashing channel block found at unified time %s", timestampString.c_str());
            debug::log(1, "  blockHash: %s block height: ", blockHash.ToString().substr(0, 30).c_str(), block.nHeight);
            debug::log(1, "  target: %s", hashTarget.ToString().substr(0, 30).c_str());
        }

        if (block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain)
            return debug::error(FUNCTION, "Generated block is stale");

        /* Add new block to request tracking in wallet */
        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[block.GetHash()] = 0;
        }

        /* Print the newly found block. */
        block.print();

        /* Process the Block and relay to network if it gets Accepted into Blockchain. */
        if (!LLP::LegacyNode::Process(block, nullptr))
            return debug::error(FUNCTION, "Generated block not accepted");

        return true;
    }

}