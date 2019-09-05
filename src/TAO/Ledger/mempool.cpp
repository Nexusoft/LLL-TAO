/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
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
        : MUTEX()
        , mapLegacy()
        , mapLedger()
        , mapOrphans()
        , mapPrevHashes()
        , mapInputs()
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
            uint512_t hashTx = tx.GetHash();

            RLOCK(MUTEX);

            /* Check the mempool. */
            if(mapLedger.count(hashTx))
                return false;

            /* Add to the map. */
            mapLedger[hashTx] = tx;

            return true;
        }


        /* Accepts a transaction with validation rules. */
        bool Mempool::Accept(TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode)
        {
            /* Check for version 7 activation timestamp. */
            if(!(TAO::Ledger::VersionActive((tx.nTimestamp), 7) || TAO::Ledger::CurrentVersion() > 7))
                return debug::error(FUNCTION, "tritium transaction not accepted until 2 hours after time-lock");

            /* Get the transaction hash. */
            uint512_t hashTx = tx.GetHash();

            /* Runtime calculations. */
            runtime::timer time;
            time.Start();

            /* Get ehe next hash being claimed. */
            uint256_t hashClaim = tx.PrevHash();
            {
                RLOCK(MUTEX);

                /* Check the mempool. */
                if(mapLedger.count(hashTx))
                    return false;

                /* The next hash that is being claimed. */
                if(mapPrevHashes.count(hashClaim))
                    return debug::error(FUNCTION, "trying to claim spent next hash ", hashClaim.SubString());

                /* Check memory and disk for previous transaction. */
                TAO::Ledger::Transaction txPrev;
                if(!tx.IsFirst()
                && !mapLedger.count(tx.hashPrevTx)
                && !LLD::Ledger->ReadTx(tx.hashPrevTx, txPrev))
                {
                    /* Debug output. */
                    debug::log(0, FUNCTION, "tx ", hashTx.SubString(), " ",
                        tx.nSequence, " genesis ", tx.hashGenesis.SubString(),
                        " ORPHAN in ", std::dec, time.ElapsedMilliseconds(), " ms");

                    /* Push to orphan queue. */
                    mapOrphans[tx.hashPrevTx] = tx;

                    /* Ask for the missing transaction. */
                    if(pnode)
                        pnode->PushMessage(uint8_t(LLP::ACTION::GET), uint8_t(LLP::TYPES::TRANSACTION), tx.hashPrevTx);

                    return true;
                }

            }

            //TODO: add mapConflcts map to soft-ban conflicting blocks

            /* Check for duplicate coinbase or coinstake. */
            if(tx.IsCoinBase())
                return debug::error(FUNCTION, "coinbase ", hashTx.SubString(), " not accepted in pool");

            /* Check for duplicate coinbase or coinstake. */
            if(tx.IsCoinStake())
                return debug::error(FUNCTION, "coinstake ", hashTx.SubString(), " not accepted in pool");

            /* Check for duplicate coinbase or coinstake. */
            if(tx.nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " too far in the future");

            /* Check that the transaction is in a valid state. */
            if(!tx.Check())
                return false;

            /* Verify the Ledger Pre-States. */
            if(!tx.Verify())
                return false;

            /* Connect transaction in memory. */
            if(!tx.Connect(TAO::Ledger::FLAGS::MEMPOOL))
                return false;

            {
                RLOCK(MUTEX);

                /* Set the internal memory. */
                mapLedger[hashTx] = tx;
                mapPrevHashes[hashClaim] = hashTx;
            }

            /* Debug output. */
            debug::log(2, FUNCTION, "tx ", hashTx.SubString(), " ACCEPTED in ", std::dec, time.ElapsedMilliseconds(), " ms");

            /* Relay the transaction. */
            if(LLP::TRITIUM_SERVER)
            {
                LLP::TRITIUM_SERVER->Relay
                (
                    LLP::ACTION::NOTIFY,
                    uint8_t(LLP::TYPES::TRANSACTION),
                    hashTx
                );
            }

            /* Check orphan queue. */
            {
                RLOCK(MUTEX);

                while(mapOrphans.count(hashTx))
                {
                    /* Get the transaction from map. */
                    TAO::Ledger::Transaction& txOrphan = mapOrphans[hashTx];

                    /* Get the previous hash. */
                    uint512_t hashThis = txOrphan.GetHash();

                    /* Debug output. */
                    debug::log(0, FUNCTION, "PROCESSING ORPHAN tx ", hashTx.SubString());

                    /* Accept the transaction into memory pool. */
                    if(!Accept(txOrphan))
                    {
                        hashTx = hashThis;

                        debug::log(0, FUNCTION, "ORPHAN tx ", hashTx.SubString(), " REJECTED");

                        continue;
                    }

                    /* Erase the transaction. */
                    mapOrphans.erase(hashTx);

                    /* Set the hashTx. */
                    hashTx = hashThis;
                }
            }

            /* Notify private to produce block if valid. */
            if(config::GetBoolArg("-private"))
                PRIVATE_CONDITION.notify_all();

            return true;
        }


        /* Gets a transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const
        {
            RLOCK(MUTEX);

            /* Fail if not found. */
            if(!mapLedger.count(hashTx))
                return false;

            /* Find the object. */
            tx = mapLedger.at(hashTx);

            return true;
        }


        /* Get by genesis. */
        bool Mempool::Get(const uint256_t& hashGenesis, std::vector<TAO::Ledger::Transaction> &vTx) const
        {
            RLOCK(MUTEX);

            /* Check through the ledger map for the genesis. */
            for(const auto& txMap : mapLedger)
            {
                /* Check for Genesis. */
                if(txMap.second.hashGenesis == hashGenesis)
                    vTx.push_back(txMap.second);

            }

            /* Check that a transaction was found. */
            if(vTx.size() == 0)
                return false;

            /* Sort the list by sequence numbers. */
            std::sort(vTx.begin(), vTx.end());

            /* Check that the mempool transactions are in correct order. */
            uint512_t hashLast = vTx[0].GetHash();
            for(uint32_t n = 1; n < vTx.size(); ++n)
            {
                /* Check that transaction is in sequence. */
                if(vTx[n].hashPrevTx != hashLast)
                {
                    debug::log(0, FUNCTION, "Last hash mismatch");

                    /* Resize to forget about mismatched sequences. */
                    vTx.resize(n);

                    break;
                }

                /* Set last hash. */
                hashLast = vTx[n].GetHash();
            }

            return (vTx.size() > 0);
        }


        /* Gets a transaction by genesis. */
        bool Mempool::Get(const uint256_t& hashGenesis, TAO::Ledger::Transaction &tx) const
        {
            /* Get the list of transactions by genesis. */
            std::vector<TAO::Ledger::Transaction> vTx;
            if(!Get(hashGenesis, vTx))
                return false;

            /* Return last item in list (newest). */
            tx = vTx.back();

            return true;
        }


        /* Checks if a transaction exists. */
        bool Mempool::Has(const uint512_t& hashTx) const
        {
            RLOCK(MUTEX);

            return mapLedger.count(hashTx) || mapLegacy.count(hashTx);
        }


        /* Checks if a genesis exists. */
        bool Mempool::Has(const uint256_t& hashGenesis) const
        {
            RLOCK(MUTEX);

            /* Check through the ledger map for the genesis. */
            for(const auto& tx : mapLedger)
                if(tx.second.hashGenesis == hashGenesis)
                    return true;

            return false;
        }


        /* Remove a transaction from pool. */
        bool Mempool::Remove(const uint512_t& hashTx)
        {
            RLOCK(MUTEX);

            /* Find the transaction in pool. */
            if(mapLedger.count(hashTx))
            {
                TAO::Ledger::Transaction tx = mapLedger[hashTx];

                /* Erase from the memory map. */
                mapPrevHashes.erase(tx.PrevHash());
                mapLedger.erase(hashTx);

                return true;
            }

            /* Find the legacy transaction in pool. */
            if(mapLegacy.count(hashTx))
            {
                Legacy::Transaction tx = mapLegacy[hashTx];

                /* Erase the claimed inputs */
                uint32_t s = static_cast<uint32_t>(tx.vin.size());
                for(uint32_t i = 0; i < s; ++i)
                    mapInputs.erase(tx.vin[i].prevout);

                mapLegacy.erase(hashTx);
            }

            return false;
        }


        /* List transactions in memory pool. */
        bool Mempool::List(std::vector<uint512_t> &vHashes, uint32_t nCount) const
        {
            RLOCK(MUTEX);

            //TODO: need to check dependant transactions and sequence them properly otherwise this will fail

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

            /* Loop transctions map by genesis. */
            for(auto& list : mapTransactions)
            {
                /* Get reference of the vector. */
                std::vector<TAO::Ledger::Transaction>& vTx = list.second;

                /* Sort the list by sequence numbers. */
                std::sort(vTx.begin(), vTx.end());

                /* Add the hashes into list. */
                uint512_t hashLast = vTx[0].GetHash();
                for(uint32_t n = 1; n <= vTx.size(); ++n)
                {
                    /* Add to the output queue. */
                    vHashes.push_back(hashLast);

                    /* Check for end of index. */
                    if(n == vTx.size())
                        break;

                    /* Check count. */
                    if(--nCount == 0)
                        return true;

                    /* Check that transaction is in sequence. */
                    if(vTx[n].hashPrevTx != hashLast)
                    {
                        debug::log(0, FUNCTION, "Last hash mismatch");

                        break;
                    }

                    /* Set last hash. */
                    hashLast = vTx[n].GetHash();
                }
            }

            return vHashes.size() > 0;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::Size()
        {
            RLOCK(MUTEX);

            return static_cast<uint32_t>(mapLedger.size() + mapLegacy.size());
        }
    }
}
