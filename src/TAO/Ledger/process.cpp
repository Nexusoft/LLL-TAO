/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

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


        /* Processes a block incoming over the network. */
        void Process(const TAO::Ledger::Block& block, uint8_t &nStatus)
        {
            LOCK(PROCESSING_MUTEX);

            /* Check if the block is valid. */
            if(!block.Check())
            {
                /* Check for missing transactions. */
                if(block.vMissing.size() != 0)
                    nStatus |= PROCESS::INCOMPLETE;

                /* Set the status. */
                nStatus |= PROCESS::REJECTED;

                return;
            }

            /* Check for orphan. */
            if(!LLD::Ledger->HasBlock(block.hashPrevBlock))
            {
                /* Skip if already in orphan queue. */
                if(!mapOrphans.count(block.hashPrevBlock))
                {
                    /* Check the checkpoint height. */
                    if(block.nHeight < TAO::Ledger::ChainState::nCheckpointHeight)
                    {
                        /* Set the status. */
                        nStatus |= PROCESS::IGNORE;

                        return;
                    }

                    /* Fast sync block requests. */
                    if(TAO::Ledger::ChainState::Synchronizing())
                        nStatus |= PROCESS::IGNORE;

                    /* Add to the orphans map. */
                    mapOrphans.insert(std::make_pair(block.hashPrevBlock, std::unique_ptr<TAO::Ledger::Block>(block.Clone())));

                    /* Debug output. */
                    debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight, " prev=", block.hashPrevBlock.ToString().substr(0, 20));
                }
                else
                    nStatus |= PROCESS::DUPLICATE;

                /* Set the status message. */
                nStatus |= PROCESS::ORPHAN;

                return;
            }

            /* Check if valid in the chain. */
            if(!block.Accept())
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
                uint64_t nElapsed = runtime::timestamp(true) - nSynchronizationTimer;
                debug::log(0, FUNCTION,
                    "Processed 1000 blocks in ", nElapsed, " ms [", std::setw(2),
                    TAO::Ledger::ChainState::PercentSynchronized(), " %]",
                    " height=", TAO::Ledger::ChainState::nBestHeight.load(),
                    " trust=", TAO::Ledger::ChainState::nBestChainTrust.load(),
                    " [", 1000000 / nElapsed, " blocks/s]");

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

                /* Accept each orphan. */
                if(!mapOrphans.at(hash)->Accept())
                    return;

                /* Erase orphans from map. */
                mapOrphans.erase(hash);
                hash = hashPrev;
            }

            return;
        }
    }
}
