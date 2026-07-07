/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/stagepool.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <map>
#include <mutex>
#include <utility>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Stage pool namespace. */
        namespace StagePool
        {
            /* Mutex protecting both maps below. */
            static std::mutex STAGE_MUTEX;


            /* Txids wanted by incomplete blocks, keyed to registration time. */
            static std::map<uint512_t, uint64_t> mapWanted;


            /* Staged transactions keyed by txid, valued with staging time. */
            static std::map<uint512_t, std::pair<TAO::Ledger::Transaction, uint64_t>> mapStaged;


            /* Internal: check TTL expiry for a stored timestamp. Caller holds lock. */
            static bool expired(const uint64_t nTimestamp)
            {
                return (runtime::timestamp() > nTimestamp + STAGE_TTL_SECONDS);
            }


            /* Mark a transaction hash as wanted by an incomplete block. */
            void Register(const uint512_t& hashTx)
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                /* Bound the map size before inserting a new entry; clearing all
                 * at once is the same intentional cheap DoS guard used by
                 * mapLastMissing (see process.h). */
                if(!mapWanted.count(hashTx) && mapWanted.size() >= MAX_WANTED_ENTRIES)
                    mapWanted.clear();

                mapWanted[hashTx] = runtime::timestamp();
            }


            /* Check whether a transaction hash is wanted by an incomplete block. */
            bool Wanted(const uint512_t& hashTx)
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                const auto it = mapWanted.find(hashTx);
                if(it == mapWanted.end())
                    return false;

                /* TTL eviction on access. */
                if(expired(it->second))
                {
                    mapWanted.erase(it);
                    return false;
                }

                return true;
            }


            /* Stage a transaction for block-context validation. */
            bool Stage(const TAO::Ledger::Transaction& tx)
            {
                /* Cache the hash before taking the lock (hashing is expensive). */
                const uint512_t hashTx = tx.GetHash();

                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                /* Only stage transactions previously registered by an incomplete
                 * block, so arbitrary policy-rejected txs can't accumulate here. */
                const auto itWanted = mapWanted.find(hashTx);
                if(itWanted == mapWanted.end() || expired(itWanted->second))
                    return false;

                /* Bound the staged map size before inserting a new entry. */
                if(!mapStaged.count(hashTx) && mapStaged.size() >= MAX_STAGED_ENTRIES)
                    mapStaged.clear();

                mapStaged[hashTx] = std::make_pair(tx, runtime::timestamp());

                return true;
            }


            /* Retrieve a staged transaction by hash. */
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction& tx)
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                const auto it = mapStaged.find(hashTx);
                if(it == mapStaged.end())
                    return false;

                /* TTL eviction on access. */
                if(expired(it->second.second))
                {
                    mapStaged.erase(it);
                    return false;
                }

                tx = it->second.first;

                return true;
            }


            /* Remove a transaction from both maps. */
            void Erase(const uint512_t& hashTx)
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                mapWanted.erase(hashTx);
                mapStaged.erase(hashTx);
            }


            /* Number of currently staged transactions. */
            uint64_t Count()
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                return mapStaged.size();
            }


            /* Remove all wanted and staged entries. */
            void Clear()
            {
                std::unique_lock<std::mutex> lock(STAGE_MUTEX);

                mapWanted.clear();
                mapStaged.clear();
            }
        }
    }
}
