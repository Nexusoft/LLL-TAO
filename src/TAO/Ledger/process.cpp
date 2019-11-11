/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* Static instantiation of orphan blocks in queue to process. */
        std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapOrphans;


        /* Mutex to protect checking more than one block at a time. */
        std::mutex PROCESSING_MUTEX;


        /* Sync timer value. */
        uint64_t nSynchronizationTimer = 0;


        /* Current sync node. */
        std::atomic<uint64_t> nSyncSession(0);


        /* Processes a block incoming over the network. */
        void Process(const TAO::Ledger::Block& block, uint8_t &nStatus)
        {
            LOCK(PROCESSING_MUTEX);

            /* Check for orphan. */
            if(!LLD::Ledger->HasBlock(block.hashPrevBlock))
            {
                /* Set the status message. */
                nStatus |= PROCESS::ORPHAN;

                /* Skip if already in orphan queue. */
                if(!mapOrphans.count(block.hashPrevBlock))
                {
                    /* Check the checkpoint height. */
                    if(!config::fTestNet.load() && block.nHeight < TAO::Ledger::ChainState::nCheckpointHeight)
                    {
                        /* Set the status. */
                        nStatus |= PROCESS::IGNORED;

                        return;
                    }

                    /* Fast sync block requests. */
                    if(TAO::Ledger::ChainState::Synchronizing())
                        nStatus |= PROCESS::IGNORED;

                    /* Insert into orphans map. */
                    mapOrphans.insert(std::make_pair(block.hashPrevBlock, std::unique_ptr<TAO::Ledger::Block>(block.Clone())));

                    /* Debug output. */
                    debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight, " prev=", block.hashPrevBlock.SubString());
                }
                else
                    nStatus |= PROCESS::DUPLICATE;

                return;
            }

            /* Check if the block is valid. */
            if(!block.Check())
            {
                /* Check for missing transactions. */
                if(block.vMissing.size() == 0)
                {
                    nStatus |= PROCESS::REJECTED;
                    return;
                }

                /* Incomplete blocks can pass through orphan checks. */
                nStatus |= PROCESS::INCOMPLETE;

                /* Set the missing block. */
                block.hashMissing = block.GetHash();

                return;
            }

            /* Check if valid in the chain. */
            else if(!block.Accept())
            {
                /* Set the status. */
                nStatus |= PROCESS::REJECTED;

                return;
            }

            /* Set the status. */
            nStatus |= PROCESS::ACCEPTED;

            /* Special meter for synchronizing. */
            if(block.nHeight % 1000 == 0 && TAO::Ledger::ChainState::Synchronizing())
            {
                /* Grab the current sync node. */
                uint32_t nHours = 0, nMinutes = 0, nSeconds = 0;
                if(LLP::TritiumNode::SessionActive(nSyncSession.load()))
                {
                    /* Get the current connected legacy node. */
                    memory::atomic_ptr<LLP::TritiumNode>& pnode = LLP::TritiumNode::GetNode(nSyncSession.load());
                    try //we want to catch exceptions thrown by atomic_ptr in the case there was a free on another thread
                    {
                        /* Check for potential overflow if current height is not set. */
                        if(pnode->nCurrentHeight > ChainState::nBestHeight.load())
                        {
                            /* Get the total height left to go. */
                            uint32_t nRemaining = (pnode->nCurrentHeight - ChainState::nBestHeight.load());
                            uint32_t nTotalBlocks = (ChainState::nBestHeight.load() - pnode->nSyncStart);

                            /* Calculate blocks per second. */
                            uint32_t nRate = nTotalBlocks / (pnode->SYNCTIMER.Elapsed() + 1);
                            LLP::TritiumNode::nRemainingTime.store(nRemaining / (nRate + 1));

                            /* Get the remaining time. */
                            nHours   = LLP::TritiumNode::nRemainingTime.load() / 3600;
                            nMinutes = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) / 60;
                            nSeconds = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) % 60;
                        }
                    }
                    catch(const std::exception& e) {}
                }

                uint64_t nElapsed = runtime::timestamp(true) - nSynchronizationTimer;
                debug::log(0, FUNCTION,
                    "Processed 1000 blocks in ", nElapsed, " ms [", std::setw(2),
                    TAO::Ledger::ChainState::PercentSynchronized(), " %]",
                    " height=", block.nHeight,
                    " trust=", TAO::Ledger::ChainState::nBestChainTrust.load(),
                    " [", 1000000 / nElapsed, " blocks/s]",
                    "[", std::setw(2), std::setfill('0'), nHours, ":",
                          std::setw(2), std::setfill('0'), nMinutes, ":",
                          std::setw(2), std::setfill('0'), nSeconds, " remaining]");

                nSynchronizationTimer = runtime::timestamp(true);
            }

            /* Process orphan if found. */
            uint1024_t hash = block.GetHash();
            while(mapOrphans.count(hash))
            {
                /* Get the next hash backwards in the series. */
                uint1024_t hashPrev = mapOrphans.at(hash)->GetHash();

                /* Debug output. */
                debug::log(0, FUNCTION, "processing ORPHAN prev=", hashPrev.SubString(), " size=", mapOrphans.size());

                /* Check if the block is valid. */
                if(!mapOrphans.at(hash)->Check())
                {
                    /* Check for missing transactions. */
                    if(mapOrphans.at(hash)->vMissing.size() == 0)
                        return;

                    /* Incomplete blocks can pass through orphan checks. */
                    nStatus |= PROCESS::INCOMPLETE;

                    /* Add the missing transactions to this current block. */
                    block.vMissing.insert(block.vMissing.end(), mapOrphans.at(hash)->vMissing.begin(), mapOrphans.at(hash)->vMissing.end());

                    /* Set the hash missing. */
                    block.hashMissing = mapOrphans.at(hash)->GetHash();
                }

                /* Accept each orphan. */
                else if(!mapOrphans.at(hash)->Accept())
                    return;

                /* Erase orphans from map. */
                mapOrphans.erase(hash);
                hash = hashPrev;
            }

            return;
        }
    }
}
