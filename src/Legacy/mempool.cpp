/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <Legacy/include/money.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>

#include <cmath>


/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Add a transaction to the memory pool without validation checks. */
        bool Mempool::AddUnchecked(const Legacy::Transaction& tx)
        {
            /* Get the transaction hash. */
            uint512_t nTxHash = tx.GetHash();

            RLOCK(MUTEX);

            /* Check the mempool. */
            if(mapLegacy.count(nTxHash))
                return false;

            /* Add to the map. */
            mapLegacy[nTxHash] = tx;

            return true;
        }


        bool Mempool::Accept(const Legacy::Transaction& tx, LLP::TritiumNode* pnode)
        {
            /* Get the transaction hash. */
            uint512_t hashTx = tx.GetHash();

            RLOCK(MUTEX);

            /* Check if we already have this tx. */
            if(LLD::Legacy->HasTx(hashTx, FLAGS::MEMPOOL))
                return false;

            debug::log(3, "ACCEPT --------------------------------------");
            if(config::nVerbose >= 3)
                tx.print();
            debug::log(3, "END ACCEPT -----------------------------------");

            /* Check transaction for errors. */
            if(!tx.CheckTransaction())
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " failed");

            /* Coinbase is only valid in a block, not as a loose transaction */
            if(tx.IsCoinBase())
                return debug::error(FUNCTION, "coinbase ", hashTx.SubString(), "as individual tx");

            /* Nexus: coinstake is also only valid in a block, not as a loose transaction */
            if(tx.IsCoinStake())
                return debug::error(FUNCTION, "coinstake ", hashTx.SubString(), " as individual tx");

            /* To help v0.1.5 clients who would see it as a negative number */
            if((uint64_t) tx.nLockTime > std::numeric_limits<int32_t>::max())
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " not accepting nLockTime beyond 2038 yet");

            /* Check previous inputs. */
            for(const auto& vin : tx.vin)
            {
                /* Check if input is already claimed. */
                if(mapInputs.count(vin.prevout))
                {
                    /* Add to conflicts map. */
                    debug::error(FUNCTION, "LEGACY CONFLICT: INPUTS CLAIMED ", vin.prevout.hash.SubString(), ", ", vin.prevout.n);
                    mapLegacyConflicts[hashTx] = tx;

                    return false;
                }
            }

            /* Check the inputs for spends. */
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;

            /* Fetch the inputs. */
            if(!tx.FetchInputs(inputs))
            {
                /* Create response data stream. */
                DataStream ssResponse(SER_NETWORK, LLP::PROTOCOL_VERSION);

                /* Check previous inputs. */
                uint32_t nTotal = 0;
                for(const auto& vin : tx.vin)
                {
                    /* Check for available inputs. */
                    bool fExists  = false;

                    /* Handle for finding inputs (fetch inputs style). */
                    if(tx.nTime >= TAO::Ledger::StartBlockTimelock(7))
                    {
                        /* Get the type of transaction. */
                        if(vin.prevout.hash.GetType() == TAO::Ledger::TRITIUM)
                        {
                            /* Ask for orphan if it wasn't found. */
                            if(LLD::Ledger->HasTx(vin.prevout.hash, FLAGS::MEMPOOL))
                                fExists  = true;
                        }
                    }

                    /* Catch all for if tritium wasn't found. */
                    if(!fExists && LLD::Legacy->HasTx(vin.prevout.hash, FLAGS::MEMPOOL))
                        fExists = true;

                    /* Check for any orphaned inputs. */
                    if(!fExists && setOrphansByIndex.count(vin.prevout.hash))
                    {
                        fExists = true;

                        /* Debug output. */
                        debug::log(2, FUNCTION, "ORPHANED ", vin.prevout.hash.SubString(), " already have");
                    }

                    /* If there are missing inputs, request them and return. */
                    if(!fExists)
                    {
                        /* Debug output. */
                        debug::log(2, FUNCTION, "tx ", hashTx.SubString(), " input ", vin.prevout.hash.SubString(),
                            " ORPHAN");

                        /* Set orphan flag. */
                        ++nTotal;

                        /* Push to stream: NOTE: we need to ask for both types here since we don't have any good way to determine with */
                        ssResponse << uint8_t(LLP::TYPES::TRANSACTION) << vin.prevout.hash; // 100% certainty what input tx type is
                        ssResponse << uint8_t(LLP::SPECIFIER::LEGACY) << uint8_t(LLP::TYPES::TRANSACTION) << vin.prevout.hash;

                        /* Debug output. */
                        debug::log(2, FUNCTION, "Requesting missing tx ", vin.prevout.hash.SubString());
                    }
                }

                /* We want to process all orphaned inputs before returning. */
                if(nTotal > 0)
                {
                    /* Increment consecutive orphans. */
                    if(pnode)
                    {
                        /* Push to stream. */
                        ssResponse << uint8_t(LLP::SPECIFIER::LEGACY) << uint8_t(LLP::TYPES::TRANSACTION) << hashTx;

                        /* Push the packet response. */
                        pnode->WritePacket(LLP::TritiumNode::NewMessage(LLP::ACTION::GET, ssResponse));

                        /* Update consecutive orphans. */
                        ++pnode->nConsecutiveOrphans;
                    }

                    return debug::error(FUNCTION, "tx ", hashTx.SubString(), " missing ", nTotal, " inputs");
                }

                return debug::error(FUNCTION, "failed to fetch inputs");
            }

            /* Check for standard inputs. */
            if(!tx.AreInputsStandard(inputs))
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " inputs are non-standard");

            /* Check the transaction fees. */
            uint64_t nFees = tx.GetValueIn(inputs) - tx.GetValueOut();
            uint32_t nSize = ::GetSerializeSize(tx, SER_NETWORK, LLP::PROTOCOL_VERSION);

            /* Don't accept if the fees are too low. */
            if(nFees < tx.GetMinFee(1000, false))
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " not enough fees");

            /* Rate limit free transactions to prevent penny flooding attacks. */
            if(nFees < Legacy::MIN_RELAY_TX_FEE)
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
                if(dFreeCount > config::GetArg("-limitfreerelay", 15) * 10 * 1000)
                    return debug::error(FUNCTION, "free transaction rejected by rate limiter");

                debug::log(2, FUNCTION, "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
                dFreeCount += nSize;
            }

            /* See if inputs can be connected. */
            TAO::Ledger::BlockState state = ChainState::stateBest.load();
            if(!tx.Connect(inputs, state, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "tx ", hashTx.SubString(), " failed to connect inputs");

            /* Set the inputs to be claimed. */
            uint32_t s = tx.vin.size();
            for(uint32_t i = 0; i < s; ++i)
                mapInputs[tx.vin[i].prevout] = hashTx;

            /* Add to the legacy map. */
            mapLegacy[hashTx] = tx;

            /* Log outputs. */
            debug::log(2, FUNCTION, "tx ", hashTx.SubString(), " ACCEPTED");

            return true;
        }


        /* Checks if a given output is spent in memory. */
        bool Mempool::IsSpent(const uint512_t& hash, const uint32_t n)
        {
            return mapInputs.count(Legacy::OutPoint(hash, n));
        }

        /* Gets a legacy transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, Legacy::Transaction &tx, bool &fConflicted) const
        {
            RLOCK(MUTEX);

            /* Check in conflict memory. */
            if(mapLegacyConflicts.count(hashTx))
            {
                /* Get from conflicts map. */
                tx = mapLegacyConflicts.at(hashTx);
                fConflicted = true;

                debug::log(0, FUNCTION, "CONFLICTED TRANSACTION: ", hashTx.SubString());

                return true;
            }

            /* Check the memory map. */
            if(mapLegacy.count(hashTx))
            {
                /* Get the transaction from memory. */
                tx = mapLegacy.at(hashTx);

                return true;
            }

            return false;
        }

        /* Gets a legacy transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, Legacy::Transaction &tx) const
        {
            RLOCK(MUTEX);

            /* Check the memory map. */
            if(!mapLegacy.count(hashTx))
                return false;

            /* Get the transaction from memory. */
            tx = mapLegacy.at(hashTx);

            return true;
        }


        /* Gets the size of the memory pool. */
        uint32_t Mempool::SizeLegacy()
        {
            RLOCK(MUTEX);

            return mapLegacy.size();
        }

    }
}
