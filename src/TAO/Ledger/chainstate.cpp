/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/timelocks.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* The best block height in the chain. */
        std::atomic<uint32_t> ChainState::nBestHeight;


        /* The best trust in the chain. */
        std::atomic<uint64_t> ChainState::nBestChainTrust;


        /* The best hash in the chain. */
        memory::atomic<uint1024_t> ChainState::hashBestChain;


        /* Hardened Checkpoint. */
        memory::atomic<uint1024_t> ChainState::hashCheckpoint;


        /* The best block in the chain. */
        memory::atomic<BlockState> ChainState::stateBest;


        /* The best block in the chain. */
        BlockState ChainState::stateGenesis;


        /* Flag to tell if initial blocks are downloading. */
        bool ChainState::Synchronizing()
        {
            /* Check for null best state. */
            if(stateBest.load().IsNull())
                return true;

            /* Check if there's been a new block. */
            static memory::atomic<uint1024_t> hashLast;
            static std::atomic<uint32_t> nLastTime;
            if(hashBestChain.load() != hashLast.load())
            {
                hashLast = hashBestChain.load();
                nLastTime = static_cast<uint32_t>(runtime::unifiedtimestamp());
            }


            /* Special testnet rule. */
            if(config::fTestNet.load())
                return (stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60) && (runtime::unifiedtimestamp() - nLastTime < 30);

            /* Check if block has been created within 20 minutes. */
            return (stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60);
        }

        /* Flag to tell if initial blocks are downloading. */
        double ChainState::PercentSynchronized()
        {
            uint32_t nChainAge = (static_cast<uint32_t>(runtime::unifiedtimestamp()) - 20 * 60) - (config::fTestNet.load() ? TAO::Ledger::NEXUS_TESTNET_TIMELOCK : TAO::Ledger::NEXUS_NETWORK_TIMELOCK);
            uint32_t nSyncAge  = static_cast<uint32_t>(stateBest.load().GetBlockTime() - static_cast<uint64_t>(config::fTestNet.load() ? TAO::Ledger::NEXUS_TESTNET_TIMELOCK : TAO::Ledger::NEXUS_NETWORK_TIMELOCK));

            return (100.0 * nSyncAge) / nChainAge;
        }


        /* Initialize the Chain State. */
        bool ChainState::Initialize()
        {
            /* Initialize the Genesis. */
            if(!CreateGenesis())
                return debug::error(FUNCTION, "failed to create genesis");

            /* Read the best chain. */
            if(!LLD::Ledger->ReadBestChain(hashBestChain))
                return debug::error(FUNCTION, "failed to read best chain");

            /* Get the best chain stats. */
            if(!LLD::Ledger->ReadBlock(hashBestChain.load(), stateBest))
            {
                debug::error(FUNCTION, "failed to read best block, attempting to recover database");

                /* The block for hashBestChain might not exist on disk if the process was unexpectedly terminated
                during a block commit.  In this case we can attempt to recover by iterating forward from the genesis
                blockState until we reach the end of the chain, which is the last written block. */
                BlockState stateBestKnown = stateGenesis;
                while(!stateBestKnown.IsNull() && stateBestKnown.hashNextBlock != 0)
                {
                    stateBest = stateBestKnown;
                    stateBestKnown = stateBestKnown.Next();
                }

                hashBestChain = stateBest.load().GetHash();
                if(!LLD::Ledger->WriteBestChain(hashBestChain.load()))
                    return debug::error(FUNCTION, "failed to write best chain");

                debug::log(0, FUNCTION, "database successfully recovered");
            }

            /* Rewind the chain a total number of blocks. */
            if(config::GetArg("-forkblocks", 0) > 0)
            {
                /* Rollback the chain a given number of blocks. */
                TAO::Ledger::BlockState state = stateBest.load();
                for(int i = 0; i < config::GetArg("-forkblocks", 0); ++i)
                    state = state.Prev();

                /* Set the best to older block. */
                LLD::TxnBegin();
                state.SetBest();
                LLD::TxnCommit();
            }

            /* Check blocks and check transactions for consistency. */
            if(config::GetArg("-checkblocks", 0) > 0)
            {
                debug::log(0, FUNCTION, "Checking from height=", stateBest.load().nHeight, " hash=", stateBest.load().GetHash().SubString());

                /* Rollback the chain a given number of blocks. */
                TAO::Ledger::BlockState state = stateBest.load();
                Legacy::Transaction tx;

                TAO::Ledger::BlockState stateReset = stateBest.load();
                for(uint32_t i = 0; i < config::GetArg("-checkblocks", 0) && !config::fShutdown.load(); ++i)
                {
                    if(state == stateGenesis)
                        break;

                    /* Scan each transaction in the block and process those related to this wallet */
                    std::vector<uint512_t> vHashes;
                    for(const auto& item : state.vtx)
                    {
                        vHashes.push_back(item.second);
                        if(item.first == TAO::Ledger::LEGACY_TX)
                        {
                            /* Read transaction from database */
                            if(!LLD::Legacy->ReadTx(item.second, tx))
                            {
                                debug::log(0, state.ToString(debug::flags::tx | debug::flags::header));

                                debug::error(FUNCTION, "tx ", item.second.SubString(), " not found");

                                stateReset = state;
                                continue;
                            }

                            /* Check for coinbase or coinstake. */
                            if(item.second == state.vtx[0].second && !tx.IsCoinBase() && !tx.IsCoinStake())
                            {
                                debug::error(FUNCTION, "first transction not coinbase/coinstake");

                                stateReset = state;
                                continue;
                            }
                        }
                    }

                    /* Check the merkle root. */
                    if(state.hashMerkleRoot != state.BuildMerkleTree(vHashes))
                    {
                        debug::log(0, state.ToString(debug::flags::tx | debug::flags::header));

                        debug::error(FUNCTION, "merkle tree mismatch");

                        stateReset = state;
                    }

                    /* Iterate backwards. */
                    state = state.Prev();
                    if(!state)
                        break;

                    /* Debug Output. */
                    if(i % 100000 == 0)
                        debug::log(0, "Checked ", i, " Blocks...");
                }

                if(stateBest != stateReset)
                {
                    /* Set the best to older block. */
                    LLD::TxnBegin();
                    stateReset.SetBest();
                    LLD::TxnCommit();
                }
            }

            /* Fill out the best chain stats. */
            nBestHeight     = stateBest.load().nHeight;
            nBestChainTrust = stateBest.load().nChainTrust;

            /* Set the checkpoint. */
            hashCheckpoint = stateBest.load().hashCheckpoint;

            /* Find the last checkpoint. */
            if(stateBest != stateGenesis)
            {
                /* Search back until fail or different checkpoint. */
                BlockState state;
                if(!LLD::Ledger->ReadBlock(hashCheckpoint.load(), state))
                    return debug::error(FUNCTION, "no pending checkpoint");

                /* Get the previous state. */
                state = state.Prev();
                if(!state)
                    return debug::error(FUNCTION, "failed to find the checkpoint");

                /* Set the checkpoint. */
                hashCheckpoint = state.hashCheckpoint;
            }

            /* Ensure the block height index is intact */
            if(config::GetBoolArg("-indexheight"))
            {
                /* Try and retrieve the block state for the current block height via the height index.
                    If this fails then we know the block height index is not fully intact so we repair it*/
                TAO::Ledger::BlockState state;
                if(!LLD::Ledger->ReadBlock(TAO::Ledger::ChainState::stateBest.load().nHeight, state))
                     LLD::Ledger->RepairIndexHeight();
            }


            /* Debug logging. */
            debug::log(0, FUNCTION, config::fTestNet.load() ? "Test" : "Nexus", " Network: genesis=", Genesis().SubString(),
            " nBitsStart=0x", std::hex, bnProofOfWorkStart[0].GetCompact(), " best=", hashBestChain.load().SubString(),
            " checkpoint=", hashCheckpoint.load().SubString()," height=", std::dec, stateBest.load().nHeight);

            return true;
        }


        /* Get the hash of the genesis block. */
        uint1024_t ChainState::Genesis()
        {
            return config::fTestNet.load() ? TAO::Ledger::hashGenesisTestnet : TAO::Ledger::hashGenesis;
        }
    }
}
