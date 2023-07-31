/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/ambassador.h>
#include <Legacy/include/create.h>
#include <Legacy/include/enum.h>
#include <Legacy/include/evaluate.h>
#include <Legacy/types/address.h>
#include <Legacy/types/txin.h>

#include <LLC/hash/SK.h>
#include <LLC/include/eckey.h>
#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

#include <map>
#include <utility>

/* Global Legacy namespace. */
namespace Legacy
{

    /** Constructs a new legacy block **/
    bool CreateBlock(Legacy::ReserveKey& coinbaseKey, const Legacy::Coinbase& coinbaseRecipients,
                     const uint32_t nChannel, const uint32_t nID, LegacyBlock& newBlock)
    {
        newBlock.SetNull();

        /* Previous block state is current best state on chain */
        TAO::Ledger::BlockState prevBlockState = TAO::Ledger::ChainState::tStateBest.load();

        /* Modulate the Block Versions if they correspond to their proper time stamp */
        /* Normally, if condition is true and block version is current version unless an activation is pending */
        uint32_t nCurrent = TAO::Ledger::CurrentBlockVersion();
        if(runtime::unifiedtimestamp() >= TAO::Ledger::StartBlockTimelock(7))
            return debug::error(FUNCTION, "Cannot create Legacy block in Tritium.");
        else if(nCurrent >= 6)
            newBlock.nVersion = 6; // Maximum legacy block version is 6

        /* Coinbase / Coinstake Transaction. **/
        Transaction txNew;

        if(nChannel == 0)
        {
            /* Create the Coinstake transaction for Proof of Stake Channel. */
            if(!CreateCoinstake(txNew))
                return debug::error(FUNCTION, "Error creating coinstake transaction");

            /* Add coinstake to block. */
            newBlock.vtx.push_back(txNew);

            /* Proof of Stake does not add mempool transactions at this time */
        }

        else
        {
            /* Create the Coinbase transaction for Prime/Hash Channels. */
            if(!CreateCoinbase(coinbaseKey, coinbaseRecipients, nChannel, nID, newBlock.nVersion, txNew))
                return debug::error(FUNCTION, "Error creating coinbase transaction");

            /* Add coinbase to block. */
            newBlock.vtx.push_back(txNew);

            /* Add transactions from mempool */
            AddTransactions(newBlock.vtx);
       }


        /* Populate the Block Data. */
        std::vector<uint512_t> vMerkleTree;
        for(const auto& tx : newBlock.vtx)
            vMerkleTree.push_back(tx.GetHash());

        newBlock.hashPrevBlock  = prevBlockState.GetHash();
        newBlock.hashMerkleRoot = newBlock.BuildMerkleTree(vMerkleTree);
        newBlock.nChannel       = nChannel;
        newBlock.nHeight        = prevBlockState.nHeight + 1;
        newBlock.nBits          = GetNextTargetRequired(prevBlockState, nChannel, false);
        newBlock.nNonce         = 1;

        /* Update the time for the newly created block. */
        newBlock.UpdateTime();

        return true;
    }


    /* Create the coinstake transaction for a legacy block. */
    bool CreateCoinstake(Transaction& coinstakeTx)
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
        coinstakeTx.nTime = TAO::Ledger::ChainState::tStateBest.load().GetBlockTime() + 1;

        return true;
    }


    /* Create the Coinbase transaction for a legacy block. */
    bool CreateCoinbase(Legacy::ReserveKey& coinbaseKey, const Legacy::Coinbase& coinbaseRecipients, const uint32_t nChannel,
                                   const uint32_t nID, const uint32_t nNewBlockVersion, Transaction& coinbaseTx)
    {
        /* Previous block state is current best state on chain */
        TAO::Ledger::BlockState prevBlockState = TAO::Ledger::ChainState::tStateBest.load();

        /* Output type 0 is mining/minting reward */
        uint64_t nBlockReward = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 0);

        /* Initialize vin. */
        coinbaseTx.vin.resize(1);
        coinbaseTx.vin[0].prevout.SetNull();

        /* Set the Proof of Work Script Signature. */
        coinbaseTx.vin[0].scriptSig = (Legacy::Script() << ((uint64_t)nID * 513513512151));

        /* Calculate the reward for this wallet */
        uint64_t nReward = 0;
        if(coinbaseRecipients.IsNull())
            nReward = nBlockReward;
        else
            nReward = coinbaseRecipients.WalletReward();

        if(nReward > 0)
        {
            coinbaseTx.vout.resize(1);
            coinbaseTx.vout[0].scriptPubKey << coinbaseKey.GetReservedKey() << Legacy::OP_CHECKSIG;
            coinbaseTx.vout[0].nValue = nReward;
        }

        /* If additional coinbase recipients have been provided them add them to vout*/
        if(!coinbaseRecipients.IsNull())
        {
            /* Get the map of outputs for this coinbase. */
            std::map<std::string, uint64_t> mapOutputs = coinbaseRecipients.Outputs();
            uint32_t nTx = 1;

            coinbaseTx.vout.resize(mapOutputs.size() + coinbaseTx.vout.size());
            for(const auto& entry : mapOutputs)
            {
                /* Set the Appropriate Outputs. */
                coinbaseTx.vout[nTx].scriptPubKey.SetNexusAddress(NexusAddress(entry.first));
                coinbaseTx.vout[nTx].nValue = entry.second;

                ++nTx;
            }

            uint64_t nMiningReward = 0;
            for(uint32_t nIndex = 0; nIndex < coinbaseTx.vout.size(); ++nIndex)
                nMiningReward += coinbaseTx.vout[nIndex].nValue;

            /* Double Check the Coinbase Transaction Fits in the Maximum Value. */
            if(nMiningReward != nBlockReward)
                return debug::error("Mining reward ", nMiningReward, " does not match block reward ", nBlockReward);
        }


        /* Make coinbase counter mod 13 of height. */
        uint32_t nCoinbaseCounter = TAO::Ledger::ChainState::tStateBest.load().nHeight % 13;

        /* Create the ambassador and developer outputs for Coinbase transaction */
        coinbaseTx.vout.resize(coinbaseTx.vout.size() + 2);

        NexusAddress ambassadorKeyAddress(config::fTestNet.load()
                                            ? (nNewBlockVersion < 5 ? TESTNET_DUMMY_ADDRESS
                                                                    : TESTNET_DUMMY_AMBASSADOR_RECYCLED)
                                            : (nNewBlockVersion < 5 ? AMBASSADOR_ADDRESSES[nCoinbaseCounter]
                                                                    : AMBASSADOR_ADDRESSES_RECYCLED[nCoinbaseCounter]));

        NexusAddress devKeyAddress(config::fTestNet.load()
                                    ? (nNewBlockVersion < 5 ? TESTNET_DUMMY_ADDRESS
                                                            : TESTNET_DUMMY_DEVELOPER_RECYCLED)
                                    : (nNewBlockVersion < 5 ? DEVELOPER_ADDRESSES[nCoinbaseCounter]
                                                            : DEVELOPER_ADDRESSES_RECYCLED[nCoinbaseCounter]));

        coinbaseTx.vout[coinbaseTx.vout.size() - 2].scriptPubKey.SetNexusAddress(ambassadorKeyAddress);
        coinbaseTx.vout[coinbaseTx.vout.size() - 1].scriptPubKey.SetNexusAddress(devKeyAddress);

        /* Output type 1 is ambassador reward. */
        coinbaseTx.vout[coinbaseTx.vout.size() - 2].nValue = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 1);

        /* Output type 2 is dev reward. */
        coinbaseTx.vout[coinbaseTx.vout.size() - 1].nValue = TAO::Ledger::GetCoinbaseReward(prevBlockState, nChannel, 2);

        return true;
    }


    void AddTransactions(std::vector<Transaction>& vtx)
    {
        /* Previous block state is current best state on chain. */
        TAO::Ledger::BlockState prevBlockState = TAO::Ledger::ChainState::tStateBest.load();

        std::vector<uint512_t> vHashes;           // legacy tx hashes currently in mempool
        std::vector<uint512_t> vRemove;          // invalid tx to remove from mempool
        std::multimap<double, Transaction> mapPriority; // processing priority for mempool tx
        uint64_t nFees = 0;

        /* Retrieve list of transaction hashes from mempool. Limit list to a sane size that would typically more than fill a
         * legacy block, rather than pulling entire pool if it is very large. */
        if(!TAO::Ledger::mempool.List(vHashes, 1000, true))
            return;

        /* Process the mempool transactions and load them into mapPriority for processing. */
        for(const uint512_t& txHash : vHashes)
        {
            Transaction tx;

            /* Retrieve next transaction from the mempool */
            if(!TAO::Ledger::mempool.Get(txHash, tx))
            {
                /* Should never have free floating Coinbase or Coinstake transaction in the mempool. */
                debug::error(FUNCTION, "Unable to read transaction from mempool ", txHash.SubString(10));
                continue;
            }

            if(tx.IsCoinBase() || tx.IsCoinStake())
            {
                /* Should never have free floating Coinbase or Coinstake transaction in the mempool. */
                debug::log(2, FUNCTION, "Mempool transaction is Coinbase/Coinstake ", txHash.SubString(10));
                continue;
            }

            if(!tx.IsFinal())
            {
                debug::log(2, FUNCTION, "Mempool transaction is not Final ", txHash.SubString(10));
                continue;
            }

            /* Calculate priority using transaction inputs. */
            double dPriority = 0;
            for(const TxIn& txin : tx.vin)
            {
                /* Check if we have previous transaction. */
                Transaction txPrev;
                if(!LLD::Legacy->ReadTx(txin.prevout.hash, txPrev))
                {
                    /* Skip any orphan transactions in the mempool until the prevout tx is confirmed. */
                    debug::log(2, FUNCTION, "Found orphan transaction in mempool ", txHash.SubString(10));
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

                //debug::log(3, FUNCTION, "Updated transaction priority: nValueIn=%-12" PRI64d " txAge=%-5" PRI64d ", new dPriority=%-20.1f\n", nValueIn, txAge, dPriority);
                debug::log(3, FUNCTION, "Updated transaction priority: nValueIn=", nValueIn, ", txAge=", txAge, ", new dPriority=", dPriority);
            }

            /* Priority is sum (valuein * age) / txsize */
            dPriority /= ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);

            /* Map will be ordered with SMALLEST value first, so add using -dPriority as key so highest priority is first */
            mapPriority.emplace(std::make_pair(-dPriority, tx));

            //debug::log(3, FUNCTION, "Final priority %-20.1f %s\n%s", dPriority, txHash.SubString(10), tx.ToString().c_str());
            debug::log(3, FUNCTION, "Final priority ", dPriority, " ", txHash.SubString(10), "\n", tx.ToString());

        }

        /* Now have all the tx in mapPriority. Collect them into block vtx */
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;

        while(!mapPriority.empty())
        {
            /* Extract first entry from the priority map  */
            auto firstMapEntry = mapPriority.begin();
            Transaction tx = firstMapEntry->second;
            mapPriority.erase(firstMapEntry);

            uint32_t nTxSize = ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);
            uint32_t nTxSigOps = 0;
            uint64_t nTxFees = 0;
            uint64_t nMinFee = tx.GetMinFee(nBlockSize, false, Legacy::GMF_BLOCK);
            std::map<uint512_t, std::pair<uint8_t, DataStream> > mapInputs;

            /* Check block size limits */
            if((nBlockSize + nTxSize) >= TAO::Ledger::MAX_BLOCK_SIZE_GEN)
            {
                debug::log(2, FUNCTION, "Block size limits reached on transaction ", tx.GetHash().SubString(10));
                continue;
            }

            /* Legacy limits on sigOps */
            nTxSigOps = tx.GetLegacySigOpCount();
            if(nBlockSigOps + nTxSigOps >= TAO::Ledger::MAX_BLOCK_SIGOPS)
            {
                debug::log(2, FUNCTION, "Too many legacy signature operations ", tx.GetHash().SubString(10));
                continue;
            }

            /* Timestamp limit. If before v7 activation, keep legacy tx compatible with legacy setting. */
            if(tx.nTime > runtime::unifiedtimestamp() + runtime::maxdrift())
            {
                debug::log(2, FUNCTION, "Transaction time too far in future ", tx.GetHash().SubString(10));
                continue;
            }

            /* Retrieve tx inputs */
            if(!tx.FetchInputs(mapInputs))
            {
                debug::log(2, FUNCTION, "Failed to get transaction inputs ", tx.GetHash().SubString(10));

                vRemove.push_back(tx.GetHash());
                continue;
            }

            /* Check tx fee meets minimum fee requirement */
            nTxFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();
            if(nTxFees < nMinFee)
            {
                debug::log(2, FUNCTION, "Not enough fees ", tx.GetHash().SubString(10));

                vRemove.push_back(tx.GetHash());
                continue;
            }

            /* Check legacy limits on sigOps with inputs included */
            nTxSigOps += tx.TotalSigOps(mapInputs);
            if((nBlockSigOps + nTxSigOps) >= TAO::Ledger::MAX_BLOCK_SIGOPS)
            {
                debug::log(2, FUNCTION, "Too many P2SH signature operations ", tx.GetHash().SubString(10));

                vRemove.push_back(tx.GetHash());
                continue;
            }

            if(!tx.Connect(mapInputs, prevBlockState))
            {
                debug::log(2, FUNCTION, "Failed to connect inputs ", tx.GetHash().SubString(10));

                vRemove.push_back(tx.GetHash());
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
        for(const uint512_t& hashRemove : vRemove)
        {
            debug::log(0, FUNCTION, "Removed invalid tx ", hashRemove.SubString(10), " from mempool");
            TAO::Ledger::mempool.Remove(hashRemove);
        }

    }


#ifndef NO_WALLET

    /* Sign the block with the key that found the block. */
    bool SignBlock(LegacyBlock& block, const KeyStore& keystore)
    {
        std::vector<std::vector<uint8_t>> vSolutions;
        Legacy::TransactionType whichType;

        /* Retrieve the Coinbase/Coinstake transaction from the block */
        const TxOut& txout = block.vtx[0].vout[0];

        /* Extract the public key from the Coinbase/Coinstake script */
        if(!Solver(txout.scriptPubKey, whichType, vSolutions))
            return false;

        if(whichType == Legacy::TX_PUBKEY)
        {
            /* Retrieve the public key returned by Solver */
            const std::vector<uint8_t>& vchPubKey = vSolutions[0];

            /* Use public key to get private key from keystore */
            LLC::ECKey key;
            if(!keystore.GetKey(LLC::SK256(vchPubKey), key))
            {
                debug::error(FUNCTION, "Key not found attempting to sign new block");
                return false;
            }

            /* Validate the key returned from keystore */
            if(key.GetPubKey() != vchPubKey)
            {
                debug::error(FUNCTION, "Invalid key attempting to sign new block");
                return false;
            }

            /* Sign the block */
            block.GenerateSignature(key);

            /* Ensure the signed block is a valid signature */
            return block.VerifySignature(key);
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
        /* Precompute the block's channel. */
        uint32_t nChannel = block.GetChannel();

        /* Precompute the block's hash based on proof hash, get hash, or stake hash. */
        uint1024_t hashBlock = (block.nVersion < 5 ? block.GetHash() : nChannel == 0 ? block.StakeHash() : block.ProofHash());
        uint1024_t hashTarget = LLC::CBigNum().SetCompact(block.nBits).getuint1024();

        /* Verify the work completed. */
        if(nChannel > 0 && !block.VerifyWork())
            return debug::error(FUNCTION, "Nexus Miner: Proof of work not meeting target.");

        /* Check stake targets based on version. */
        if(nChannel == 0)
        {
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(block.nBits);

            if((block.nVersion < 5 ? block.GetHash() : block.StakeHash()) > bnTarget.getuint1024())
                return debug::error(FUNCTION, "Nexus Stake Minter: Proof of stake not meeting target");
        }

        /* Output for stake channel. */
        std::string strTimestamp(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));
        if(nChannel == 0)
        {
            debug::log(1, FUNCTION, "Nexus Stake Minter: new nPoS channel block found at unified time ", strTimestamp);
            debug::log(1, " hashBlock: ", hashBlock.SubString(30), " block height: ", block.nHeight);
        }

        /* Output for prime channel. */
        else if(nChannel == 1)
        {
            debug::log(1, FUNCTION, "Nexus Miner: new Prime channel block found at unified time ", strTimestamp);
            debug::log(1, "  hashBlock: ", hashBlock.SubString(30), " block height: ", block.nHeight);
            debug::log(1, "  prime cluster verified of size ", TAO::Ledger::GetDifficulty(block.nBits, 1));
        }

        /* Output for hashing channel. */
        else if(nChannel == 2)
        {
            debug::log(1, FUNCTION, "Nexus Miner: new Hashing channel block found at unified time ", strTimestamp);
            debug::log(1, "  hashBlock: ", hashBlock.SubString(30), " block height: ", block.nHeight);
            debug::log(1, "  target: ", hashTarget.SubString(30));
        }

        /* Check for a stale block. */
        if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
            return debug::error(FUNCTION, "Generated block is stale");

        /* Process the Block and relay to network if it gets Accepted into Blockchain. */
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        /* Check the statues. */
        if(!(nStatus & TAO::Ledger::PROCESS::ACCEPTED))
            return debug::error(FUNCTION, "Generated block not accepted");

        return true;
    }

#endif

}
