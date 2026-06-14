/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/enum.h> //for internal flags
#include <TAO/Ledger/types/transaction.h>

namespace LLD
{
    /* Run our indexing entries and routines. */
    void Indexing()
    {
        /* Check to disable for -client mode. */
        if(config::fClient.load())
            return;

        debug::log(0, FUNCTION, "Indexing LLD");

        /* Track the latest height of all recent indexes. */
        std::map<std::string, uint32_t> mapHeights =
        {
            { "-indexheight",   2944206 },
            { "-indexaddress",  2944206 },
            { "-indexproofs",   2944206 },
            { "-indexregister", 2944206 }
        };

        /* Find the lowest height to start at. */
        std::pair<uint32_t, uint1024_t> pairStartingHash =
            std::make_pair(std::numeric_limits<uint32_t>::max(), 0);

        /* Check for -indexheight flags. */
        bool fIndexHeightComplete    = true,
             fIndexAddressesComplete = true,
             fIndexProofsComplete    = true,
             fIndexRegistersComplete = true;

        /* Check where we last left off. */
        uint1024_t hashIndexHeight;
        if(LLD::Ledger->ReadIndexHeight(hashIndexHeight))
        {
            /* Set our flag to false if we are forcing reindexing. */
            if(config::GetBoolArg("-reindexheight"))
                fIndexHeightComplete = false;
            else
            {
                /* Read block state of height. */
                TAO::Ledger::BlockState rState;
                if(LLD::Ledger->ReadBlock(hashIndexHeight, rState))
                {
                    /* Set our heights map. */
                    mapHeights["-indexheight"] = rState.nHeight;

                    /* If our height is less than current chain we want to scan. */
                    if(rState.nHeight < TAO::Ledger::ChainState::nBestHeight.load())
                    {
                        /* Establish we are not complete here. */
                        fIndexHeightComplete = false;

                        /* Check if this is lowest height. */
                        if(rState.nHeight < pairStartingHash.first)
                        {
                            pairStartingHash.first  = rState.nHeight;
                            pairStartingHash.second = hashIndexHeight;
                        }
                    }

                    debug::notice(FUNCTION, "-indexheight starting at height ", mapHeights["-indexheight"]);
                }
            }

            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexheight"))
            {
                /* Warn that -indexheight is persistent. */
                debug::notice(FUNCTION, "-indexheight enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexheight"] = "1";
            }
        }
        else if(config::GetBoolArg("-indexheight", false))
            fIndexHeightComplete = false;


        /* Check where we last left off. */
        uint1024_t hashIndexAddress;
        if(LLD::Register->ReadIndexAddress(hashIndexAddress))
        {
            /* Set our flag to false if we are forcing reindexing. */
            if(config::GetBoolArg("-reindexaddress"))
                fIndexAddressesComplete = false;
            else
            {
                /* Read block state of height. */
                TAO::Ledger::BlockState rState;
                if(LLD::Ledger->ReadBlock(hashIndexAddress, rState))
                {
                    /* Set our heights map. */
                    mapHeights["-indexaddress"] = rState.nHeight;

                    /* If our height is less than current chain we want to scan. */
                    if(rState.nHeight < TAO::Ledger::ChainState::nBestHeight.load())
                    {
                        /* Establish we are not complete here. */
                        fIndexAddressesComplete = false;

                        /* Check if this is lowest height. */
                        if(rState.nHeight < pairStartingHash.first)
                        {
                            pairStartingHash.first  = rState.nHeight;
                            pairStartingHash.second = hashIndexAddress;
                        }
                    }

                    debug::notice(FUNCTION, "-indexaddress starting at height ", mapHeights["-indexaddress"]);
                }
            }

            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexaddress"))
            {
                /* Warn that -indexaddress is persistent. */
                debug::notice(FUNCTION, "-indexaddress enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexaddress"] = "1";

                /* Set our internal configuration value. */
                config::fIndexAddress.store(true);
            }
        }
        else if(config::GetBoolArg("-indexaddress", false))
            fIndexAddressesComplete = false;


        /* Check where we last left off. */
        uint1024_t hashIndexProofs;
        if(LLD::Ledger->ReadIndexProofs(hashIndexProofs))
        {
            /* Reset our falgs if we have reindexed proofs. */
            if(config::GetBoolArg("-reindexproofs"))
                fIndexProofsComplete = false;
            else
            {
                /* Read block state of height. */
                TAO::Ledger::BlockState rState;
                if(LLD::Ledger->ReadBlock(hashIndexProofs, rState))
                {
                    /* Set our heights map. */
                    mapHeights["-indexproofs"] = rState.nHeight;

                    /* If our height is less than current chain we want to scan. */
                    if(rState.nHeight < TAO::Ledger::ChainState::nBestHeight.load())
                    {
                        /* Establish we are not complete here. */
                        fIndexProofsComplete = false;

                        /* Check if this is lowest height. */
                        if(rState.nHeight < pairStartingHash.first)
                        {
                            pairStartingHash.first  = rState.nHeight;
                            pairStartingHash.second = hashIndexProofs;
                        }
                    }

                    debug::notice(FUNCTION, "-indexproofs starting at height ", mapHeights["-indexproofs"]);
                }
            }

            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexproofs"))
            {
                /* Warn that -indexproofs is persistent. */
                debug::notice(FUNCTION, "-indexproofs enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexproofs"] = "1";

                /* Cache our internal arguments. */
                config::fIndexProofs.store(true);
            }
        }
        else if(config::GetBoolArg("-indexproofs", false))
            fIndexProofsComplete = false;


        /* Check where we last left off. */
        uint1024_t hashIndexRegister;
        if(LLD::Logical->ReadIndexRegisters(hashIndexRegister))
        {
            /* Reset our falgs if we have reindexed proofs. */
            if(config::GetBoolArg("-reindexregister"))
                fIndexRegistersComplete = false;
            else
            {
                /* Read block state of height. */
                TAO::Ledger::BlockState rState;
                if(LLD::Ledger->ReadBlock(hashIndexRegister, rState))
                {
                    /* Set our heights map. */
                    mapHeights["-indexregister"] = rState.nHeight;

                    /* If our height is less than current chain we want to scan. */
                    if(rState.nHeight < TAO::Ledger::ChainState::nBestHeight.load())
                    {
                        fIndexRegistersComplete = false;

                        /* Check if this is lowest height. */
                        if(rState.nHeight < pairStartingHash.first)
                        {
                            pairStartingHash.first  = rState.nHeight;
                            pairStartingHash.second = hashIndexRegister;
                        }
                    }

                    /* Establish we are not complete here. */
                    debug::notice(FUNCTION, "-indexregister starting at height ", mapHeights["-indexregister"]);
                }
            }


            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexregister"))
            {
                /* Warn that -indexregister is persistent. */
                debug::notice(FUNCTION, "-indexregister enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexregister"] = "1";

                /* Set our internal configuration. */
                config::fIndexRegister.store(true);
            }
        }
        else if(config::GetBoolArg("-indexregister", false))
            fIndexRegistersComplete = false;

        /* We don't need to do any work here if all of our indexes are complete. */
        if(fIndexHeightComplete && fIndexAddressesComplete && fIndexProofsComplete && fIndexRegistersComplete)
            return;

        /* Our list of transactions to read. */
        std::map<uint512_t, TAO::Ledger::Transaction> mapTransactions;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Track the last block processed. */
        TAO::Ledger::BlockState tStateLast;

        /* Set our internal values. */
        uint1024_t hashBlock = TAO::Ledger::hashTritium;

        /* Check for testnet mode. */
        if(config::fTestNet.load())
            hashBlock = TAO::Ledger::hashGenesisTestnet;

        /* Check for hybrid mode. */
        if(config::fHybrid.load())
            LLD::Ledger->ReadHybridGenesis(hashBlock);

        /* Check if we have a lowest height to start from. */
        if(pairStartingHash.first != std::numeric_limits<uint32_t>::max())
            hashBlock = pairStartingHash.second;

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


        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Keep track of already processed addresses. */
        std::set<uint256_t> setScanned;

        /* Start our scan. */
        debug::log(0, FUNCTION, "Scanning from block ", hashBlock.SubString());

        /* Set error logging to off for indexing. */
        debug::fLogError = false;

        /* Build our loop based on the blocks we have read sequentially. */
        std::vector<TAO::Ledger::BlockState> vStates;
        while(!config::fShutdown.load())
        {
            /* Start an ACID transaction based on block batches. */
            LLD::Ledger->TxnBegin();
            LLD::Register->TxnBegin();
            LLD::Contract->TxnBegin();
            LLD::Logical->TxnBegin();

            /* Break if we can't read the batch */
            if(!LLD::Ledger->BatchRead(hashBlock, "block", vStates, 1000, true))
                break;

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

                /* Handle for indexing the height. */
                if(!fIndexHeightComplete && state.nHeight > mapHeights["-indexheight"])
                {
                    /* Write the new heights to disk. */
                    LLD::Ledger->IndexBlock(state.nHeight, hashBlock);
                }

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
                    std::set<uint256_t> setAddresses;
                    for(uint32_t nContract = 0; nContract < rTX.Size(); ++nContract)
                    {
                        /* Grab contract reference. */
                        const TAO::Operation::Contract& rContract = rTX[nContract];

                        /* Handle if we need to index our proofs. */
                        if(!fIndexProofsComplete && state.nHeight > mapHeights["-indexproofs"])
                        {
                            /* Check for a validation index. */
                            uint512_t hashTx;
                            if(TAO::Register::Unpack(rContract, hashTx, nContract))
                            {
                                /* Check that we have the contract validated. */
                                if(Contract->HasContract(std::make_pair(hashTx, nContract)))
                                {
                                    /* Index our record to the database. */
                                    Ledger->IndexContract(hashTx, nContract, proof.second);
                                }

                                /* Unpack the contract info we are working on. */
                                uint256_t hashProof;
                                if(TAO::Register::Unpack(rContract, hashProof, hashTx, nContract))
                                {
                                    /* Check for a valid proof. */
                                    if(Ledger->HasProof(hashProof, hashTx, nContract))
                                    {
                                        /* Index our record to the database. */
                                        Ledger->IndexProof(hashProof, hashTx, nContract, proof.second);
                                    }
                                }
                            }
                        }

                        /* Handle if we need to index our addresses. */
                        if(!fIndexRegistersComplete  && state.nHeight > mapHeights["-indexregister"])
                        {
                            /* Unpack the address we will be working on. */
                            uint256_t hashAddress;
                            if(TAO::Register::Unpack(rContract, hashAddress))
                            {
                                /* Check for duplicate entries. */
                                if(!setAddresses.count(hashAddress))
                                {
                                    /* Check fo register in database. */
                                    if(Logical->PushRegisterTx(hashAddress, proof.second))
                                    {
                                        /* Push the address now. */
                                        setAddresses.insert(hashAddress);
                                    }
                                }
                            }
                        }

                        /* Handle if we need to index addresses. */
                        if(!fIndexAddressesComplete && state.nHeight > mapHeights["-indexaddress"])
                        {
                            /* Unpack the address we will be working on. */
                            uint256_t hashAddress;
                            if(TAO::Register::Unpack(rContract, hashAddress))
                            {
                                /* Check if already in set. */
                                if(!setScanned.count(hashAddress))
                                {
                                    /* Track if we have read either choice. */
                                    TAO::Register::State rState;
                                    if(Register->Read(std::make_pair(std::string("state"), hashAddress), rState))
                                    {
                                        /* We should have a valid state if the register hasn't had this index set already. */
                                        if(rState.IsValid())
                                        {
                                            //debug::notice("Register is valid, writing new index.");

                                            /* Erase our record from the database. */
                                            Register->Erase(std::make_pair(std::string("state"), hashAddress));

                                            /* Create our new register record. */
                                            if(Register->Write(std::make_pair(std::string("state"), hashAddress),
                                                std::make_pair(hashAddress, rState), Register->get_address_type(hashAddress) + "_address"))
                                            {
                                                /* Update this item as scanned now. */
                                                setScanned.insert(hashAddress);

                                                /* Update database pointer with position. */
                                                LLD::Register->WriteIndexAddress(hashBlock);
                                            }
                                        }
                                        else
                                        {
                                            /* Check if we have a valid state. */
                                            std::pair<uint256_t, TAO::Register::State&> pairRead = std::make_pair(0, std::ref(rState));
                                            if(Register->Read(std::make_pair(std::string("state"), hashAddress), pairRead))
                                            {
                                                /* Check if we are in a valid state. */
                                                if(rState.IsValid())
                                                {
                                                    //debug::notice("Register index already exists...");

                                                    /* Update this item as scanned now. */
                                                    setScanned.insert(hashAddress);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }


                    }

                    /* Delete processed transaction from memory. */
                    mapTransactions.erase(proof.second);

                    /* Update the scanned count for meters. */
                    ++nScannedCount;

                    /* Meter for output. */
                    if(nScannedCount % 10000 == 0)
                    {
                        /* Get the time it took to rescan. */
                        const uint32_t nElapsedSeconds = timer.Elapsed();
                        debug::log(0, FUNCTION, "Processed ", nScannedCount, " in ", nElapsedSeconds, " seconds from height ", tStateLast.nHeight, " (",
                            std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                    }
                }

                /* Check if we are ready to terminate. */
                if(hashBlock == TAO::Ledger::ChainState::hashBestChain.load())
                    break;
            }

            /* Write the new -indexheight to disk. */
            if(!fIndexHeightComplete && tStateLast.nHeight > mapHeights["-indexheight"])
            {
                /* Write the index to the disk. */
                LLD::Ledger->WriteIndexHeight(hashBlock);

                /* Update our heights map. */
                mapHeights["-indexheight"] = tStateLast.nHeight;
                //debug::notice("Writing -indexheight at height ", tStateLast.nHeight);
            }

            /* Write the new -indexaddress to disk. */
            if(!fIndexAddressesComplete && tStateLast.nHeight > mapHeights["-indexaddress"])
            {
                /* Write the index to the disk. */
                LLD::Register->WriteIndexAddress(hashBlock);

                /* Update our heights map. */
                mapHeights["-indexaddress"] = tStateLast.nHeight;
                //debug::notice("Writing -indexaddress at height ", tStateLast.nHeight);
            }

            /* Write the new -indexproofs to disk. */
            if(!fIndexProofsComplete && tStateLast.nHeight > mapHeights["-indexproofs"])
            {
                /* Write the index to the disk. */
                LLD::Ledger->WriteIndexProofs(hashBlock);

                /* Update our heights map. */
                mapHeights["-indexproofs"] = tStateLast.nHeight;
                //debug::notice("Writing -indexproofs at height ", tStateLast.nHeight);
            }

            /* Write the new -indexregister to disk. */
            if(!fIndexRegistersComplete && tStateLast.nHeight > mapHeights["-indexregister"])
            {
                /* Write the index to the disk. */
                LLD::Logical->WriteIndexRegisters(hashBlock);

                /* Update our heights map. */
                mapHeights["-indexregister"] = tStateLast.nHeight;
                //debug::notice("Writing -indexregister at height ", tStateLast.nHeight);
            }

            /* Start an ACID transaction based on block batches. */
            LLD::Ledger->TxnCommit();
            LLD::Register->TxnCommit();
            LLD::Contract->TxnCommit();
            LLD::Logical->TxnCommit();
        }


        /* Set error logging to off for indexing. */
        debug::fLogError = true;

        debug::log(0, FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }


    /* Global handler for our LLD::Indexing to keep indexes up to date on chain. */
    void UpdateIndexing(const uint1024_t& hashBlock)
    {
        /* Update our -indexheight indexes. */
        if(config::GetBoolArg("-indexheight", false))
            LLD::Ledger->WriteIndexHeight(hashBlock);

        /* Update our -indexproofs indexes. */
        if(config::GetBoolArg("-indexproofs", false))
            LLD::Ledger->WriteIndexProofs(hashBlock);

        /* Update our -indexaddress indexes. */
        if(config::GetBoolArg("-indexaddress", false))
            LLD::Register->WriteIndexAddress(hashBlock);

        /* Update our -indexregister indexes. */
        if(config::GetBoolArg("-indexregister", false))
            LLD::Logical->WriteIndexRegisters(hashBlock);
    }
}
