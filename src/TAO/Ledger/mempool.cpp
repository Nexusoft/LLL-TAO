/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

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

        /* Add a transaction to the memory pool without validation checks. */
        bool Mempool::AddUnchecked(TAO::Ledger::Transaction tx)
        {
            LOCK(MUTEX);

            /* Get the transaction hash. */
            uint512_t hash = tx.GetHash();

            /* Check the mempool. */
            if(mapLedger.count(hash))
                return false;

            /* Add to the map. */
            mapLedger[hash] = tx;

            return true;
        }


        /* Accepts a transaction with validation rules. */
        bool Mempool::Accept(TAO::Ledger::Transaction tx)
        {
            LOCK(MUTEX);

            /* Runtime calculations. */
            runtime::timer time;
            time.Start();

            /* Check the mempool. */
            uint512_t hash = tx.GetHash();
            if(mapLedger.count(hash))
                return false;

            /* The next hash that is being claimed. */
            uint256_t hashClaim = tx.PrevHash();
            if(mapPrevHashes.count(hashClaim))
                return debug::error(FUNCTION, "trying to claim spent next hash ", hashClaim.ToString().substr(0, 20));

            /* Coinbase is only valid in a block, not as a loose transaction */
            if (tx.IsCoinbase())
                return debug::error(FUNCTION, "coinbase ", tx.GetHash().ToString().substr(0, 20), "as individual tx");

            /* Nexus: coinstake is also only valid in a block, not as a loose transaction */
            if (tx.IsTrust())
                return debug::error(FUNCTION, "coinstake ", tx.GetHash().ToString().substr(0, 20), " as individual tx");

            /* Check for duplicate coinbase or coinstake. */
            if(tx.nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "tx ", tx.GetHash().ToString().substr(0, 20), " too far in the future");

            /* Check that the transaction is in a valid state. */
            if(!tx.IsValid())
                return debug::error(FUNCTION, tx.GetHash().ToString().substr(0, 20), " is invalid");

            /* Verify the Ledger Pre-States. */
            if(!TAO::Register::Verify(tx))
                return debug::error(FUNCTION, tx.GetHash().ToString().substr(0, 20), " register verification failed");

            /* Calculate the future potential states. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, tx.GetHash().ToString().substr(0, 20), " operations execution failed");

            /* Add to the map. */
            mapLedger[hash] = tx;
            mapPrevHashes[hashClaim] = tx.GetHash();

            /* Debug output. */
            debug::log(2, FUNCTION, "tx ", hash.ToString().substr(0, 20), " ACCEPTED in ", std::dec, time.ElapsedMilliseconds(), " ms");

            /* Notify private to produce block if valid. */
            if(config::GetBoolArg("-private"))
                PRIVATE_CONDITION.notify_all();

            return true;
        }


        /* Gets a transaction from mempool */
        bool Mempool::Get(uint512_t hashTx, TAO::Ledger::Transaction& tx) const
        {
            LOCK(MUTEX);

            /* Fail if not found. */
            if(!mapLedger.count(hashTx))
                return false;

            /* Find the object. */
            tx = mapLedger.at(hashTx);

            return true;
        }


        /* Checks if a transaction exists. */
        bool Mempool::Has(uint512_t hashTx) const
        {
            LOCK(MUTEX);

            return mapLedger.count(hashTx);
        }


        /* Remove a transaction from pool. */
        bool Mempool::Remove(uint512_t hashTx)
        {
            LOCK(MUTEX);

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
            LOCK(MUTEX);

            for(auto it = mapLedger.begin(); it != mapLedger.end() && nCount > 0; it++)
            {

                vHashes.push_back(it->first);
                --nCount;
            }

            return vHashes.size() > 1;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::Size()
        {
            return static_cast<uint32_t>(mapLedger.size() + mapLegacy.size());
        }
    }
}
