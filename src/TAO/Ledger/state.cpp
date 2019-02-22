/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <string>

#include <LLD/include/global.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/supply.h>

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
            uint1024_t genesisHash =  ChainState::Genesis();

            /* Loop back 10k blocks. */
            for(uint_t i = 0; i < 1440; ++i)
            {
                /* Return false on genesis. */
                if(state.GetHash() == genesisHash)
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
        , vtx()
        , nChainTrust(0)
        , nMoneySupply(0)
        , nMint(0)
        , nChannelHeight(0)
        , nReleasedReserve{0, 0, 0}
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
            vtx.push_back(std::make_pair(TYPE::TRITIUM_TX, block.producer.GetHash()));
            vtx.insert(vtx.end(), block.vtx.begin(), block.vtx.end());

            assert(vtx.size() == block.vtx.size()); //TODO: maybe a softer way to verify?
        }


        /* Construct a block state from a legacy block. */
        BlockState::BlockState(const Legacy::LegacyBlock& block)
        : Block(block)
        , vtx()
        , nChainTrust(0)
        , nMoneySupply(0)
        , nMint(0)
        , nChannelHeight(0)
        , nReleasedReserve{0, 0, 0}
        , hashNextBlock(0)
        , hashCheckpoint(0)
        {
            for(const auto& tx : block.vtx)
                vtx.push_back(std::make_pair(TYPE::LEGACY_TX, tx.GetHash()));

            assert(vtx.size() == block.vtx.size()); //TODO: maybe a softer way to verify?
        }


        /* Get the previous block state in chain. */
        BlockState BlockState::Prev() const
        {
            BlockState state;
            if(hashPrevBlock == 0)
                return state;

            if(LLD::legDB->ReadBlock(hashPrevBlock, state))
                return state;
            else
                debug::error("failed to read previous block state ", hashPrevBlock.ToString());

            return state;
        }


        /* Get the next block state in chain. */
        BlockState BlockState::Next() const
        {
            BlockState state;
            if(hashNextBlock == 0)
                return state;

            if(LLD::legDB->ReadBlock(hashNextBlock, state))
                return state;

            return state;
        }

        /* Accept a block state into chain. */
        bool BlockState::Index()
        {
            /* Read leger DB for duplicate block. */
            if(LLD::legDB->HasBlock(GetHash()))
                return debug::error(FUNCTION, "already have block ", GetHash().ToString().substr(0, 20), " height ", nHeight);

            /* Read leger DB for previous block. */
            BlockState statePrev = Prev();
            if(!statePrev)
                return debug::error(FUNCTION, "previous block state not found");

            /* Compute the Chain Trust */
            nChainTrust = statePrev.nChainTrust + GetBlockTrust();

            /* Compute the Channel Height. */
            BlockState stateLast = statePrev;
            GetLastState(stateLast, GetChannel());
            nChannelHeight = stateLast.nChannelHeight + 1;

            /* Compute the Released Reserves. */
            if(IsProofOfWork())
            {
                /* Calculate the coinbase rewards from the coinbase transaction. */
                uint64_t nCoinbaseRewards[3] = {0, 0, 0};
                if(vtx[0].first == TYPE::LEGACY_TX)
                {
                    /* Get the coinbase from the memory pool. */
                    Legacy::Transaction tx;
                    if(!mempool.Get(vtx[0].second, tx))
                        return debug::error(FUNCTION, "coinbase is not in the memory pool");

                    /* Get the size of the coinbase. */
                    uint32_t nSize = tx.vout.size();

                    /* Add up the Miner Rewards from Coinbase Tx Outputs. */
                    for(int32_t nIndex = 0; nIndex < nSize - 2; nIndex++)
                        nCoinbaseRewards[0] += tx.vout[nIndex].nValue;

                    /* Get the ambassador and developer coinbase. */
                    nCoinbaseRewards[1] = tx.vout[nSize - 2].nValue;
                    nCoinbaseRewards[2] = tx.vout[nSize - 1].nValue;
                }
                else
                {
                    //TODO: handle for the coinbase in Tritium
                }

                for(int nType = 0; nType < 3; nType++)
                {
                    /* Calculate the Reserves from the Previous Block in Channel's reserve and new Release. */
                    uint64_t nReserve  = stateLast.nReleasedReserve[nType] +
                        GetReleasedReserve(*this, GetChannel(), nType);

                    /* Block Version 3 Check. Disable Reserves from going below 0. */
                    if(nVersion != 2 && nCoinbaseRewards[nType] >= nReserve)
                        return debug::error(FUNCTION, "out of reserve limits");

                    /* Check coinbase rewards. */
                    nReleasedReserve[nType] =  (nReserve - nCoinbaseRewards[nType]);

                    debug::log(2, "Reserve Balance ", nType, " | ", std::fixed, nReleasedReserve[nType] / 1000000.0,
                        " Nexus | Released ", std::fixed, (nReserve - stateLast.nReleasedReserve[nType]) / 1000000.0);
                }
            }

            /* Add the Pending Checkpoint into the Blockchain. */
            if(IsNewTimespan(statePrev))
            {
                hashCheckpoint = GetHash();

                debug::log(1, "===== New Pending Checkpoint Hash = ", hashCheckpoint.ToString().substr(0, 15));
            }
            else
            {
                hashCheckpoint = statePrev.hashCheckpoint;

                debug::log(1, "===== Pending Checkpoint Hash = ", hashCheckpoint.ToString().substr(0, 15));
            }


            /* Start the database transaction. */
            LLD::TxnBegin();


            /* Write the block to disk. */
            if(!LLD::legDB->WriteBlock(GetHash(), *this))
                return debug::error(FUNCTION, "block state failed to write");


            /* Write the transactions. */
            for(const auto& tx : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(tx.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!mempool.Get(hash, tx))
                        return debug::error(FUNCTION, "transaction is not in memory pool"); //TODO: recover from missing transactions

                    /* Write to disk. */
                    if(!LLD::legDB->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                }
                else if(tx.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Check if in memory pool. */
                    Legacy::Transaction tx;
                    if(!mempool.Get(hash, tx))
                        return debug::error(FUNCTION, "transaction is not in memory pool");  //TODO: recover from missing transactions

                    /* Write to disk. */
                    if(!LLD::legacyDB->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                    debug::log(0, FUNCTION, "wrote tx ", hash.ToString().substr(0, 20), " to disk");

                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");
            }


            /* Signal to set the best chain. */
            if(nChainTrust > ChainState::nBestChainTrust.load())
                if(!SetBest())
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
            runtime::timer time;
            time.Start();

            /* Watch for genesis. */
            if (!ChainState::stateGenesis)
            {
                /* Write the best chain pointer. */
                if(!LLD::legDB->WriteBestChain(GetHash()))
                    return debug::error(FUNCTION, "failed to write best chain");

                /* Write the block to disk. */
                if(!LLD::legDB->WriteBlock(GetHash(), *this))
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
                while (fork != longer)
                {
                    /* Find the root block in common. */
                    while (longer.nHeight > fork.nHeight)
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
                    if (fork == longer)
                        break;

                    /* Iterate backwards to find fork. */
                    vDisconnect.push_back(fork);
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
                        "..", this->GetHash().ToString().substr(0,20));
                }

                /* List of transactions to resurrect. */
                std::vector<std::pair<uint8_t, uint512_t>> vResurrect;

                /* Disconnect given blocks. */
                std::vector<TAO::Ledger::Transaction> vTritiumResurrect;
                std::vector<Legacy::Transaction> vLegacyResurrect;
                for(auto& state : vDisconnect)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state.ToString(debug::flags::header | debug::flags::tx));

                    /* Add transactions into memory pool. */
                    if(!vConnect.empty())
                    {
                        for(const auto& txAdd : state.vtx)
                        {
                            if(txAdd.first == TYPE::TRITIUM_TX)
                            {
                                /* Check if in memory pool. */
                                TAO::Ledger::Transaction tx;
                                if(!LLD::legDB->ReadTx(txAdd.second, tx))
                                    return debug::error(FUNCTION, "transaction is not on disk");

                                /* Resurrect. */
                                if(!tx.IsCoinbase() && !tx.IsTrust())
                                    vTritiumResurrect.push_back(tx);
                            }
                            else if(txAdd.first == TYPE::LEGACY_TX)
                            {
                                /* Check if in memory pool. */
                                Legacy::Transaction tx;
                                if(!LLD::legacyDB->ReadTx(txAdd.second, tx))
                                    return debug::error(FUNCTION, "transaction is not on disk");

                                /* Resurrect */
                                if(!tx.IsCoinBase() && !tx.IsCoinStake())
                                    vLegacyResurrect.push_back(tx);
                            }
                        }
                    }

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
                        LLD::legDB->EraseBlock(state.GetHash());
                        //LLD::legDB->EraseIndex(state.nHeight);
                    }
                }


                /* Only add transactions to memory pool if there are blocks to connect. */
                if(!vConnect.empty())
                {
                    /* Resurrect the tritium transactions. */
                    for(const auto& tx : vTritiumResurrect)
                        mempool.Accept(tx);

                    /* Resurrect the legacy transactions. */
                    for(const auto& tx : vLegacyResurrect)
                        mempool.Accept(tx);
                }


                /* List of transactions to remove from pool. */
                std::vector<uint512_t> vDelete;


                /* Reverse the blocks to connect to connect in ascending height. */
                std::reverse(vConnect.begin(), vConnect.end());
                for(auto& state : vConnect)
                {
                    /* Output the block state if flagged. */
                    if(config::GetBoolArg("-printstate"))
                        debug::log(0, state.ToString(debug::flags::header | debug::flags::tx));

                    /* Connect the block. */
                    if(!state.Connect())
                    {
                        /* Abort the Transaction. */
                        LLD::TxnAbort();

                        /* Debug errors. */
                        return debug::error(FUNCTION, "failed to connect ",
                            state.GetHash().ToString().substr(0, 20));
                    }

                    /* Remove transactions from memory pool. */
                    for(const auto& tx : state.vtx)
                        vDelete.push_back(tx.second);

                    /* Harden a checkpoint if there is any. */
                    HardenCheckpoint(Prev());
                }


                /* Remove transactions from memory pool. */
                for(const auto& hashTx : vDelete)
                    mempool.Remove(hashTx);


                /* Set the best chain variables. */
                ChainState::stateBest          = *this;
                ChainState::hashBestChain      = GetHash();
                ChainState::nBestChainTrust    = nChainTrust;
                ChainState::nBestHeight        = nHeight;


                /* Write the best chain pointer. */
                if(!LLD::legDB->WriteBestChain(ChainState::hashBestChain.load()))
                    return debug::error(FUNCTION, "failed to write best chain");


                /* Debug output about the best chain. */
                debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION,
                    "New Best Block hash=", GetHash().ToString().substr(0, 20),
                    " height=", ChainState::nBestHeight.load(),
                    " trust=", ChainState::nBestChainTrust.load(),
                    " [verified in ", time.ElapsedMilliseconds(), " ms]",
                    " [", ::GetSerializeSize(*this, SER_LLD, nVersion), " bytes]");



                /* Broadcast the block to nodes if not synchronizing. */
                if(!ChainState::Synchronizing())
                {
                    /* Block notify. */
                    std::string strCmd = config::GetArg("-blocknotify", "");
                    if (!strCmd.empty())
                    {
                        replace_all(strCmd, "%s", ChainState::hashBestChain.load().GetHex());
                        std::thread t(runtime::command, strCmd);
                    }

                    /* Create the inventory object. */
                    std::vector<LLP::CInv> vInv = { LLP::CInv(ChainState::hashBestChain.load(), LLP::MSG_BLOCK) };

                    /* Relay the new block to all connected nodes. */
                    if(LLP::LEGACY_SERVER)
                        LLP::LEGACY_SERVER->Relay("inv", vInv);
                }
            }

            return true;
        }


        /** Connect a block state into chain. **/
        bool BlockState::Connect()
        {

            /* Check through all the transactions. */
            for(const auto& tx : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(tx.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Make sure the transaction is on disk. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::legDB->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    /* Check for existing indexes. */
                    if(LLD::legDB->HasIndex(hash))
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Verify the ledger layer. */
                    if(!TAO::Register::Verify(tx))
                        return debug::error(FUNCTION, "transaction register layer failed to verify");

                    /* Execute the operations layers. */
                    if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::WRITE))
                        return debug::error(FUNCTION, "transaction operation layer failed to execute");

                    /* Write to disk. */
                    if(!LLD::legDB->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                    /* Check for genesis. */
                    if(tx.IsGenesis())
                    {
                        //Check for duplicate genesis

                        /* Write the Genesis to disk. */
                        if(!LLD::legDB->WriteGenesis(tx.hashGenesis, tx.GetHash()))
                            return debug::error(FUNCTION, "failed to write genesis");

                        /* Write the last to disk. */
                        if(!LLD::legDB->WriteLast(tx.hashGenesis, tx.GetHash()))
                            return debug::error(FUNCTION, "failed to write last hash");
                    }
                    else
                    {
                        /* Check for the last hash. */
                        uint512_t hashLast;
                        if(!LLD::legDB->ReadLast(tx.hashGenesis, hashLast))
                            return debug::error(FUNCTION, "failed to read last on non-genesis");

                        /* Check that advertised last transaction is correct. */
                        if(tx.hashPrevTx != hashLast)
                            return debug::error(FUNCTION,
                                "previous transaction ", tx.hashPrevTx.ToString().substr(0, 20),
                                " and last hash mismatch ", hashLast.ToString().substr(0, 20));

                        /* Write the last to disk. */
                        if(!LLD::legDB->WriteLast(tx.hashGenesis, tx.GetHash()))
                            return debug::error(FUNCTION, "failed to write last hash");
                    }
                }
                else if(tx.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Make sure the transaction isn't on disk. */
                    Legacy::Transaction tx;
                    if(!LLD::legacyDB->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction not on disk");

                    /* Check for existing indexes. */
                    if(LLD::legDB->HasIndex(hash))
                        return debug::error(FUNCTION, "transaction overwrites not allowed");

                    /* Check for coinbase or coinstake. */
                    if(hash == vtx[0].second && !tx.IsCoinBase() && !tx.IsCoinStake())
                        return debug::error(FUNCTION, "first transction not coinbase/coinstake");

                    /* Fetch the inputs. */
                    std::map<uint512_t, Legacy::Transaction> inputs;
                    if(!tx.FetchInputs(inputs))
                        return debug::error(FUNCTION, "failed to fetch the inputs");

                    /* Write to disk. */
                    if(!LLD::legacyDB->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");

                    /* Connect the inputs. */
                    if(!tx.Connect(inputs, *this, Legacy::FLAGS::BLOCK))
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Add legacy transactions to the wallet where appropriate */
                    Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, *this, true);

                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");

                /* Write the indexing entries. */
                LLD::legDB->IndexBlock(tx.second, GetHash());
            }

            /* Update the previous state's next pointer. */
            BlockState prev = Prev();

            /* Update the money supply. */
            nMoneySupply = (prev.IsNull() ? 0 : prev.nMoneySupply) + nMint;

            /* Log how much was generated / destroyed. */
            debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, nMint > 0 ? "Generated " : "Destroyed ", std::fixed, (double)nMint / Legacy::COIN, " Nexus | Money Supply ", std::fixed, (double)nMoneySupply / Legacy::COIN);

            /* Write the updated block state to disk. */
            if(!LLD::legDB->WriteBlock(GetHash(), *this))
                return debug::error(FUNCTION, "failed to update block state");

            /* Index the block by height if enabled. */
            if(config::GetBoolArg("-indexheight"))
                LLD::legDB->IndexBlock(nHeight, GetHash());

            /* Update chain pointer for previous block. */
            if(!prev.IsNull())
            {
                prev.hashNextBlock = GetHash();
                if(!LLD::legDB->WriteBlock(prev.GetHash(), prev))
                    return debug::error(FUNCTION, "failed to update previous block state");
            }

            return true;
        }


        /** Disconnect a block state from the chain. **/
        bool BlockState::Disconnect()
        {
            /* Check through all the transactions. */
            for(const auto& tx : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(tx.first == TYPE::TRITIUM_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Check if in memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::legDB->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Rollback the register layer. */
                    if(!TAO::Register::Rollback(tx))
                        return debug::error(FUNCTION, "transaction register layer failed to rollback");

                    /* Delete the transaction. */
                    if(!LLD::legDB->EraseTx(hash))
                        return debug::error(FUNCTION, "could not erase transaction");
                }
                else if(tx.first == TYPE::LEGACY_TX)
                {
                    /* Get the transaction hash. */
                    uint512_t hash = tx.second;

                    /* Check if in memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::legacyDB->ReadTx(hash, tx))
                        return debug::error(FUNCTION, "transaction is not on disk");

                    /* Disconnect the inputs. */
                    if(!tx.Disconnect())
                        return debug::error(FUNCTION, "failed to connect inputs");

                    /* Wallets need to refund inputs when disonnecting coinstake */
                    if (tx.IsCoinStake() && Legacy::Wallet::GetInstance().IsFromMe(tx))
                       Legacy::Wallet::GetInstance().DisableTransaction(tx);
                }

                /* Write the indexing entries. */
                LLD::legDB->EraseIndex(tx.second);
            }

            /* Erase the index for block by height. */
            if(config::GetBoolArg("-indexheight"))
                LLD::legDB->EraseIndex(nHeight);

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
                if(!LLD::legDB->ReadTx(vtx[0].second, tx))
                    return debug::error(FUNCTION, "transaction is not on disk");

                return Block::StakeHash( tx.IsGenesis(), tx.hashGenesis);
            }
            else if(vtx[0].first == TYPE::LEGACY_TX)
            {
                /* Get the legacy transaction from the database. */
                Legacy::Transaction tx;
                if(!LLD::legacyDB->ReadTx(vtx[0].second, tx))
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
