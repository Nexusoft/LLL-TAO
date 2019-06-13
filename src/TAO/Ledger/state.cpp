/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <string>

#include <LLD/include/global.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/prime.h>

#include <Util/include/string.h>



/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Get the block state object. */
        bool GetLastState(BlockState &state, uint32_t nChannel)
        {
            /* Get the genesis block hash. */
            uint1024_t hashGenesis =  ChainState::Genesis();

            /* Loop back 1440 blocks. */
            for(uint_t i = 0; i < 1440; ++i)
            {
                /* Return false on genesis. */
                if(state.GetHash() == hashGenesis)
                    return false;

                /* Return true on channel found. */
                if(state.GetChannel() == nChannel)
                    return true;

                /* Iterate backwards. */
                state = state.Prev();
                if(!state)
                    return false;
            }

            /* If the max depth expired, return the genesis. */
            state = ChainState::stateGenesis;

            return false;
        }


        /** Default Constructor. **/
        BlockState::BlockState(const TritiumBlock& block)
        : Block(block)
        , ssSystem()
        , vtx(block.vtx.begin(), block.vtx.end())
        , nChainTrust(0)
        , nMoneySupply(0)
        , nMint(0)
        , nFees(0)
        , nChannelHeight(0)
        , nChannelWeight{0, 0, 0}
        , nReleasedReserve{0, 0, 0}
        , nFeeReserve(0)
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
            /* Set producer to be last transaction. */
            vtx.push_back(std::make_pair(TYPE::TRITIUM_TX, block.producer.GetHash()));

            /* Check that sizes are expected. */
            if(vtx.size() != block.vtx.size() + 1)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "tritium block to state incorrect sizes"));
        }


        /* Construct a block state from a legacy block. */
        BlockState::BlockState(const Legacy::LegacyBlock& block)
        : Block(block)
        , ssSystem()
        , vtx()
        , nChainTrust(0)
        , nMoneySupply(0)
        , nMint(0)
        , nFees(0)
        , nChannelHeight(0)
        , nChannelWeight{0, 0, 0}
        , nReleasedReserve{0, 0, 0}
        , nFeeReserve(0)
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
            for(const auto& tx : block.vtx)
                vtx.push_back(std::make_pair(TYPE::LEGACY_TX, tx.GetHash()));

            if(vtx.size() != block.vtx.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "legacy block to state incorrect sizes"));
        }


        /* Get the previous block state in chain. */
        BlockState BlockState::Prev() const
        {
            /* Check for genesis. */
            if(hashPrevBlock == 0)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "called on genesis"));

            /* Read the previous block from ledger. */
            BlockState state;
            if(!LLD::Ledger->ReadBlock(hashPrevBlock, state))
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to read previous block state ", hashPrevBlock.SubString()));

            return state;
        }


        /* Get the next block state in chain. */
        BlockState BlockState::Next() const
        {
            /* Check for genesis. */
            if(hashNextBlock == 0)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "called on best block"));

            /* Read next block from the ledger. */
            BlockState state;
            if(!LLD::Ledger->ReadBlock(hashNextBlock, state))
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to read next block state ", hashNextBlock.SubString()));

            return state;
        }


        /* Accept a block state into chain. */
        bool BlockState::Index()
        {
            /* Read leger DB for previous block. */
            BlockState statePrev = Prev();
            if(!statePrev)
                return debug::error(FUNCTION, hashPrevBlock.SubString(), " block state not found");

            /* Compute the Chain Weight. */
            //debug::log(0, "Weight Limit: ", std::numeric_limits<uint64_t>::max());
            for(uint32_t n = 0; n < 3; ++n)
            {
                nChannelWeight[n] = statePrev.nChannelWeight[n];
                //debug::log(0, "Weight ", n == 0 ? "Stake" : (n == 1 ? "Prime" : "Hash "), ": ", nChannelWeight[n]);
            }

            /* Find the last block of this channel. */
            BlockState stateLast = statePrev;
            GetLastState(stateLast, GetChannel());

            /* Compute the Channel Height. */
            nChannelHeight = stateLast.nChannelHeight + 1;

            /* Compute the Released Reserves. */
            if(IsProofOfWork())
            {
                /* Calculate the coinbase rewards from the coinbase transaction. */
                uint64_t nCoinbaseRewards[3] = { 0, 0, 0 };
                if(vtx[0].first == TYPE::LEGACY_TX)
                {
                    /* Get the coinbase from the memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(vtx[0].second, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "cannot get coinbase tx");

                    /* Get the size of the coinbase. */
                    uint32_t nSize = tx.vout.size();

                    /* Add up the Miner Rewards from Coinbase Tx Outputs. */
                    for(uint32_t n = 0; n < nSize - 2; ++n)
                        nCoinbaseRewards[0] += tx.vout[n].nValue;

                    /* Get the ambassador and developer coinbase. */
                    nCoinbaseRewards[1] = tx.vout[nSize - 2].nValue;
                    nCoinbaseRewards[2] = tx.vout[nSize - 1].nValue;
                }
                else
                {
                    /* Get the coinbase from the memory pool. */
                    Transaction tx;
                    if(!LLD::Ledger->ReadTx(vtx.back().second, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "cannot get coinbase tx");

                    /* Loop through the contracts. */
                    uint32_t nSize = tx.Size();
                    for(uint32_t n = 0; n < nSize; ++n)
                    {
                        /* Get the reward. */
                        uint64_t nValue = 0;
                        if(!tx[n].Value(nValue))
                            return debug::error(FUNCTION, "no value in contract");

                        nCoinbaseRewards[0] += nValue;
                    }
                }

                /* Calculate the new reserve amounts. */
                for(int nType = 0; nType < 3; ++nType)
                {
                    /* Calculate the Reserves from the Previous Block in Channel's reserve and new Release. */
                    uint64_t nReserve  = stateLast.nReleasedReserve[nType] +
                        GetReleasedReserve(*this, GetChannel(), nType);

                    /* Block Version 3 Check. Disable Reserves from going below 0. */
                    if(nVersion != 2 && nCoinbaseRewards[nType] >= nReserve)
                        return debug::error(FUNCTION, "out of reserve limits");

                    /* Check coinbase rewards. */
                    nReleasedReserve[nType] =  (nReserve - nCoinbaseRewards[nType]);

                    /* Verbose output. */
                    debug::log(2, "Reserve Balance ", nType, " | ",
                        std::fixed, nReleasedReserve[nType] / (double)TAO::Ledger::NXS_COIN,
                        " Nexus | Released ",
                        std::fixed, (nReserve - stateLast.nReleasedReserve[nType]) / (double)TAO::Ledger::NXS_COIN);
                }
            }

            /* Add the Pending Checkpoint into the Blockchain. */
            if(IsNewTimespan(statePrev))
            {
                /* Set new checkpoint hash. */
                hashCheckpoint = GetHash();

                /* Verbose output. */
                debug::log(1, "===== New Pending Checkpoint Hash = ", hashCheckpoint.ToString().substr(0, 15));
            }
            else
            {
                /* Continue the old checkpoint through chain. */
                hashCheckpoint = statePrev.hashCheckpoint;

                /* Verbose output. */
                debug::log(1, "===== Pending Checkpoint Hash = ", hashCheckpoint.ToString().substr(0, 15));
            }

            /* Start the database transaction. */
            LLD::TxnBegin();

            /* Write the transactions. */
            for(const auto& proof : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(proof.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Write to disk. */
                    if(!LLD::Ledger->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                    /* Remove indexed tx from memory pool. */
                    mempool.Remove(hash);

                }
                else if(proof.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Check if in memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Write to disk. */
                    if(!LLD::Legacy->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                    /* Remove indexed tx from memory pool. */
                    mempool.Remove(hash);
                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");
            }

            /* Add new weights for this channel. */
            nChannelWeight[nChannel] += Weight();

            /* Compute the chain trust. */
            nChainTrust = statePrev.nChainTrust + Trust();

            /* Write the block to disk. */
            if(!LLD::Ledger->WriteBlock(GetHash(), *this))
                return debug::error(FUNCTION, "block state failed to write");

            /* Signal to set the best chain. */
            if(nVersion >= 7)
            {
                /* Set the chain trust. */
                uint8_t nTotal = 0;

                /* Check to best state. */
                for(uint32_t n = 0; n < 3; ++n)
                {
                    /* Check each weight. */
                    if(nChannelWeight[nChannel] >= ChainState::stateBest.load().nChannelWeight[n])
                        ++nTotal;
                }

                /* Check for best chain. */
                if(nTotal == 3 && !SetBest())
                    return debug::error(FUNCTION, "failed to set best chain");
            }
            else if(nChainTrust > ChainState::nBestChainTrust.load() && !SetBest())
                return debug::error(FUNCTION, "failed to set best chain");

            /* Commit the transaction to database. */
            LLD::TxnCommit();

            /* Debug output. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, "ACCEPTED");

            return true;
        }


        bool BlockState::SetBest()
        {
            /* Runtime calculations. */
            runtime::timer timer;
            timer.Start();

            /* Get the hash. */
            uint1024_t nHash = GetHash();

            /* Watch for genesis. */
            if(!ChainState::stateGenesis)
            {
                /* Write the best chain pointer. */
                if(!LLD::Ledger->WriteBestChain(nHash))
                    return debug::error(FUNCTION, "failed to write best chain");

                /* Write the block to disk. */
                if(!LLD::Ledger->WriteBlock(nHash, *this))
                    return debug::error(FUNCTION, "block state already exists");

                /* Set the genesis block. */
                ChainState::stateGenesis = *this;
            }
            else
            {
                /* Get initial block states. */
                BlockState fork   = ChainState::stateBest.load();
                BlockState longer = *this;

                /* Get the blocks to connect and disconnect. */
                std::vector<BlockState> vDisconnect;
                std::vector<BlockState> vConnect;
                while(fork != longer)
                {
                    /* Find the root block in common. */
                    while(longer.nHeight > fork.nHeight)
                    {
                        /* Add to connect queue. */
                        vConnect.push_back(longer);

                        /* Iterate backwards in chain. */
                        longer = longer.Prev();
                        if(!longer)
                        {
                            /* Abort the Transaction. */
                            LLD::TxnAbort();

                            return debug::error(FUNCTION, "failed to find longer ancestor block");
                        }
                    }

                    /* Break if found. */
                    if(fork == longer)
                        break;

                    /* Iterate backwards to find fork. */
                    vDisconnect.push_back(fork);

                    /* Iterate to previous block. */
                    fork = fork.Prev();
                    if(!fork)
                    {
                        /* Abort the Transaction. */
                        LLD::TxnAbort();

                        /* Debug errors. */
                        return debug::error(FUNCTION, "failed to find ancestor fork block");
                    }
                }

                /* Log if there are blocks to disconnect. */
                if(vDisconnect.size() > 0)
                {
                    debug::log(0, FUNCTION, "REORGANIZE: Disconnect ", vDisconnect.size(),
                        " blocks; ", fork.GetHash().ToString().substr(0,20),
                        "..",  ChainState::stateBest.load().GetHash().ToString().substr(0,20));

                    debug::log(0, FUNCTION, "REORGANIZE: Connect ", vConnect.size(), " blocks; ", fork.GetHash().ToString().substr(0,20),
                        "..", nHash.ToString().substr(0,20));
                }

                /* Disconnect given blocks. */
                for(auto& state : vDisconnect)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state.ToString(debug::flags::header | debug::flags::tx));

                    /* Connect the block. */
                    if(!state.Disconnect())
                    {
                        /* Abort the Transaction. */
                        LLD::TxnAbort();

                        /* Debug errors. */
                        return debug::error(FUNCTION, "failed to disconnect ",
                            state.GetHash().ToString().substr(0, 20));
                    }

                    /* Erase block if not connecting anything. */
                    if(vConnect.empty())
                    {
                        LLD::Ledger->EraseBlock(state.GetHash());
                        //LLD::Ledger->EraseIndex(state.nHeight);
                    }
                }

                /* Reverse the blocks to connect to connect in ascending height. */
                for(auto state = vConnect.rbegin(); state != vConnect.rend(); ++state)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state->ToString(debug::flags::header | debug::flags::tx));

                    /* Connect the block. */
                    if(!state->Connect())
                    {
                        /* Abort the Transaction. */
                        LLD::TxnAbort();

                        /* Debug errors. */
                        return debug::error(FUNCTION, "failed to connect ", state->GetHash().ToString().substr(0, 20));
                    }

                    /* Harden a checkpoint if there is any. */
                    HardenCheckpoint(Prev());
                }

                /* Set the best chain variables. */
                ChainState::hashBestChain      = nHash;
                ChainState::nBestChainTrust    = nChainTrust;
                ChainState::nBestHeight        = nHeight;

                /* Write the best chain pointer. */
                if(!LLD::Ledger->WriteBestChain(ChainState::hashBestChain.load()))
                    return debug::error(FUNCTION, "failed to write best chain");

                /* Debug output about the best chain. */
                debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION,
                    "New Best Block hash=", nHash.ToString().substr(0, 20),
                    " height=", ChainState::nBestHeight.load(),
                    " trust=", ChainState::nBestChainTrust.load(),
                    " tx=", vtx.size(),
                    " [", double(vtx.size()) / (GetBlockTime() - ChainState::stateBest.load().GetBlockTime()), " tx/s]"
                    " [verified in ", timer.ElapsedMilliseconds(), " ms]",
                    " [", ::GetSerializeSize(*this, SER_LLD, nVersion), " bytes]");

                /* Set best block state. */
                ChainState::stateBest = *this;

                /* Broadcast the block to nodes if not synchronizing. */
                if(!ChainState::Synchronizing())
                {
                    /* Block notify. */
                    std::string strCmd = config::GetArg("-blocknotify", "");
                    if(!strCmd.empty())
                    {
                        ReplaceAll(strCmd, "%s", ChainState::hashBestChain.load().GetHex());
                        std::thread t(runtime::command, strCmd);
                    }

                    /* Create the inventory object. */
                    bool fLegacy = TAO::Ledger::ChainState::stateBest.load().vtx[0].first == TAO::Ledger::TYPE::LEGACY_TX;

                    /* Relay the block that was just found. */
                    std::vector<LLP::CInv> vInv =
                    {
                        LLP::CInv(ChainState::hashBestChain.load(), fLegacy ? LLP::MSG_BLOCK_LEGACY : LLP::MSG_BLOCK_TRITIUM)
                    };

                    /* Relay the new block to all connected nodes. */
                    if(LLP::LEGACY_SERVER)
                        LLP::LEGACY_SERVER->Relay("inv", vInv);

                    /* If using Tritium server then we need to include the blocks transactions in the inventory before the block. */
                    if(LLP::TRITIUM_SERVER)
                        LLP::TRITIUM_SERVER->Relay(LLP::DAT_INVENTORY, vInv);
                }
            }

            return true;
        }


        /** Connect a block state into chain. **/
        bool BlockState::Connect()
        {
            /* Check through all the transactions. */
            for(const auto& proof : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(proof.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    /* Check the next hash pointer. */
                    if(tx.IsConfirmed())
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Connect the transaction. */
                    if(!tx.Connect())
                        return debug::error(FUNCTION, "failed to connect transaction");

                    /* Check for first. */
                    if(!tx.IsFirst())
                    {
                        /* Check for the last hash. */
                        uint512_t hashLast = 0;
                        if(!LLD::Ledger->ReadLast(tx.hashGenesis, hashLast))
                            return debug::error(FUNCTION, "failed to read last on non-genesis");

                        /* Check that the last transaction is correct. */
                        if(tx.hashPrevTx != hashLast)
                        {
                            /* Make sure the transaction is on disk. */
                            TAO::Ledger::Transaction tx2;
                            if(!LLD::Ledger->ReadTx(hashLast, tx2))
                                return debug::error(FUNCTION, "last transaction not on disk");

                            tx2.print();
                            tx.print();

                            for(uint32_t i = 0; i < 10; ++i)
                            {
                                if(!LLD::Ledger->ReadTx(tx.hashPrevTx, tx2))
                                    break;

                                tx2.print();
                                tx = tx2;
                            }

                            return debug::error(FUNCTION, "last hash hash mismatch");
                        }
                    }

                    /* Write the last to disk. */
                    if(!LLD::Ledger->WriteLast(tx.hashGenesis, hash))
                        return debug::error(FUNCTION, "failed to write last hash");
                }
                else if(proof.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Make sure the transaction isn't on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    /* Check for existing indexes. */
                    if(LLD::Ledger->HasIndex(hash))
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Fetch the inputs. */
                    std::map<uint512_t, Legacy::Transaction> inputs;
                    if(!tx.FetchInputs(inputs))
                        return debug::error(FUNCTION, "failed to fetch the inputs");

                    /* Connect the inputs. */
                    if(!tx.Connect(inputs, *this, Legacy::FLAGS::BLOCK))
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Add legacy transactions to the wallet where appropriate */
                    Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, *this, true);

                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");

                /* Write the indexing entries. */
                LLD::Ledger->IndexBlock(proof.second, GetHash());
            }

            /* Update the previous state's next pointer. */
            BlockState prev = Prev();

            /* Update the money supply. */
            nMoneySupply = (prev.IsNull() ? 0 : prev.nMoneySupply) + nMint;

            /* Log how much was generated / destroyed. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, nMint > 0 ? "Generated " : "Destroyed ", std::fixed, (double)nMint / TAO::Ledger::NXS_COIN, " Nexus | Money Supply ", std::fixed, (double)nMoneySupply / TAO::Ledger::NXS_COIN);

            /* Write the updated block state to disk. */
            if(!LLD::Ledger->WriteBlock(GetHash(), *this))
                return debug::error(FUNCTION, "failed to update block state");

            /* Index the block by height if enabled. */
            if(config::GetBoolArg("-indexheight"))
                LLD::Ledger->IndexBlock(nHeight, GetHash());

            /* Update chain pointer for previous block. */
            if(!prev.IsNull())
            {
                prev.hashNextBlock = GetHash();
                if(!LLD::Ledger->WriteBlock(prev.GetHash(), prev))
                    return debug::error(FUNCTION, "failed to update previous block state");
            }

            return true;
        }


        /** Disconnect a block state from the chain. **/
        bool BlockState::Disconnect()
        {
            /* Check through all the transactions. */
            for(const auto& proof : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(proof.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Read from disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Disconnect the transaction. */
                    if(!tx.Disconnect())
                        return debug::error(FUNCTION, "failed to disconnect transaction");
                }
                else if(proof.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = proof.second;

                    /* Read from disk. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Disconnect the inputs. */
                    if(!tx.Disconnect())
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Wallets need to refund inputs when disonnecting coinstake */
                    if(tx.IsCoinStake() && Legacy::Wallet::GetInstance().IsFromMe(tx))
                       Legacy::Wallet::GetInstance().DisableTransaction(tx);
                }

                /* Write the indexing entries. */
                LLD::Ledger->EraseIndex(proof.second);
            }

            /* Erase the index for block by height. */
            if(config::GetBoolArg("-indexheight"))
                LLD::Ledger->EraseIndex(nHeight);

            /* Update the previous state's next pointer. */
            BlockState prev = Prev();
            if(!prev.IsNull())
            {
                prev.hashNextBlock = 0;
                LLD::Ledger->WriteBlock(prev.GetHash(), prev);
            }

            return true;
        }


        /* USed to determine the trust of a block in the chain. */
        uint64_t BlockState::Trust() const
        {
            /** Give higher block trust if last block was of different channel **/
            BlockState prev = Prev();
            if(!prev.IsNull() && prev.GetChannel() != GetChannel())
                return 3;

            return 1;
        }


        /* Get the weight of this block. */
        uint64_t BlockState::Weight() const
        {
            /* Switch between the weights of the channels. */
            switch(nChannel)
            {
                /* Hash is the weight of the found hash. */
                case CHANNEL::HASH:
                {
                    /* Get the proof hash. */
                    uint1024_t hashProof = ProofHash();

                    /* Get the total weighting. */
                    uint64_t nWeight = ((~hashProof / (hashProof + 1)) + 1).Get64() / 10000;

                    return nWeight;
                }

                /* Proof of stake channel. */
                case CHANNEL::STAKE:
                {
                    /* Get the proof hash. */
                    uint1024_t hashProof = ProofHash();

                    /* Trust inforamtion variables. */
                    uint64_t nTrust = 0;
                    uint64_t nStake = 0;

                    /* Handle for version 7 blocks. */
                    if(nVersion >= 7)
                    {
                        /* Get the producer. */
                        Transaction tx;
                        if(!LLD::Ledger->ReadTx(vtx.back().second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read producer"));

                        /* Get trust information. */
                        if(!tx.GetTrustInfo(nTrust, nStake))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get trust info"));
                    }

                    /* Handle for version 5 blocks. */
                    else if(nVersion >= 5)
                    {
                        /* Get the coinstake. */
                        Legacy::Transaction tx;
                        if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read coinstake"));

                        /* Check for genesis. */
                        if(tx.IsTrust())
                        {
                            /* Temp variables */
                            uint1024_t hashLastBlock;
                            uint32_t   nSequence;
                            uint32_t   nTrustScore;

                            /* Extract trust from coinstake. */
                            if(!tx.ExtractTrust(hashLastBlock, nSequence, nTrustScore))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get trust info"));

                            /* Get the trust score. */
                            nTrust = uint64_t(nTrustScore);
                        }

                        /* Get the stake amount. */
                        nStake = uint64_t(tx.vout[0].nValue);
                    }
                    else
                    {
                        /* Get the coinstake. */
                        Legacy::Transaction tx;
                        if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                            throw std::runtime_error(debug::safe_printstr(FUNCTION, "could not read coinstake"));

                        /* Check for genesis. */
                        if(tx.IsTrust())
                        {

                            /* Extract the trust key from the coinstake. */
                            uint576_t cKey;
                            if(!tx.TrustKey(cKey))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "trust key not found in script"));

                            /* Get the trust key. */
                            Legacy::TrustKey trustKey;
                            if(!LLD::Trust->ReadTrustKey(cKey, trustKey))
                                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to read trust key"));

                            /* Get trust score. */
                            nTrust = trustKey.Age(TAO::Ledger::ChainState::stateBest.load().GetBlockTime());

                        }

                        /* Get the stake amount. */
                        nStake = uint64_t(tx.vout[0].nValue);
                    }

                    /* Get the total weighting. */
                    uint64_t nWeight = (((~hashProof / (hashProof + 1)) + 1).Get64() / 10000) * (nStake / NXS_COIN);

                    /* Include trust info in weighting. */
                    return nWeight + nTrust;
                }

                /* Prime (for now) is the weight of the prime difficulty. */
                case CHANNEL::PRIME:
                {
                    /* Check for offet patterns. */
                    if(vOffsets.empty())
                        return nBits * 25;

                    /* Get the prime difficulty. */
                    uint64_t nWeight = SetBits(GetPrimeDifficulty(GetPrime(), vOffsets)) * 25;

                    return nWeight;
                }
            }

            return 0;
        }


        /* Function to determine if this block has been connected into the main chain. */
        bool BlockState::IsInMainChain() const
        {
            return (hashNextBlock != 0 || GetHash() == ChainState::hashBestChain.load());
        }


        /* For debugging Purposes seeing block state data dump */
        std::string BlockState::ToString(uint8_t nState) const
        {
            std::string strDebug = "";

            /* Handle verbose output for just header. */
            if(nState & debug::flags::header)
            {
                strDebug += debug::safe_printstr("Block(",
                VALUE("hash") " = ", GetHash().ToString().substr(0, 20), ", ",
                VALUE("nVersion") " = ", nVersion, ", ",
                VALUE("hashPrevBlock") " = ", hashPrevBlock.ToString().substr(0, 20), ", ",
                VALUE("hashMerkleRoot") " = ", hashMerkleRoot.ToString().substr(0, 20), ", ",
                VALUE("nChannel") " = ", nChannel, ", ",
                VALUE("nHeight") " = ", nHeight, ", ",
                VALUE("nDiff") " = ", GetDifficulty(nBits, nChannel), ", ",
                VALUE("nNonce") " = ", nNonce, ", ",
                VALUE("nTime") " = ", nTime, ", ",
                VALUE("blockSig") " = ", HexStr(vchBlockSig.begin(), vchBlockSig.end()));
            }

            /* Handle the verbose output for chain state. */
            if(nState & debug::flags::chain)
            {
                strDebug += debug::safe_printstr(", ",
                VALUE("nChainTrust") " = ", nChainTrust, ", ",
                VALUE("nMoneySupply") " = ", nMoneySupply, ", ",
                VALUE("nChannelHeight") " = ", nChannelHeight, ", ",
                VALUE("nMinerReserve") " = ", nReleasedReserve[0], ", ",
                VALUE("nAmbassadorReserve") " = ", nReleasedReserve[1], ", ",
                VALUE("nDeveloperReserve") " = ", nReleasedReserve[2], ", ",
                VALUE("hashNextBlock") " = ", hashNextBlock.ToString().substr(0, 20), ", ",
                VALUE("hashCheckpoint") " = ", hashCheckpoint.ToString().substr(0, 20));
            }

            strDebug += ")";

            /* Handle the verbose output for transactions. */
            if(nState & debug::flags::tx)
            {
                for(const auto& tx : vtx)
                    strDebug += debug::safe_printstr("\nProof(nType = ", (uint32_t)tx.first, ", hash = ", tx.second.ToString().substr(0, 20), ")");
            }

            return strDebug;
        }


        /* For debugging purposes, printing the block to stdout */
        void BlockState::print() const
        {
            debug::log(0, ToString(debug::flags::header | debug::flags::chain));
        }

        uint1024_t BlockState::StakeHash() const
        {
            if(vtx[0].first == TYPE::TRITIUM_TX)
            {
                /* Get the tritium transaction  from the database*/
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(vtx[0].second, tx))
                    return debug::error(FUNCTION, "transaction is not on disk");

                return Block::StakeHash( tx.IsGenesis(), tx.hashGenesis);
            }
            else if(vtx[0].first == TYPE::LEGACY_TX)
            {
                /* Get the legacy transaction from the database. */
                Legacy::Transaction tx;
                if(!LLD::Legacy->ReadTx(vtx[0].second, tx))
                    return debug::error(FUNCTION, "transaction is not on disk");

                /* Get the trust key. */
                uint576_t keyTrust;
                tx.TrustKey(keyTrust);

                return Block::StakeHash(tx.IsGenesis(), keyTrust);
            }
            else
                return debug::error(FUNCTION, "StakeHash called on invalid BlockState");
        }
    }
}
