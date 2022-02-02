/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
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


        /* The current checkpoint height. */
        std::atomic<uint64_t> ChainState::nCheckpointHeight;


        /* The best hash in the chain. */
        memory::atomic<uint1024_t> ChainState::hashBestChain;


        /* Hardened Checkpoint. */
        memory::atomic<uint1024_t> ChainState::hashCheckpoint;


        /* The best block in the chain. */
        memory::atomic<BlockState> ChainState::stateBest;


        /* The best block in the chain. */
        BlockState ChainState::stateGenesis;


        /* Flag to tell if initial blocks are downloading. */
        //static std::atomic<bool> fSynchronizing(true);
        bool ChainState::Synchronizing()
        {
            /* Static values to check synchronization status. */
            static memory::atomic<uint1024_t> hashLast;
            static std::atomic<uint64_t> nLastTime;

            bool fSynchronizing = true;
            if(!config::GetBoolArg("-sync", true)) //hard value to rely on if needed
                return false;

            /* Persistent switch once synchronized. */
            //if(!fSynchronizing.load())
            //    return false;

            #ifndef UNIT_TESTS

            /* Check for null best state. */
            if(stateBest.load().IsNull())
                return true;

            /* Check if there's been a new block from internal static values. */
            if(hashBestChain.load() != hashLast.load())
            {
                hashLast = hashBestChain.load();
                nLastTime = runtime::unifiedtimestamp();
            }

            /* Special testnet rules*/
            if(config::fTestNet.load())
            {
                /* Check for specific conditions such as local testnet */
                const bool fLocalTestnet   =
                    (config::fTestNet.load() && (!config::GetBoolArg("-dns", true) || config::fHybrid.load()));

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return false;

                /* Check for active connections. */
                bool fHasConnections =
                    (LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->GetConnectionCount() > 0);

                /* Set the synchronizing flag. */
                fSynchronizing =
                (
                    /* If using main testnet then rely on the LLP synchronized flag */
                    (!fLocalTestnet && !LLP::TritiumNode::fSynchronized.load()
                        && stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60)

                    /* If local testnet with connections then rely on LLP flag  */
                    || (fLocalTestnet && fHasConnections && !LLP::TritiumNode::fSynchronized.load())

                    /* If local testnet with no connections then assume sync'd if the last block was more than 30s ago
                       and block age is more than 20 mins, which gives us a 30s window to connect to a local peer */
                    || (fLocalTestnet && !fHasConnections
                        && runtime::unifiedtimestamp() - nLastTime < 30
                        && stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60)
                );

                return fSynchronizing;
            }

            /* Check if block has been created within 60 minutes. */
            fSynchronizing =
            (
                (!LLP::TritiumNode::fSynchronized.load() &&
                (stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 60 * 60))
            );

            return fSynchronizing;

            /* On unit tests, always keep Synchronizing off. */
            #else
            _unused(fSynchronizing); //suppress compiler warnings
            return false;
            #endif
        }


        /* Real value of the total synchronization percent completion. */
        double ChainState::PercentSynchronized()
        {
            /* The timstamp of the genesis block.   */
            uint32_t nGenesis = stateGenesis.GetBlockTime();

            /* Find the chain age relative to the genesis */

            /* Calculate the time between the last block received and now.  NOTE we calculate this to one minute in the past,
                to accommodate for the average block time of 50s */
            uint32_t nChainAge = (static_cast<uint32_t>(runtime::unifiedtimestamp()) - (60 * 20) - nGenesis);
            uint32_t nSyncAge  = static_cast<uint32_t>(stateBest.load().GetBlockTime() - nGenesis);

            /* Ensure that the chain age is not less than the sync age, which would happen if the
                last block time was within the last minute */
            nChainAge = std::max(nChainAge, nSyncAge);

            /* Calculate the sync percent. */
            return std::min(100.0, (100.0 * nSyncAge) / nChainAge);

        }


        /* Percentage of blocks synchronized since the node started. */
        double ChainState::SyncProgress()
        {
            /* Catch if we aren't syncing yet. */
            if(LLP::TritiumNode::nSyncStop.load() == 0)
                return 0.0;

            /* Total blocks synchronized */
            const uint32_t nBlocks = stateBest.load().nHeight - LLP::TritiumNode::nSyncStart.load();
            const uint32_t nTotals = LLP::TritiumNode::nSyncStop.load() - LLP::TritiumNode::nSyncStart.load();

            /* Calculate the sync percent. */
            return std::min(100.0, (100.0 * nBlocks) / nTotals);
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

                /* If hashBestChain exists, but block doesn't attempt to recover database from invalid write.  */
                BlockState stateBestKnown = stateGenesis;
                while(!stateBestKnown.IsNull() && stateBestKnown.hashNextBlock != 0)
                {
                    stateBest = stateBestKnown;
                    stateBestKnown = stateBestKnown.Next();
                }

                /* Once new best chain is found, write it to disk. */
                hashBestChain = stateBest.load().GetHash();
                if(!LLD::Ledger->WriteBestChain(hashBestChain.load()))
                    return debug::error(FUNCTION, "failed to write best chain");

                debug::log(0, FUNCTION, "database successfully recovered");
            }

            /* Check database consistency. */
            if(stateBest.load().GetHash() != hashBestChain.load())
                return debug::error(FUNCTION, "disk index inconsistent with best chain");

            /* Reverse iterator to find the most recent common ancestor. */
            if(!config::fHybrid.load())
            {
                BlockState stateFork;
                for(auto it = mapCheckpoints.rbegin(); it != mapCheckpoints.rend(); ++it)
                {
                    /* Check that we are within correct height ranges. */
                    if(it->first > stateBest.load().nHeight)
                        continue;

                    /* Load the block from disk. */
                    BlockState stateCheck;
                    if(!LLD::Ledger->ReadBlock(it->second, stateCheck))
                    {
                        /* Find nearest ancestory block. */
                        auto iAncestor = it;
                        iAncestor++;

                        /* Find the most common ancestor. */
                        BlockState stateAncestor;
                        if(LLD::Ledger->ReadBlock(iAncestor->second, stateAncestor))
                        {
                            /* Debug output if ancestor was found. */
                            debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "WARNING: ", ANSI_COLOR_RESET,
                                " REVERTING TO HARDCODED Ancestor ", iAncestor->first, " Hash ", iAncestor->second.SubString());

                            /* Set the best to older block. */
                            LLD::TxnBegin();
                            stateAncestor.SetBest();
                            LLD::TxnCommit();

                            break;
                        }
                    }
                }
            }

            /* Rewind the chain a total number of blocks. */
            int64_t nForkblocks = config::GetArg("-forkblocks", 0);
            if(nForkblocks > 0)
            {
                /* Rollback the chain a given number of blocks. */
                TAO::Ledger::BlockState state = stateBest.load();
                for(int i = 0; i < nForkblocks; ++i)
                {
                    /* Check for Genesis. */
                    if(state.hashPrevBlock == 0)
                        break;

                    /* Iterate backwards the total number of blocks requested. */
                    state = state.Prev();
                    if(!state)
                        return debug::error(FUNCTION, "failed to find ancestor block");
                }

                /* Set the best to older block. */
                LLD::TxnBegin();
                state.SetBest();
                LLD::TxnCommit();

                /* Debug Output. */
                debug::log(0, FUNCTION, "-forkblocks=XXX requested removal of ", nForkblocks, " blocks");
            }

            /* Fill out the best chain stats. */
            nBestHeight     = stateBest.load().nHeight;
            nBestChainTrust = stateBest.load().nChainTrust;

            /* Set the checkpoint. */
            hashCheckpoint = stateBest.load().hashCheckpoint;

            /* Find the last checkpoint. */
            if(stateBest != stateGenesis)
            {
                /* Go back 10 checkpoints on startup. */
                for(uint32_t i = 0; i < config::GetArg("-checkpoints", 100); ++i)
                {
                    /* Search back until fail or different checkpoint. */
                    BlockState state;
                    if(!LLD::Ledger->ReadBlock(hashCheckpoint.load(), state))
                        break;

                    /* Check we haven't reached the genesis */
                    if(state == stateGenesis)
                        break;

                    /* Get the previous state. */
                    state = state.Prev();
                    if(!state)
                        return debug::error(FUNCTION, "failed to find the checkpoint");

                    /* Set the checkpoint. */
                    hashCheckpoint    = state.hashCheckpoint;

                    /* Get checkpoint state. */
                    BlockState stateCheckpoint;
                    if(!LLD::Ledger->ReadBlock(state.hashCheckpoint, stateCheckpoint))
                        return debug::error(FUNCTION, "failed to read checkpoint");

                    /* Set the correct height for the checkpoint. */
                    nCheckpointHeight = stateCheckpoint.nHeight;
                }
            }

            /* Ensure the block height index is intact */
            if(config::GetBoolArg("-indexheight") || config::GetBoolArg("-reindexheight"))
            {
                /* Build our indexing height. */
                TAO::Ledger::BlockState tLastBlock;
                if(config::GetBoolArg("-reindexheight") || !LLD::Ledger->ReadBlock(nCheckpointHeight.load(), tLastBlock))
                {
                    /* Set our last block as genesis. */
                    tLastBlock = stateGenesis;

                    /* Use genesis as our hash start. */
                    uint1024_t hashStart = tLastBlock.GetHash();

                    /* Track our timing. */
                    runtime::timer tElapsed;
                    tElapsed.Start();

                    /* List our blocks via a batch read for efficiency. */
                    std::vector<TAO::Ledger::BlockState> vStates;
                    while(!config::fShutdown.load() && hashStart != TAO::Ledger::ChainState::hashBestChain.load() &&
                        LLD::Ledger->BatchRead(hashStart, "block", vStates, 1000, true))
                    {
                        /* Loop through all available states. */
                        for(auto& tBlock : vStates)
                        {
                            /* Update start every iteration. */
                            hashStart = tBlock.GetHash();

                            /* Skip if not in main chain. */
                            if(!tBlock.IsInMainChain())
                                continue;

                            /* Check for matching hashes. */
                            if(tBlock.hashPrevBlock != tLastBlock.GetHash())
                            {
                                /* Read the correct block from next index. */
                                if(!LLD::Ledger->ReadBlock(tLastBlock.hashNextBlock, tBlock))
                                    return debug::error("Block not found: ", tLastBlock.hashNextBlock.SubString());

                                /* Update hashStart. */
                                hashStart = tBlock.GetHash();
                            }

                            /* Cache the block hash. */
                            tLastBlock = tBlock;

                            /* Add a meter for progress output. */
                            if(tLastBlock.nHeight % 10000 == 0)
                            {
                                /* Calculate our percentage completed. */
                                const double dPercentage = (100.0 * tLastBlock.nHeight) / TAO::Ledger::ChainState::nBestHeight.load();

                                /* Get elapsed timestamp. */
                                const uint64_t nElapsed = tElapsed.ElapsedMilliseconds() + 1;

                                /* Find remaining time. */
                                const uint64_t nRemaining =
                                    uint64_t(nElapsed * TAO::Ledger::ChainState::nBestHeight.load()) / tBlock.nHeight;

                                /* Log status message of completion time. */
                                debug::log(0, "Completed ", tLastBlock.nHeight, "/",
                                    TAO::Ledger::ChainState::nBestHeight.load(), std::fixed, " [", dPercentage, " %]",
                                    "[", (nRemaining - nElapsed) / 1000, "s remaining]");
                            }

                            /* Write the new heights to disk. */
                            if(!LLD::Ledger->IndexBlock(tBlock.nHeight, hashStart))
                                return debug::error("Failed to index height: ", hashStart.SubString());
                        }
                    }
                }
            }

            /* Check if we need to persist the -indexheight flag. */
            else
            {
                /* Build our indexing height. */
                TAO::Ledger::BlockState tLastBlock;
                if(LLD::Ledger->ReadBlock(nCheckpointHeight.load(), tLastBlock))
                {
                    /* Check there is no argument supplied. */
                    if(!config::HasArg("-indexheight"))
                    {
                        /* Warn that -indexheight is persistent. */
                        debug::log(0, FUNCTION, "-indexheight enabled from valid indexes, to disable please use -noindexheight");

                        /* Set indexing argument now. */
                        LOCK(config::ARGS_MUTEX);
                        config::mapArgs["-indexheight"] = "1";
                    }
                    else
                    {
                        /* Check for disabled mode. */
                        if(!config::GetBoolArg("-indexheight"))
                            debug::warning(FUNCTION, "-indexheight disabled with valid indexes, to enable please remove -noindexheight");
                    }
                }
            }

            stateBest.load().print();

            /* Log the weights. */
            debug::log(0, FUNCTION, "WEIGHTS",
                " Prime ", stateBest.load().nChannelWeight[1].Get64(),
                " Hash ",  stateBest.load().nChannelWeight[2].Get64(),
                " Stake ", stateBest.load().nChannelWeight[0].Get64());


            /* Debug logging. */
            debug::log(0, FUNCTION, config::fTestNet.load() ? "Test" : "Nexus", " Network: genesis=", Genesis().SubString(),
            " nBitsStart=0x", std::hex, bnProofOfWorkStart[0].GetCompact(), " best=", hashBestChain.load().SubString(),
            " checkpoint=", hashCheckpoint.load().SubString()," height=", std::dec, stateBest.load().nHeight);

            return true;
        }


        /* Get the hash of the genesis block. */
        uint1024_t ChainState::Genesis()
        {
            return (config::fHybrid.load() ? TAO::Ledger::hashGenesisHybrid : config::fTestNet.load() ? TAO::Ledger::hashGenesisTestnet : (config::fClient.load() ? TAO::Ledger::hashTritium : TAO::Ledger::hashGenesis));
        }
    }
}
