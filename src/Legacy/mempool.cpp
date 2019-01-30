/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cmath>

#include <LLC/types/uint1024.h>

#include <Legacy/include/money.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Add a transaction to the memory pool without validation checks. */
        bool Mempool::AddUnchecked(Legacy::Transaction tx)
        {
            LOCK(MUTEX);

            /* Get the transaction hash. */
            uint512_t hash = tx.GetHash();

            /* Check the mempool. */
            if(mapLegacy.count(hash))
                return debug::error(FUNCTION, "%s already exists ", tx.GetHash().ToString().substr(0, 20).c_str());

            /* Add to the map. */
            mapLegacy[hash] = tx;

            return true;
        }


        bool Mempool::Accept(Legacy::Transaction tx)
        {
            LOCK(MUTEX);

            /* Check transaction for errors. */
            if (!tx.CheckTransaction())
                return debug::error(FUNCTION, "CheckTransaction failed");

            /* Coinbase is only valid in a block, not as a loose transaction */
            if (tx.IsCoinBase())
                return debug::error(FUNCTION, "coinbase as individual tx");

            /* Nexus: coinstake is also only valid in a block, not as a loose transaction */
            if (tx.IsCoinStake())
                return debug::error(FUNCTION, "coinstake as individual tx");

            /* To help v0.1.5 clients who would see it as a negative number */
            if ((uint64_t) tx.nLockTime > std::numeric_limits<int32_t>::max())
                return debug::error(FUNCTION, "not accepting nLockTime beyond 2038 yet");

            /* Rather not work on nonstandard transactions (unless -testnet) */
            if (!config::fTestNet && !tx.IsStandard())
                return debug::error(FUNCTION, "nonstandard transaction type");

            /* Check previous inputs. */
            for (auto vin : tx.vin)
            {
                if (mapInputs.count(vin.prevout))
                    return debug::error(FUNCTION, "double spend attempt on inputs");
            }

            /* Check the inputs for spends. */
            std::map<uint512_t, Legacy::Transaction> inputs;

            /* Fetch the inputs. */
            if(!tx.FetchInputs(inputs))
                return debug::error(FUNCTION, "failed to fetch the inputs");

            /* Check for standard inputs. */
            if(!tx.AreInputsStandard(inputs))
                return debug::error(FUNCTION, "inputs are non-standard");

            /* Check the transaction fees. */
            uint64_t nFees = tx.GetValueIn(inputs) - tx.GetValueOut();
            uint32_t nSize = ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);

            /* Don't accept if the fees are too low. */
            if (nFees < tx.GetMinFee(1000, false))
                return debug::error(FUNCTION, "not enough fees");

            /* Rate limit free transactions to prevent penny flooding attacks. */
            if (nFees < Legacy::MIN_RELAY_TX_FEE)
            {
                /* Static values to keep track of last tx's. */
                static double dFreeCount;
                static uint64_t nLastTime;

                /* Get the current time. */
                uint64_t nNow = runtime::unifiedtimestamp();

                // Use an exponentially decaying ~10-minute window:
                dFreeCount *= pow(1.0 - 1.0/600.0, (double)(nNow - nLastTime));
                nLastTime = nNow;

                // -limitfreerelay unit is thousand-bytes-per-minute
                // At default rate it would take over a month to fill 1GB
                if (dFreeCount > config::GetArg("-limitfreerelay", 15) * 10 * 1000)
                    return debug::error(FUNCTION, "free transaction rejected by rate limiter");

                debug::log(2, FUNCTION, "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
                dFreeCount += nSize;
            }

            /* See if inputs can be connected. */
            if(!tx.Connect(inputs, ChainState::stateBest, Legacy::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "failed to connect inputs");

            /* Set the inputs to be claimed. */
            uint32_t s = tx.vin.size();
            for (uint32_t i = 0; i < s; ++i)
                mapInputs[tx.vin[i].prevout] = tx.GetHash();

            /* Add to the legacy map. */
            mapLegacy[tx.GetHash()] = tx;

            /* Log outputs. */
            debug::log(2, FUNCTION, tx.GetHash().ToString().substr(0, 20), " ACCEPTED");

            return true;
        }

        /* Gets a legacy transaction from mempool */
        bool Mempool::Get(uint512_t hashTx, Legacy::Transaction& tx) const
        {
            LOCK(MUTEX);

            /* Check the memory map. */
            if(!mapLegacy.count(hashTx))
                return false;

            /* Get the transaction from memory. */
            tx = mapLegacy.at(hashTx);

            return true;
        }


        /* Checks if a legacy transaction exists. */
        bool Mempool::HasLegacy(uint512_t hashTx) const
        {
            LOCK(MUTEX);

            return mapLegacy.count(hashTx);
        }


        /* Remove a legacy transaction from pool. */
        bool Mempool::RemoveLegacy(uint512_t hashTx)
        {
            LOCK(MUTEX);

            if(mapLegacy.count(hashTx))
            {
                Legacy::Transaction tx = mapLegacy[hashTx];

                /* Erase the claimed inputs */
                uint32_t s = tx.vin.size();
                for (uint32_t i = 0; i < s; ++i)
                    mapInputs.erase(tx.vin[i].prevout);

                mapLegacy.erase(hashTx);

                return true;
            }

            return false;
        }


        /* List legacy transactions in memory pool. */
        bool Mempool::ListLegacy(std::vector<uint512_t> &vHashes, uint32_t nCount) const
        {
            LOCK(MUTEX);

            for(auto it = mapLegacy.begin(); it != mapLegacy.end() && nCount > 0; it++)
            {

                vHashes.push_back(it->first);
                --nCount;
            }

            return vHashes.size() > 1;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::SizeLegacy()
        {
            return mapLegacy.size();
        }

    }
}
