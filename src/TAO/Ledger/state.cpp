/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <string>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/supply.h>




namespace TAO::Ledger
{

    /* Get the block state object. */
    bool GetLastState(BlockState& state, uint32_t nChannel)
    {
        for(uint32_t index = 0; index < 10000; index++) //set limit on searchable blocks
        {
            state = state.Prev();
            if(state.GetChannel() == nChannel)
                return true;
        }

        return false;
    }


    /* Get the previous block state in chain. */
    BlockState BlockState::Prev() const
    {
        BlockState state;
        if(!LLD::legDB->ReadBlock(hashPrevBlock, state))
            return state;

        return state;
    }


    /* Get the next block state in chain. */
    BlockState BlockState::Next() const
    {
        BlockState state;
        if(hashNextBlock == 0)
            return state;

        if(!LLD::legDB->ReadBlock(hashNextBlock, state))
            return state;

        return state;
    }

    /* Accept a block state into chain. */
    bool BlockState::Accept()
    {
        /* Read leger DB for duplicate block. */
        BlockState state;
        if(LLD::legDB->ReadBlock(GetHash(), state))
            return debug::error(FUNCTION "block state already exists", __PRETTY_FUNCTION__);


        /* Read leger DB for previous block. */
        BlockState statePrev = Prev();
        if(statePrev.IsNull())
            return debug::error(FUNCTION "previous block state not found", __PRETTY_FUNCTION__);


        /* Check the Height of Block to Previous Block. */
        if(statePrev.nHeight + 1 != nHeight)
            return debug::error(FUNCTION "incorrect block height.", __PRETTY_FUNCTION__);


        /* Get the proof hash for this block. */
        uint1024_t hash = (nVersion < 5 ? GetHash() : GetChannel() == 0 ? StakeHash() : ProofHash());


        /* Get the target hash for this block. */
        uint1024_t hashTarget = LLC::CBigNum().SetCompact(nBits).getuint1024();


        /* Verbose logging of proof and target. */
        debug::log(2, "  proof:  %s", hash.ToString().substr(0, 30).c_str());


        /* Channel switched output. */
        if(GetChannel() == 1)
            debug::log(2, "  prime cluster verified of size %f", GetDifficulty(nBits, 1));
        else
            debug::log(2, "  target: %s", hashTarget.ToString().substr(0, 30).c_str());


        /* Check that the nBits match the current Difficulty. **/
        if (nBits != GetNextTargetRequired(statePrev, GetChannel()))
            return debug::error(FUNCTION "incorrect proof-of-work/proof-of-stake", __PRETTY_FUNCTION__);


        /* Check That Block timestamp is not before previous block. */
        if (GetBlockTime() <= statePrev.GetBlockTime())
            return debug::error(FUNCTION "block's timestamp too early Block: %" PRId64 " Prev: %" PRId64 "", __PRETTY_FUNCTION__,
            GetBlockTime(), statePrev.GetBlockTime());


        /* Check that Block is Descendant of Hardened Checkpoints. */
        if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
            return debug::error(FUNCTION "not descendant of last checkpoint", __PRETTY_FUNCTION__);


        /* Check the block proof of work rewards. */
        if(IsProofOfWork() && nVersion >= 3)
        {
            /* Get the stream from coinbase. */
            producer.ssOperation.seek(33, STREAM::BEGIN); //set the read position to where reward will be.

            /* Read the mining reward. */
            uint64_t nMiningReward;
            producer.ssOperation >> nMiningReward;

            /* Check that the Mining Reward Matches the Coinbase Calculations. */
            if (nMiningReward != GetCoinbaseReward(statePrev, GetChannel(), 0))
                return debug::error(FUNCTION "miner reward mismatch %" PRId64 " : %" PRId64 "",
                    __PRETTY_FUNCTION__, nMiningReward, GetCoinbaseReward(statePrev, GetChannel(), 0));
        }
        else if (IsProofOfStake())
        {
            /* Check that the Coinbase / CoinstakeTimstamp is after Previous Block. */
            if (producer.nTimestamp < statePrev.GetBlockTime())
                return debug::error(FUNCTION "coinstake transaction too early", __PRETTY_FUNCTION__);
        }


        //TODO: check legacy transactions for finality

        /* Compute the Chain Trust */
        nChainTrust = statePrev.nChainTrust + GetBlockTrust();


        /* Compute the Channel Height. */
        BlockState stateLast = statePrev;
        if(!GetLastState(stateLast, GetChannel()))
            nChannelHeight = 1;
        else
            nChannelHeight = stateLast.nChannelHeight + 1;


        /* Compute the Released Reserves. */
        for(int nType = 0; nType < 3; nType++)
        {
            if(IsProofOfWork())
            {
                /* Calculate the Reserves from the Previous Block in Channel's reserve and new Release. */
                uint64_t nReserve  = stateLast.nReleasedReserve[nType] +
                    GetReleasedReserve(*this, GetChannel(), nType);

                /* Get the coinbase rewards. */
                uint64_t nCoinbaseRewards = GetCoinbaseReward(statePrev, GetChannel(), nType);

                /* Block Version 3 Check. Disable Reserves from going below 0. */
                if(nVersion >= 3 && nCoinbaseRewards >= nReserve)
                    return debug::error(FUNCTION "out of reserve limits", __PRETTY_FUNCTION__);

                /* Check coinbase rewards. */
                nReleasedReserve[nType] =  nReserve - nCoinbaseRewards;

                debug::log(2, "Reserve Balance %i | %f Nexus | Released %f", nType, stateLast.nReleasedReserve[nType] / 1000000.0, (nReserve - stateLast.nReleasedReserve[nType]) / 1000000.0 );
            }
            else
                nReleasedReserve[nType] = 0;

        }

        /* Add the Pending Checkpoint into the Blockchain. */
        if(IsNewTimespan(Prev()))
        {
            hashCheckpoint = GetHash();

            debug::log(0, "===== New Pending Checkpoint Hash = %s", hashCheckpoint.ToString().substr(0, 15).c_str());
        }
        else
        {
            hashCheckpoint = stateLast.hashCheckpoint;

            debug::log(0, "===== Pending Checkpoint Hash = %s", hashCheckpoint.ToString().substr(0, 15).c_str());
        }


        /* Signal to set the best chain. */
        if(nChainTrust > ChainState::nBestChainTrust)
        {
            /* Start the database transaction. */
            LLD::legDB->TxnBegin();
            LLD::regDB->TxnBegin();

            /* Watch for genesis. */
            if (ChainState::stateGenesis.IsNull())
            {
                /* Write the best chain pointer. */
                if(!LLD::legDB->WriteBestChain(hash))
                    return debug::error(FUNCTION "failed to write best chain", __PRETTY_FUNCTION__);

                /* Write the block to disk. */
                if(LLD::legDB->WriteBlock(GetHash(), *this))
                    return debug::error(FUNCTION "block state already exists", __PRETTY_FUNCTION__);

                /* Set the genesis block. */
                ChainState::stateGenesis = *this;
            }
            else
            {
                /* Get initial block states. */
                BlockState fork   = ChainState::stateBest;
                BlockState longer = *this;

                /* Get the blocks to connect and disconnect. */
                std::vector<BlockState> vDisconnect;
                std::vector<BlockState> vConnect;
                while (fork != longer)
                {
                    /* Find the root block in common. */
                    while (longer.nHeight > fork.nHeight)
                    {
                        /* Add to connect queue. */
                        vConnect.push_back(longer);

                        /* Iterate backwards in chain. */
                        longer = longer.Prev();
                        if(longer.IsNull())
                            return debug::error(FUNCTION "failed to find longer ancestor block", __PRETTY_FUNCTION__);
                    }

                    /* Break if found. */
                    if (fork == longer)
                        break;

                    /* Iterate backwards to find fork. */
                    vDisconnect.push_back(fork);
                    fork = fork.Prev();
                    if(fork.IsNull())
                        return debug::error(FUNCTION "failed to find ancestor fork block", __PRETTY_FUNCTION__);
                }

                /* Log if there are blocks to disconnect. */
                if(vDisconnect.size() > 0)
                {
                    debug::log(0, FUNCTION "REORGANIZE: Disconnect %i blocks; %s..%s\n", __PRETTY_FUNCTION__,
                        vDisconnect.size(), fork.GetHash().ToString().substr(0,20).c_str(), ChainState::stateBest.GetHash().ToString().substr(0,20).c_str());

                    debug::log(0, FUNCTION "REORGANIZE: Connect %i blocks; %s..%s\n", __PRETTY_FUNCTION__,
                        vConnect.size(), fork.GetHash().ToString().substr(0,20).c_str(),
                        this->GetHash().ToString().substr(0,20).c_str());
                }

                /* List of transactions to resurrect. */
                std::vector<uint512_t> vResurrect;

                /* Disconnect given blocks. */
                for(auto & state : vDisconnect)
                {
                    /* Connect the block. */
                    if(!state.Disconnect())
                        return debug::error(FUNCTION "failed to disconnect %s", __PRETTY_FUNCTION__,
                            state.GetHash().ToString().substr(0, 20).c_str());

                    /* Add transactions into memory pool. */
                    for(auto tx : state.vtx)
                        vResurrect.push_back(tx.second);
                }

                /* List of transactions to remove from pool. */
                std::vector<uint512_t> vDelete;

                /* Set the next hash from fork. */
                fork.hashNextBlock = vConnect[0].GetHash();

                /* Reverse the blocks to connect to connect in ascending height. */
                std::reverse(vConnect.begin(), vConnect.end());
                for(auto & state : vConnect)
                {
                    /* Connect the block. */
                    if(!state.Connect())
                        return debug::error(FUNCTION "failed to connect %s", __PRETTY_FUNCTION__,
                            state.GetHash().ToString().substr(0, 20).c_str());

                    /* Remove transactions from memory pool. */
                    for(auto tx : state.vtx)
                        vDelete.push_back(tx.second);

                    /* Harden a checkpoint if there is any. */
                    HardenCheckpoint(state);
                }

                /* Write the best chain pointer. */
                if(!LLD::legDB->WriteBestChain(hash))
                    return debug::error(FUNCTION "failed to write best chain", __PRETTY_FUNCTION__);


                /* Remove transactions from memory pool. */
                for(auto & hashTx : vDelete)
                    mempool.Remove(hashTx);

                /* Add transaction back to memory pool. */
                for(auto & hashTx : vResurrect)
                {
                    /* Check if in memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::legDB->ReadTx(hashTx, tx))
                        return debug::error(FUNCTION "transaction is not on disk", __PRETTY_FUNCTION__);

                    /* Add to the mempool. */
                    mempool.Accept(tx);
                }

                /* Write the block to disk. */
                if(LLD::legDB->WriteBlock(GetHash(), *this))
                    return debug::error(FUNCTION "block state already exists", __PRETTY_FUNCTION__);


                /* Set the best chain variables. */
                ChainState::stateBest          = *this;
                ChainState::hashBestChain      = hash;
                ChainState::nBestChainTrust    = nChainTrust;
                ChainState::nBestHeight        = nHeight;


                /* Debug output about the best chain. */
                debug::log(0, FUNCTION "new best block = %s height = %u trust=%" PRIu64, hash.ToString().substr(0, 20).c_str(), nHeight, nChainTrust);


                //TODO: blocknotify
                //TODO: broadcast to nodes
            }

            /* Commit the transaction to database. */
            LLD::legDB->TxnCommit();
            LLD::regDB->TxnCommit();
        }

        return true;
    }


    /** Connect a block state into chain. **/
    bool BlockState::Connect()
    {
        /* Check through all the transactions. */
        for(auto tx : vtx)
        {
            /* Only work on tritium transactions for now. */
            if(tx.first == TYPE::TRITIUM_TX)
            {
                /* Get the transaction hash. */
                uint512_t hash = tx.second;

                /* Check if in memory pool. */
                TAO::Ledger::Transaction tx;
                if(!mempool.Get(hash, tx))
                    return debug::error(FUNCTION "transaction is not in memory pool", __PRETTY_FUNCTION__);

                /* Execute the operations layers. */
                if(!TAO::Register::Verify(tx))
                    return debug::error(FUNCTION "transaction register layer failed to verify", __PRETTY_FUNCTION__);

                /* Execute the operations layers. */
                if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::WRITE))
                    return debug::error(FUNCTION "transaction operation layer failed to execute", __PRETTY_FUNCTION__);

                /* Write to disk. */
                if(!LLD::legDB->ReadTx(hash, tx))
                    return debug::error(FUNCTION "transaction is not on disk", __PRETTY_FUNCTION__);
            }
        }

        /* Update the previous state's next pointer. */
        BlockState prev = Prev();
        if(!prev.IsNull())
        {
            prev.hashNextBlock = GetHash();
            LLD::legDB->WriteBlock(prev.GetHash(), prev);
        }

        return true;
    }


    /** Disconnect a block state from the chain. **/
    bool BlockState::Disconnect()
    {
        /* Check through all the transactions. */
        for(auto tx : vtx)
        {
            /* Only work on tritium transactions for now. */
            if(tx.first == TYPE::TRITIUM_TX)
            {
                /* Get the transaction hash. */
                uint512_t hash = tx.second;

                /* Check if in memory pool. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hash, tx))
                    return debug::error(FUNCTION "transaction is not on disk", __PRETTY_FUNCTION__);

                /* Execute the operations layers. */
                if(!TAO::Register::Rollback(tx))
                    return debug::error(FUNCTION "transaction register layer failed to rollback", __PRETTY_FUNCTION__);
            }
        }

        /* Update the previous state's next pointer. */
        BlockState prev = Prev();
        if(!prev.IsNull())
        {
            prev.hashNextBlock = 0;
            LLD::legDB->WriteBlock(prev.GetHash(), prev);
        }

        return true;
    }


    /* USed to determine the trust of a block in the chain. */
    uint64_t BlockState::GetBlockTrust() const
    {
        /** Give higher block trust if last block was of different channel **/
        BlockState prev = Prev();
        if(!prev.IsNull() && prev.GetChannel() != GetChannel())
            return 3;

        return 1;
    }


    /* Function to determine if this block has been connected into the main chain. */
    bool BlockState::IsInMainChain() const
    {
        return (hashNextBlock != 0 || GetHash() == ChainState::hashBestChain);
    }


    /* For debugging Purposes seeing block state data dump */
    std::string BlockState::ToString(uint8_t nState) const
    {
        std::string strDebug = "";

        /* Handle verbose output for just header. */
        if(nState & debug::flags::header)
        {
            strDebug += debug::strprintf("Block("
            VALUE("nVersion") " = %u, "
            VALUE("hashPrevBlock") " = %s, "
            VALUE("hashMerkleRoot") " = %s, "
            VALUE("nChannel") " = %u, "
            VALUE("nHeight") " = %u, "
            VALUE("nDiff") " = %f, "
            VALUE("nNonce") " = %" PRIu64 ", "
            VALUE("nTime") " = %u, "
            VALUE("blockSig") " = %s",
            nVersion, hashPrevBlock.ToString().substr(0, 20).c_str(),
            hashMerkleRoot.ToString().substr(0, 20).c_str(), nChannel,
            nHeight, GetDifficulty(nBits, nChannel), nNonce, nTime, HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
        }

        /* Handle the verbose output for chain state. */
        if(nState & debug::flags::chain)
        {
            strDebug += debug::strprintf(", "
            VALUE("nChainTrust") " = %" PRIu64 ", "
            VALUE("nMoneySupply") " = %" PRIu64 ", "
            VALUE("nChannelHeight") " = %" PRIu32 ", "
            VALUE("nMinerReserve") " = %" PRIu32 ", "
            VALUE("nAmbassadorReserve") " = %" PRIu32 ", "
            VALUE("nDeveloperReserve") " = %" PRIu32 ", "
            VALUE("hashNextBlock") " = %s, "
            VALUE("hashCheckpoint") " = %s",
            nChainTrust, nMoneySupply, nChannelHeight, nReleasedReserve[0], nReleasedReserve[1],
            nReleasedReserve[2], hashNextBlock.ToString().substr(0, 20).c_str(),
            hashCheckpoint.ToString().substr(0, 20).c_str());
        }
        strDebug += ")\n";

        /* Handle the verbose output for transactions. */
        if(nState & debug::flags::tx)
        {
            for(auto tx : vtx)
                strDebug += debug::strprintf("Proof(nType = %u, hash = %s)\n", tx.first, tx.second);
        }

        return strDebug;
    }


    /* For debugging purposes, printing the block to stdout */
    void BlockState::print(uint8_t nState) const
    {
        debug::log(0, ToString(nState).c_str());
    }

}
