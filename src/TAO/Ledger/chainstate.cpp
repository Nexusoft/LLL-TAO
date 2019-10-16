/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>

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
        static std::atomic<bool> fSynchronizing(true);
        bool ChainState::Synchronizing()
        {
            /* Persistent switch once synchronized. */
            if(!fSynchronizing.load())
                return false;

            #ifndef UNIT_TESTS

            /* Check for null best state. */
            if(stateBest.load().IsNull())
                return true;

            /* Check if there's been a new block. */
            static memory::atomic<uint1024_t> hashLast;
            static std::atomic<uint64_t> nLastTime;
            if(hashBestChain.load() != hashLast.load())
            {
                hashLast = hashBestChain.load();
                nLastTime = runtime::unifiedtimestamp();
            }

            /* Special testnet rule. */
            if(config::fTestNet.load())
            {
                /* Set the synchronizing flag. */
                fSynchronizing =
                (
                    (stateBest.load().GetBlockTime() < runtime::unifiedtimestamp() - 20 * 60) &&
                    (runtime::unifiedtimestamp() - nLastTime < 30)
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
            return false;
            #endif
        }


        /* Flag to tell if initial blocks are downloading. */
        double ChainState::PercentSynchronized()
        {
            uint32_t nChainAge = (static_cast<uint32_t>(runtime::unifiedtimestamp()) - 60 * 60) - (config::fTestNet.load() ?
                NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK);

            uint32_t nSyncAge  = static_cast<uint32_t>(stateBest.load().GetBlockTime() - static_cast<uint64_t>(config::fTestNet.load() ?
                NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK));

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

            /* Check database consistency. */
            if(stateBest.load().GetHash() != hashBestChain.load())
                return debug::error(FUNCTION, "disk index inconsistent with best chain");

            /* Rewind the chain a total number of blocks. */
            int64_t nForkblocks = config::GetArg("-forkblocks", 0);
            if(nForkblocks > 0)
            {
                /* Rollback the chain a given number of blocks. */
                TAO::Ledger::BlockState state = stateBest.load();

                debug::log(0, FUNCTION, "forkblocks requested removal of ", nForkblocks, " blocks");

                for(int i = 0; i < nForkblocks; ++i)
                {
                    if(state.hashPrevBlock == 0)
                        break; //Stop if reach genesis

                    state = state.Prev();

                    if(!state)
                        return debug::error(FUNCTION, "failed to find ancestor block");
                }

                /* Set the best to older block. */
                LLD::TxnBegin();
                state.SetBest();
                LLD::TxnCommit();
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
                hashCheckpoint    = state.hashCheckpoint;

                /* Get checkpoint state. */
                BlockState stateCheckpoint;
                if(!LLD::Ledger->ReadBlock(state.hashCheckpoint, stateCheckpoint))
                    return debug::error(FUNCTION, "failed to read checkpoint");

                /* Set the correct height for the checkpoint. */
                nCheckpointHeight = stateCheckpoint.nHeight;
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
            return config::fTestNet.load() ? TAO::Ledger::hashGenesisTestnet : TAO::Ledger::hashGenesis;
        }
    }
}
