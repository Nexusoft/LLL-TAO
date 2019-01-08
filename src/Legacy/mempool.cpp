/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/mempool.h>

#include <Util/include/args.h>

namespace TAO::Ledger
{

    bool Mempool::Accept(Legacy::Transaction tx)
    {
        LOCK(MUTEX);

        /* Check transaction for errors. */
        if (!tx.CheckTransaction())
            return debug::error(FUNCTION "CheckTransaction failed");

        /* Coinbase is only valid in a block, not as a loose transaction */
        if (tx.IsCoinBase())
            return debug::error(FUNCTION "coinbase as individual tx");

        /* Nexus: coinstake is also only valid in a block, not as a loose transaction */
        if (tx.IsCoinStake())
            return debug::error(FUNCTION "coinstake as individual tx");

        /* To help v0.1.5 clients who would see it as a negative number */
        if ((uint64_t) tx.nLockTime > std::numeric_limits<int32_t>::max())
            return debug::error(FUNCTION "not accepting nLockTime beyond 2038 yet");

        /* Rather not work on nonstandard transactions (unless -testnet) */
        if (!config::fTestNet && !tx.IsStandard())
            return debug::error(FUNCTION "nonstandard transaction type");

        /* Check previous inputs. */
        for (auto vin : tx.vin)
        {
            if (mapInputs.count(vin.prevout))
                return debug::error(FUNCTION "double spend attempt on inputs");
        }

        /* Set the inputs to be claimed. */
        uint32_t s = tx.vin.size();
        for (uint32_t i = 0; i < s; ++i)
            mapInputs[tx.vin[i].prevout] = Legacy::CInPoint(&tx, i);

        return true;
    }
}
