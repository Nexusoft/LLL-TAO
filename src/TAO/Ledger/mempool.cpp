/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Ledger/include/create.h>


/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {

        Mempool mempool;

        /** Default Constructor. **/
        Mempool::Mempool()
        : MUTEX              ( )
        , mapLegacy          ( )
        , mapLegacyConflicts ( )
        , mapLedger          ( )
        , mapConflicts       ( )
        , mapOrphans         ( )
        , mapClaimed         ( )
        , mapRejected        ( )
        , mapInputs          ( )
        , setOrphansByIndex  ( )
        {
        }


        /** Default Destructor. **/
        Mempool::~Mempool()
        {
        }


        /* Add a transaction to the memory pool without validation checks. */
        bool Mempool::AddUnchecked(const TAO::Ledger::Transaction& tx)
        {
            /* Get the transaction hash. */
            const uint512_t hashTx = tx.GetHash();

            RECURSIVE(MUTEX);

            /* Check the mempool. */
            if(mapLedger.count(hashTx))
                return false;

            /* Add to the map. */
            mapLedger[hashTx] = tx;

            return true;
        }


        /* Accepts a transaction with validation rules. */
        bool Mempool::Accept(const TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode)
        {
            RECURSIVE(MUTEX);

            /* Get the transaction hash. */
            uint512_t hashTx = tx.GetHash();

            try
            {
                /* Check for transaction on disk. */
                if(LLD::Ledger->HasTx(hashTx, FLAGS::MEMPOOL))
                    return false; //NOTE: this was true, but changed to false to prevent relay loops in tritium LLP

                /* Check for rejected tx. */
                if(mapRejected.count(tx.hashPrevTx))
                {
                    mapRejected.insert(hashTx);
                    return false;
                }

                /* Print the transaction here. */
                if(config::nVerbose >= 3)
                    tx.print();

                /* Runtime calculations. */
                runtime::timer timer;
                timer.Start();

                /* Check for duplicate coinbase or coinstake. */
                if(tx.IsCoinBase())
                {
                    mapRejected.insert(hashTx);
                    return debug::error(FUNCTION, "coinbase ", hashTx.SubString(), " not accepted in pool");
                }

                /* Check for duplicate coinbase or coinstake. */
                if(tx.IsCoinStake())
                {
                    mapRejected.insert(hashTx);
                    return debug::error(FUNCTION, "coinstake ", hashTx.SubString(), " not accepted in pool");
                }

                /* Check that the transaction is in a valid state. */
                if(!tx.Check())
                {
                    mapRejected.insert(hashTx);
                    return debug::error(FUNCTION, "tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());
                }

                /* Check for orphans and conflicts when not first transaction. */
                if(!tx.IsFirst())
                {
                    /* Check memory and disk for previous transaction. */
                    if(!LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::MEMPOOL))
                    {
                        /* Debug output. */
                        debug::log(0, FUNCTION, "tx ", hashTx.SubString(), " ",
                            tx.nSequence, " prev ", tx.hashPrevTx.SubString(),
                            " ORPHAN in ", std::dec, timer.ElapsedMilliseconds(), " ms");

                        /* Push to orphan queue. */
                        mapOrphans[tx.hashPrevTx] = tx;
                        setOrphansByIndex.insert(hashTx);

                        /* Increment consecutive orphans. */
                        if(pnode)
                            ++pnode->nConsecutiveOrphans;

                        /* Ask for the missing transaction. */
                        if(pnode)
                            pnode->PushMessage(LLP::TritiumNode::ACTION::GET, uint8_t(LLP::TritiumNode::TYPES::TRANSACTION), tx.hashPrevTx);

                        return false;
                    }

                    /* Check for conflicts. */
                    if(mapClaimed.count(tx.hashPrevTx) || mapConflicts.count(tx.hashPrevTx))
                    {
                        /* Add to conflicts map. */
                        debug::error(FUNCTION, "CONFLICT: prev tx ", (mapClaimed.count(tx.hashPrevTx) ? "CLAIMED " : "CONFLICTED "), tx.hashPrevTx.SubString());
                        mapConflicts[hashTx] = tx;

                        return false;
                    }

                    /* Get the last hash. */
                    uint512_t hashLast = 0;
                    if(!LLD::Ledger->ReadLast(tx.hashGenesis, hashLast, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "tx ", hashTx.SubString(), " REJECTED: Failed to read hash last");

                    /* Check for conflicts. */
                    if(tx.hashPrevTx != hashLast)
                    {
                        /* Add to conflicts map. */
                        debug::error(FUNCTION, "CONFLICT: hash last mismatch ", tx.hashPrevTx.SubString(), " and ", hashLast.SubString());
                        mapConflicts[hashTx] = tx;

                        return false;
                    }
                }
                else if(LLD::Ledger->HasFirst(tx.hashGenesis))
                {
                    /* Add to conflicts map. */
                    debug::error(FUNCTION, "CONFLICT: duplicate genesis-id ", tx.hashGenesis.SubString());
                    mapConflicts[hashTx] = tx;

                    return false;
                }

                /* Begin an ACID transaction for internal memory commits. */
                if(!tx.Verify(FLAGS::MEMPOOL))
                {
                    mapRejected.insert(hashTx);
                    return debug::error(FUNCTION, "tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());
                }

                /* Connect transaction in memory. */
                LLD::TxnBegin(FLAGS::MEMPOOL);
                if(!tx.Connect(FLAGS::MEMPOOL))
                {
                    /* Abort memory commits on failures. */
                    LLD::TxnAbort(FLAGS::MEMPOOL);
                    mapRejected.insert(hashTx);

                    return debug::error(FUNCTION, "tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());
                }

                /* Commit new memory into database states. */
                LLD::TxnCommit(FLAGS::MEMPOOL);

                /* Set the internal memory. */
                mapLedger[hashTx] = tx;

                /* Update map claimed if not first tx. */
                if(!tx.IsFirst())
                    mapClaimed[tx.hashPrevTx] = hashTx;

                /* Debug output. */
                debug::log(2, FUNCTION, "tx ", hashTx.SubString(), " ACCEPTED in ", std::dec, timer.ElapsedMilliseconds(), " ms");

                /* Process orphan queue. */
                ProcessOrphans(hashTx);

                /* Relay tx if creating ourselves. */
                if(!pnode && LLP::TRITIUM_SERVER)
                {
                    /* Relay the transaction notification. */
                    LLP::TRITIUM_SERVER->Relay
                    (
                        LLP::TritiumNode::ACTION::NOTIFY,
                        uint8_t(LLP::TritiumNode::TYPES::TRANSACTION),
                        hashTx
                    );
                }

                /* Notify private to produce block if valid. */
                if(config::fHybrid.load())
                    PRIVATE_CONDITION.notify_all();

                return true;
            }
            catch(const std::exception& e)
            {
                mapRejected.insert(hashTx);
                return debug::error(FUNCTION, "REJECTED: exception encountered ", e.what());
            }

            return false;
        }


        /* Process orphan transactions if triggered in queue. */
        void Mempool::ProcessOrphans(const uint512_t& hash)
        {
            RECURSIVE(MUTEX);

            /* Check orphan queue. */
            uint512_t hashTx = hash;
            while(mapOrphans.count(hashTx))
            {
                /* Get the transaction from map. */
                const TAO::Ledger::Transaction& tx = mapOrphans[hashTx];

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

                /* Get the previous hash. */
                const uint512_t hashThis = tx.GetHash();

                /* Debug output. */
                debug::log(0, FUNCTION, "PROCESSING ORPHAN tx ", hashThis.SubString());

                /* Accept the transaction into memory pool. */
                if(!Accept(tx))
                {
                    debug::log(0, FUNCTION, "ORPHAN tx ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                    break;
                }

                /* Erase the transaction. */
                mapOrphans.erase(hashTx);
                setOrphansByIndex.erase(hashThis);

                /* Set the hashTx. */
                hashTx = hashThis;
            }
        }


        /* Gets a transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted) const
        {
            RECURSIVE(MUTEX);

            /* Check in conflict memory. */
            if(mapConflicts.count(hashTx))
            {
                /* Get from conflicts map. */
                tx = mapConflicts.at(hashTx);
                fConflicted = true;

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

                debug::log(0, FUNCTION, "CONFLICTED TRANSACTION: ", hashTx.SubString());

                return true;
            }

            /* Check in ledger memory. */
            if(mapLedger.count(hashTx))
            {
                tx = mapLedger.at(hashTx);

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

                return true;
            }

            return false;
        }


        /* Gets a transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const
        {
            RECURSIVE(MUTEX);

            /* Check in ledger memory. */
            if(mapLedger.count(hashTx))
            {
                tx = mapLedger.at(hashTx);

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

                return true;
            }

            return false;
        }


        /* Get by genesis. */
        bool Mempool::Get(const uint256_t& hashGenesis, std::vector<TAO::Ledger::Transaction> &vtx) const
        {
            RECURSIVE(MUTEX);

            /* Check through the ledger map for the genesis. */
            for(const auto& tx : mapLedger)
            {
                /* Check for non-conflicted genesis-id's. */
                if(tx.second.hashGenesis == hashGenesis)
                {
                    /* Cache our txid in here. */
                    tx.second.hashCache = tx.first;
                    vtx.push_back(tx.second);
                }
            }

            /* Check that a transaction was found. */
            if(vtx.size() == 0)
                return false;

            /* Sort the list by sequence numbers. */
            std::sort(vtx.begin(), vtx.end());

            /* Check that the mempool transactions are in correct order. */
            uint512_t hashLast = vtx[0].GetHash();
            for(uint32_t n = 1; n < vtx.size(); ++n)
            {
                /* Check that transaction is in sequence. */
                if(vtx[n].hashPrevTx != hashLast)
                {
                    debug::log(0, FUNCTION, "Last hash mismatch");

                    /* Resize to forget about mismatched sequences. */
                    vtx.resize(n);

                    break;
                }

                /* Set last hash. */
                hashLast = vtx[n].GetHash();
            }

            return (vtx.size() > 0);
        }


        /* Gets a transaction by genesis. */
        bool Mempool::Get(const uint256_t& hashGenesis, TAO::Ledger::Transaction &tx) const
        {
            /* Get the list of transactions by genesis. */
            std::vector<TAO::Ledger::Transaction> vtx;
            if(!Get(hashGenesis, vtx))
                return false;

            /* Return last item in list (newest). */
            tx = vtx.back();

            return true;
        }


        /* Checks if a transaction exists. */
        bool Mempool::Has(const uint512_t& hashTx) const
        {
            RECURSIVE(MUTEX);

            return mapLedger.count(hashTx) || mapLegacy.count(hashTx) || mapConflicts.count(hashTx);
        }


        /* Checks if a genesis exists. */
        bool Mempool::Has(const uint256_t& hashGenesis) const
        {
            RECURSIVE(MUTEX);

            /* Check through the ledger map for the genesis. */
            for(const auto& tx : mapLedger)
                if(tx.second.hashGenesis == hashGenesis)
                    return true;

            return false;
        }


        /* Remove a transaction from pool. */
        bool Mempool::Remove(const uint512_t& hashTx)
        {
            RECURSIVE(MUTEX);

            /* Erase from conflicted memory. */
            if(mapConflicts.count(hashTx))
                mapConflicts.erase(hashTx);

            /* Erase from legacy conflicted memory. */
            if(mapLegacyConflicts.count(hashTx))
                mapLegacyConflicts.erase(hashTx);

            /* Erase from orphans memory. */
            if(setOrphansByIndex.count(hashTx))
                setOrphansByIndex.erase(hashTx);

            /* Find the transaction in pool. */
            if(mapLedger.count(hashTx))
            {
                /* Get a reference from the map. */
                const TAO::Ledger::Transaction& tx = mapLedger[hashTx];

                /* Erase from the memory map. */
                mapClaimed.erase(tx.hashPrevTx);
                mapOrphans.erase(tx.hashPrevTx);
                mapLedger.erase(hashTx);

                return true;
            }

            /* Find the legacy transaction in pool. */
            if(mapLegacy.count(hashTx))
            {
                const Legacy::Transaction& tx = mapLegacy[hashTx];

                /* Erase the claimed inputs */
                uint32_t nSize = static_cast<uint32_t>(tx.vin.size());
                for(uint32_t i = 0; i < nSize; ++i)
                    mapInputs.erase(tx.vin[i].prevout);

                mapLegacy.erase(hashTx);
            }

            return false;
        }


        /* Check the memory pool for consistency. */
        void Mempool::Check()
        {
            RECURSIVE(MUTEX);

            //TODO: evict conflicted transactions from mempool

            /* Create map of transactions by genesis. */
            std::map<uint256_t, std::vector<TAO::Ledger::Transaction> > mapTransactions;

            /* Loop through all the transactions. */
            for(const auto& tx : mapLedger)
            {
                /* Cache the genesis. */
                const uint256_t& hashGenesis = tx.second.hashGenesis;

                /* Check in map for push back. */
                if(!mapTransactions.count(hashGenesis))
                    mapTransactions[hashGenesis] = std::vector<TAO::Ledger::Transaction>();

                /* Push to back of map. */
                mapTransactions[hashGenesis].push_back(tx.second);
            }

            /* Loop transactions map by genesis. */
            for(auto& list : mapTransactions)
            {
                /* Get reference of the vector. */
                std::vector<TAO::Ledger::Transaction>& vtx = list.second;

                /* Sort the list by sequence numbers. */
                std::sort(vtx.begin(), vtx.end());

                /* Add the hashes into list. */
                uint512_t hashLast = 0;

                /* Check last hash for valid transactions. */
                if(!vtx[0].IsFirst())
                {
                    /* Read last hash. */
                    if(!LLD::Ledger->ReadLast(list.first, hashLast))
                        break;

                    /* Check the last hash. */
                    if(vtx[0].hashPrevTx != hashLast)
                    {
                        /* Debug information. */
                        debug::error(FUNCTION, "ROOT ORPHAN: last hash mismatch ", vtx[0].hashPrevTx.SubString());

                        debug::log(3, "REMOVE ------------------------------");

                        /* Disconnect all transactions in reverse order. */
                        for(auto tx = vtx.rbegin(); tx != vtx.rend(); ++tx)
                        {
                            /* Show the removal. */
                            tx->print();

                            /* Reset memory states to disk indexes. */
                            if(!tx->Disconnect(FLAGS::ERASE))
                            {
                                debug::error(FUNCTION, "failed to disconnect tx ", tx->GetHash().SubString());

                                break;
                            }

                            /* Find the transaction in pool. */
                            if(mapLedger.count(tx->GetHash()))
                            {
                                debug::log(0, "DELETED ", tx->GetHash().SubString());

                                /* Erase from the memory map. */
                                mapClaimed.erase(tx->hashPrevTx);
                                mapLedger.erase(tx->GetHash());
                            }
                        }

                        debug::log(3, "END REMOVE ------------------------------");


                        break;
                    }
                }

                /* Set last from next transaction. */
                hashLast = vtx[0].GetHash();

                /* Loop through transaction by genesis. */
                for(uint32_t n = 1; n < vtx.size(); ++n)
                {
                    /* Check for end of index. */
                    if(n == vtx.size())
                        break;

                    /* Check that transaction is in sequence. */
                    if(vtx[n].hashPrevTx != hashLast)
                    {
                        /* Debug information. */
                        debug::error(FUNCTION, "ORPHAN DETECTED INDEX ", n, ": last hash mismatch ", vtx[n].hashPrevTx.SubString());

                        debug::log(3, "REMOVE ------------------------------");

                        /* Begin the memory transaction. */
                        LLD::TxnBegin(FLAGS::MEMPOOL);

                        /* Disconnect all transactions in reverse order. */
                        for(auto tx = vtx.rbegin(); tx != vtx.rend(); ++tx)
                        {
                            tx->print();

                            if(tx->GetHash() == hashLast)
                            {
                                debug::log(0, "REACHED HASH LAST");
                                break;
                            }

                            /* Reset memory states to disk indexes. */
                            if(!tx->Disconnect(FLAGS::MEMPOOL))
                            {
                                LLD::TxnAbort(FLAGS::MEMPOOL);

                                break;
                            }

                            Remove(tx->GetHash());
                        }

                        /* Commit the memory transaction. */
                        LLD::TxnCommit(FLAGS::MEMPOOL);

                        debug::log(3, "END REMOVE ------------------------------");


                        break;
                    }

                    /* Set last hash. */
                    hashLast = vtx[n].GetHash();
                }
            }
        }


        /* List transactions in memory pool. */
        bool Mempool::List(std::vector<uint512_t> &vHashes, uint32_t nCount, bool fLegacy)
        {
            RECURSIVE(MUTEX);

            /* If legacy flag set, skip over getting tritium transactions. */
            if(!fLegacy)
            {
                /* Create map of transactions by genesis. */
                std::map<uint256_t, std::vector<TAO::Ledger::Transaction> > mapTransactions;

                /* Loop through all the transactions. */
                for(const auto& tx : mapLedger)
                {
                    /* Check that this transaction isn't conflicted. */
                    //if(mapConflicts.count(tx.first))
                    //    continue;

                    /* Check that this transaction hasn't been rejected. */
                    if(mapRejected.count(tx.first))
                        continue;

                    /* Cache the genesis. */
                    const uint256_t& hashGenesis = tx.second.hashGenesis;

                    /* Check in map for push back. */
                    if(!mapTransactions.count(hashGenesis))
                        mapTransactions[hashGenesis] = std::vector<TAO::Ledger::Transaction>();

                    /* Push to back of map. */
                    mapTransactions[hashGenesis].push_back(tx.second);
                }

                /* Loop transactions map by genesis. */
                for(auto& list : mapTransactions)
                {
                    /* Get reference of the vector. */
                    std::vector<TAO::Ledger::Transaction>& vtx = list.second;

                    /* Sort the list by sequence numbers. */
                    std::sort(vtx.begin(), vtx.end());

                    /* Add the hashes into list. */
                    uint512_t hashLast = 0;

                    /* Check last hash for valid transactions. */
                    if(!vtx[0].IsFirst())
                    {
                        /* Read last index from disk. */
                        if(!LLD::Ledger->ReadLast(list.first, hashLast))
                            break; //NOTE: this may need an error

                        /* Check the last hash. */
                        if(vtx[0].hashPrevTx != hashLast)
                            break;
                    }

                    /* Set last from next transaction. */
                    hashLast = vtx[0].GetHash();

                    /* Loop through transaction by genesis. */
                    for(uint32_t n = 1; n <= vtx.size(); ++n)
                    {
                        /* Add to the output queue. */
                        vHashes.push_back(hashLast);

                        /* Check for end of index. */
                        if(n == vtx.size())
                            break;

                        /* Check count. */
                        if(--nCount == 0)
                            return true;

                        /* Check that transaction is in sequence. */
                        if(vtx[n].hashPrevTx != hashLast)
                            break; //SKIP ANY ORPHANS FOUND

                        /* Set last hash. */
                        hashLast = vtx[n].GetHash();
                    }
                }
            }
            else
            {
                /* Loop transactions map by genesis. */
                for(const auto& list : mapLegacy)
                {
                    /* Push legacy transactions last. */
                    vHashes.push_back(list.first);;

                    /* Check for end of line. */
                    if(--nCount == 0)
                        return true;
                }
            }

            return vHashes.size() > 0;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::Size()
        {
            RECURSIVE(MUTEX);

            return static_cast<uint32_t>(mapLedger.size() + mapLegacy.size());
        }
    }
}
