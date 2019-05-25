/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/packets/tritium.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/constants.h>
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
        bool Mempool::Accept(TAO::Ledger::Transaction& tx)
        {
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
                    return debug::error(FUNCTION, "trying to claim spent next hash ", hashClaim.ToString().substr(0, 20));

                /* Check memory and disk for previous transaction. */
                TAO::Ledger::Transaction txPrev;
                if(!tx.IsFirst()
                && !mapLedger.count(tx.hashPrevTx)
                && !LLD::legDB->ReadTx(tx.hashPrevTx, txPrev))
                {
                    /* Check for max orphan queue. */
                    if(mapOrphans.size() > 1000)
                    {
                        /* Clear the orphans map. */
                        mapOrphans.clear();

                        return false;
                    }

                    /* Debug output. */
                    debug::log(2, FUNCTION, "tx ", hashTx.ToString().substr(0, 20), " ", tx.nSequence, " ORPHAN in ", std::dec, time.ElapsedMilliseconds(), " ms");

                    /* Push to orphan queue. */
                    mapOrphans[tx.hashPrevTx] = tx;

                    return true;
                }

                //TODO: add mapConflcts map to soft-ban conflicting blocks
            }

            /* Check for duplicate coinbase or coinstake. */
            if(tx.IsCoinbase())
                return debug::error(FUNCTION, "coinbase ", hashTx.ToString().substr(0, 20), " not accepted in pool");

            /* Check for duplicate coinbase or coinstake. */
            if(tx.IsCoinstake())
                return debug::error(FUNCTION, "coinstake ", hashTx.ToString().substr(0, 20), " not accepted in pool");

            /* Check for duplicate coinbase or coinstake. */
            if(tx.nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "tx ", hashTx.ToString().substr(0, 20), " too far in the future");

            /* Check that the transaction is in a valid state. */
            if(!tx.IsValid(TAO::Register::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, hashTx.ToString().substr(0, 20), " is invalid");

            /* Verify the Ledger Pre-States. */
            if(!TAO::Register::Verify(tx, TAO::Register::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, hashTx.ToString().substr(0, 20), " register verification failed");

            /* Calculate the future potential states. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, hashTx.ToString().substr(0, 20), " operations execution failed");

            /* Add to the map. */
            {
                RLOCK(MUTEX);

                /* Set the internal memory. */
                mapLedger[hashTx] = tx;
                mapPrevHashes[hashClaim] = hashTx;
            }

            /* Debug output. */
            debug::log(2, FUNCTION, "tx ", hashTx.ToString().substr(0, 20), " ACCEPTED in ", std::dec, time.ElapsedMilliseconds(), " ms");

            /* Relay the transaction. */
            std::vector<LLP::CInv> vInv = { LLP::CInv(hashTx, LLP::MSG_TX_TRITIUM) };
            if(LLP::TRITIUM_SERVER)
                LLP::TRITIUM_SERVER->Relay(LLP::DAT_INVENTORY, vInv);

            /* Process orphans. */
            { RLOCK(MUTEX);

                /* Check orphan queue. */
                while(mapOrphans.count(hashTx))
                {
                    /* Get the transaction from map. */
                    TAO::Ledger::Transaction& txOrphan = mapOrphans[hashTx];

                    /* Get the previous hash. */
                    uint512_t hashThis = txOrphan.GetHash();

                    /* Debug output. */
                    debug::log(2, FUNCTION, "PROCESSING ORPHAN tx ", hashTx.ToString().substr(0, 20));

                    /* Accept the transaction into memory pool. */
                    if(!Accept(txOrphan))
                        return true;

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

            /* Sort the list by sequence numbers. */
            std::sort(vTx.begin(), vTx.end());

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
                for (uint32_t i = 0; i < s; ++i)
                    mapInputs.erase(tx.vin[i].prevout);

                mapLegacy.erase(hashTx);
            }

            return false;
        }


        /* List transactions in memory pool. */
        bool Mempool::List(std::vector<uint512_t> &vHashes, uint32_t nCount) const
        {
            RLOCK(MUTEX);

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
                for(const auto& tx : vTx)
                {
                    /* Add to the output queue. */
                    vHashes.push_back(tx.GetHash());

                    /* Decrement counter. */
                    --nCount;

                    /* Check count. */
                    if(nCount == 0)
                        return true;
                }
            }

            return vHashes.size() > 1;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::Size()
        {
            RLOCK(MUTEX);

            return static_cast<uint32_t>(mapLedger.size() + mapLegacy.size());
        }
    }
}
