/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"Cogito, ergo sum" - 	I think, therefore I am

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/indexing.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /* Checks current events against transaction history to ensure we are up to date. */
    void Indexing::RefreshEvents()
    {
        /* Check to disable for -client mode. */
        if(config::fClient.load())
            return;

        /* Our list of transactions to read. */
        std::map<uint512_t, TAO::Ledger::Transaction> mapTransactions;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Check our starting block to read from. */
        uint1024_t hashBlock;

        /* Track the last block processed. */
        TAO::Ledger::BlockState tStateLast;

        /* Handle first key if needed. */
        uint512_t hashIndex;
        if(!LLD::Logical->ReadLastIndex(hashIndex))
        {
            /* Set our internal values. */
            hashBlock = TAO::Ledger::hashTritium;

            /* Check for testnet mode. */
            if(config::fTestNet.load())
                hashBlock = TAO::Ledger::hashGenesisTestnet;

            /* Check for hybrid mode. */
            if(config::fHybrid.load())
                LLD::Ledger->ReadHybridGenesis(hashBlock);

            /* Read the first tritium block. */
            TAO::Ledger::BlockState tCurrent;
            if(!LLD::Ledger->ReadBlock(hashBlock, tCurrent))
            {
                debug::warning(FUNCTION, "No tritium blocks available to initialize ", hashBlock.SubString());
                return;
            }

            /* Set our last block as prev tritium block. */
            if(!tCurrent.Prev())
                tStateLast = tCurrent;
            else
            {
                hashBlock  = tCurrent.hashPrevBlock;
                tStateLast = tCurrent.Prev();
            }

            debug::log(0, FUNCTION, "Initializing indexing at tx ", hashBlock.SubString(), " and height ", tCurrent.nHeight);
        }
        else
        {
            /* Set our initial block hash. */
            TAO::Ledger::BlockState tCurrent;
            if(LLD::Ledger->ReadBlock(hashIndex, tCurrent))
            {
                /* Set our last block hash. */
                hashBlock = tCurrent.hashPrevBlock;

                /* Set our last block as prev tritium block. */
                tStateLast = tCurrent.Prev();
            }
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::log(0, FUNCTION, "Scanning from block ", hashBlock.SubString());

        /* Build our loop based on the blocks we have read sequentially. */
        std::vector<TAO::Ledger::BlockState> vStates;
        while(!config::fShutdown.load() && LLD::Ledger->BatchRead(hashBlock, "block", vStates, 1000, true))
        {
            /* Loop through all available states. */
            for(auto& state : vStates)
            {
                /* Update start every iteration. */
                hashBlock = state.GetHash();

                /* Skip if not in main chain. */
                if(!state.IsInMainChain())
                    continue;

                /* Check for matching hashes. */
                if(state.hashPrevBlock != tStateLast.GetHash())
                {
                    debug::log(0, FUNCTION, "Correcting chain ", tStateLast.hashNextBlock.SubString());

                    /* Read the correct block from next index. */
                    if(!LLD::Ledger->ReadBlock(tStateLast.hashNextBlock, state))
                    {
                        debug::log(0, FUNCTION, "Terminated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
                        return;
                    }

                    /* Update hashBlock. */
                    hashBlock = state.GetHash();
                }

                /* Cache the block hash. */
                tStateLast = state;

                /* Track our checkpoint by first transaction in non-processed block. */
                LLD::Logical->WriteLastIndex(state.vtx[0].second);

                /* Handle our transactions now. */
                for(const auto& proof : state.vtx)
                {
                    /* Skip over legacy indexes. */
                    if(proof.first == TAO::Ledger::TRANSACTION::LEGACY)
                        continue;

                    /* Check our map contains transactions. */
                    if(!mapTransactions.count(proof.second))
                    {
                        /* Read the next batch of inventory. */
                        std::vector<TAO::Ledger::Transaction> vList;
                        if(LLD::Ledger->BatchRead(proof.second, "tx", vList, 1000, false))
                        {
                            /* Add all of our values to a map. */
                            for(const auto& tBatch : vList)
                                mapTransactions[tBatch.GetHash()] = tBatch;
                        }
                    }

                    /* Check that we found it in batch. */
                    if(!mapTransactions.count(proof.second))
                    {
                        /* Track this warning since this should not happen. */
                        debug::warning(FUNCTION, "batch read for ", proof.second.SubString(), " did not find results");

                        /* Make sure we have the transaction. */
                        TAO::Ledger::Transaction tMissing;
                        if(LLD::Ledger->ReadTx(proof.second, tMissing))
                            mapTransactions[proof.second] = tMissing;
                        else
                        {
                            debug::warning(FUNCTION, "single read for ", proof.second.SubString(), " is missing");
                            continue;
                        }
                    }

                    /* Get our transaction now. */
                    const TAO::Ledger::Transaction& rTX =
                        mapTransactions[proof.second];

                    /* Iterate the transaction contracts. */
                    for(uint32_t nContract = 0; nContract < rTX.Size(); ++nContract)
                    {
                        /* Grab contract reference. */
                        const TAO::Operation::Contract& rContract = rTX[nContract];

                        {
                            LOCK(REGISTERED_MUTEX);

                            /* Loop through registered commands. */
                            for(const auto& strCommands : REGISTERED)
                                Commands::Instance(strCommands)->Index(rContract, nContract);
                        }
                    }

                    /* Delete processed transaction from memory. */
                    mapTransactions.erase(proof.second);

                    /* Update the scanned count for meters. */
                    ++nScannedCount;

                    /* Meter for output. */
                    if(nScannedCount % 100000 == 0)
                    {
                        /* Get the time it took to rescan. */
                        const uint32_t nElapsedSeconds = timer.Elapsed();
                        debug::log(0, FUNCTION, "Processed ", nScannedCount, " in ", nElapsedSeconds, " seconds from height ", state.nHeight, " (",
                            std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                    }
                }

                /* Check if we are ready to terminate. */
                if(hashBlock == TAO::Ledger::ChainState::hashBestChain.load())
                    break;
            }
        }

        debug::log(0, FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }
}
