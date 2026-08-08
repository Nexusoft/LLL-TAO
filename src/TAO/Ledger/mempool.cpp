/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>

#include <LLD/include/global.h>

#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/include/admissibility.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <TAO/Ledger/include/create.h>

#include <Util/include/config.h>
#include <Util/include/runtime.h>

#include <limits>


/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {

        Mempool mempool;

        /** Default Constructor. **/
        Mempool::Mempool()
        : MUTEX                       ( )
        , mapLegacy                   ( )
        , mapLegacyConflicts          ( )
        , mapLedger                   ( )
        , mapConflicts                ( )
        , mapConflictRootByGenesis    ( )
        , mapConflictDependents       ( )
        , mapConflictDependentsByIndex( )
        , mapOrphans                  ( )
        , mapRequestCount             ( )
        , mapClaimed                  ( )
        , mapRejected                 ( )
        , mapInputs                   ( )
        , setOrphansByIndex           ( )
        , mapOrphansByIndex           ( )
        {
        }


        /** Default Destructor. **/
        Mempool::~Mempool()
        {
        }


        /* Bounds the whole conflict DAG (roots + dependents + indexes). */
        void Mempool::BoundConflictDAG()
        {
            /* Clear the entire DAG once either cap is hit rather than doing LRU
             * eviction; matches the existing cheap DoS-guard pattern used for
             * mapLastMissing/mapCheckRejects. Any conflict still relevant will
             * simply be re-added the next time the transaction is relayed. */
            const bool fRootsFull =
                (mapConflicts.size() >= MAX_CONFLICTS_MAP_ENTRIES);
            const bool fDepsFull  =
                (mapConflictDependentsByIndex.size() >= MAX_CONFLICT_DEPENDENTS);

            if(fRootsFull || fDepsFull)
            {
                debug::warning(FUNCTION,
                    "conflict DAG cap reached (roots=", mapConflicts.size(),
                    "/", MAX_CONFLICTS_MAP_ENTRIES,
                    " dependents=", mapConflictDependentsByIndex.size(),
                    "/", MAX_CONFLICT_DEPENDENTS,
                    "); clearing DAG to bound memory use");

                mapConflicts.clear();
                mapConflictRootByGenesis.clear();
                mapConflictDependents.clear();
                mapConflictDependentsByIndex.clear();
                mapConflictRetries.clear();
                setStrandedGeneses.clear();
                mapUnknownAncestorRetries.clear();
                setUnknownAncestorGeneses.clear();
            }
        }


        /* Insert tx as a conflict ROOT and maintain per-genesis earliest index. */
        void Mempool::AddConflictRoot(const TAO::Ledger::Transaction& tx)
        {
            const uint512_t hashTx = tx.GetHash();

            /* Already tracked as a root — nothing to do. */
            if(mapConflicts.count(hashTx))
                return;

            BoundConflictDAG();

            mapConflicts[hashTx] = tx;

            /* Maintain earliest-sequence root per genesis. */
            const auto itGenesis = mapConflictRootByGenesis.find(tx.hashGenesis);
            if(itGenesis == mapConflictRootByGenesis.end())
            {
                mapConflictRootByGenesis[tx.hashGenesis] = hashTx;
            }
            else
            {
                const auto itRoot = mapConflicts.find(itGenesis->second);
                if(itRoot == mapConflicts.end() || tx.nSequence < itRoot->second.nSequence)
                    itGenesis->second = hashTx;
            }
        }


        /* Erase one conflict root and repair the per-genesis index. */
        void Mempool::EraseConflictRoot(const uint512_t& hashTx)
        {
            const auto it = mapConflicts.find(hashTx);
            if(it == mapConflicts.end())
                return;

            const uint256_t hashGenesis = it->second.hashGenesis;
            mapConflicts.erase(it);

            /* Repair per-genesis earliest-root index. */
            const auto itGenesis = mapConflictRootByGenesis.find(hashGenesis);
            if(itGenesis == mapConflictRootByGenesis.end())
                return;

            if(itGenesis->second != hashTx && mapConflicts.count(itGenesis->second))
                return; /* index still points at a live root */

            /* Re-scan remaining roots for this genesis (usually tiny). */
            uint512_t hashBest = 0;
            uint32_t  nBestSeq = std::numeric_limits<uint32_t>::max();
            bool fFound = false;

            for(const auto& entry : mapConflicts)
            {
                if(entry.second.hashGenesis != hashGenesis)
                    continue;

                if(!fFound || entry.second.nSequence < nBestSeq)
                {
                    fFound   = true;
                    nBestSeq = entry.second.nSequence;
                    hashBest = entry.first;
                }
            }

            if(fFound)
                itGenesis->second = hashBest;
            else
                mapConflictRootByGenesis.erase(itGenesis);
        }


        /* Drop parked dependents hanging off hashParent (tail only). */
        void Mempool::DropConflictDependents(const uint512_t& hashParent)
        {
            uint512_t hashCur = hashParent;
            while(mapConflictDependents.count(hashCur))
            {
                const TAO::Ledger::Transaction txChild =
                    mapConflictDependents[hashCur];
                const uint512_t hashChild = txChild.GetHash();

                mapConflictDependents.erase(hashCur);
                mapConflictDependentsByIndex.erase(hashChild);

                hashCur = hashChild;
            }
        }


        /* Drop a root and its parked dependent chain. */
        void Mempool::DropConflictTree(const uint512_t& hashRoot)
        {
            EraseConflictRoot(hashRoot);
            DropConflictDependents(hashRoot);
        }


        /* Soft-park a descendant of a conflicted/dependent parent. */
        bool Mempool::ParkConflictDependent(const TAO::Ledger::Transaction& tx)
        {
            const uint512_t hashTx = tx.GetHash();

            /* Already parked by hash. */
            if(mapConflictDependentsByIndex.count(hashTx))
                return true;

            BoundConflictDAG();

            /* Capacity clear may have wiped the parent root/dependent that
             * justified parking. Refuse to create a detached dependent with no
             * Check() reconciliation trigger; peers can re-offer after the next
             * conflict classification. */
            if(!IsConflictNode(tx.hashPrevTx))
                return false;

            /* One child slot per parent (orphan-queue shape). Keep the
             * earliest-sequence occupant; drop later contenders silently. */
            const auto itParent = mapConflictDependents.find(tx.hashPrevTx);
            if(itParent != mapConflictDependents.end())
            {
                if(itParent->second.GetHash() == hashTx)
                    return true;

                if(tx.nSequence >= itParent->second.nSequence)
                {
                    debug::log(3, FUNCTION, "drop later conflict-dependent ",
                        hashTx.SubString(), " prev ", tx.hashPrevTx.SubString(),
                        " (slot held by earlier seq)");
                    return false;
                }

                /* Replace later occupant with earlier-sequence child. Drop the
                 * displaced child's entire tail first so grandchildren cannot
                 * remain keyed under a hash that is no longer reachable. */
                const uint512_t hashDisplaced = itParent->second.GetHash();
                DropConflictDependents(hashDisplaced);
                mapConflictDependentsByIndex.erase(hashDisplaced);
            }

            mapConflictDependents[tx.hashPrevTx] = tx;
            mapConflictDependentsByIndex[hashTx] = tx;

            debug::log(2, FUNCTION, "parked conflict-dependent ",
                hashTx.SubString(), " prev ", tx.hashPrevTx.SubString(),
                " genesis ", tx.hashGenesis.SubString());

            return true;
        }


        /* True when hashTx is a conflict root OR a parked dependent. */
        bool Mempool::IsConflictNode(const uint512_t& hashTx) const
        {
            return mapConflicts.count(hashTx) ||
                   mapConflictDependentsByIndex.count(hashTx);
        }


        /* Re-Accept parked dependents after a parent clears. */
        void Mempool::ProcessConflictDependents(const uint512_t& hashParent)
        {
            uint512_t hashTx = hashParent;
            while(mapConflictDependents.count(hashTx))
            {
                const TAO::Ledger::Transaction tx = mapConflictDependents[hashTx];
                const uint512_t hashThis = tx.GetHash();

                /* Detach before Accept so Accept cannot see a stale self-entry. */
                mapConflictDependents.erase(hashTx);
                mapConflictDependentsByIndex.erase(hashThis);

                debug::log(0, FUNCTION, "PROCESSING CONFLICT-DEPENDENT tx ",
                    hashThis.SubString());

                /* Already live — continue walking any further tail. */
                if(mapLedger.count(hashThis))
                {
                    hashTx = hashThis;
                    continue;
                }

                tx.hashCache = hashThis;

                if(!Accept(tx))
                {
                    debug::log(0, FUNCTION, "CONFLICT-DEPENDENT tx ",
                        hashThis.SubString(), " not re-admitted: ",
                        debug::GetLastError());
                    /* Drop any remaining parked tail under hashThis so it
                     * cannot sit unreachable until BoundConflictDAG clears.
                     * Peers can re-offer the chain after the root recovers. */
                    DropConflictDependents(hashThis);
                    return;
                }

                hashTx = hashThis;
            }
        }


        /* Clear DEFERRED/UNKNOWN retry + diagnostic state for a genesis. */
        void Mempool::ClearGenesisConflictState(const uint256_t& hashGenesis)
        {
            mapConflictRetries.erase(hashGenesis);
            setStrandedGeneses.erase(hashGenesis);
            mapUnknownAncestorRetries.erase(hashGenesis);
            setUnknownAncestorGeneses.erase(hashGenesis);
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

            /* If Accept detaches this tx from the conflict-dependent index and
             * then fails later, drop any tail still keyed under hashTx so it
             * cannot remain unreachable. Disarmed on successful mapLedger
             * insert (ProcessConflictDependents drains the tail instead). */
            struct ConflictDepTailGuard
            {
                Mempool*    pPool;
                uint512_t   hash;
                bool        fActive;

                ConflictDepTailGuard(Mempool* p, const uint512_t& h)
                : pPool(p), hash(h), fActive(false) { }

                ~ConflictDepTailGuard()
                {
                    if(fActive && pPool)
                        pPool->DropConflictDependents(hash);
                }

                void Arm()   { fActive = true;  }
                void Disarm(){ fActive = false; }
            } depTailGuard(this, hashTx);

            try
            {
                /* Reset our request count if we received it. */
                if(mapRequestCount.count(hashTx))
                    mapRequestCount[hashTx] = 0;

                /* Check for transaction on disk. */
                if(mapLedger.count(hashTx))
                    return false; //NOTE: this was true, but changed to false to prevent relay loops in tritium LLP

                /* Check for rejected tx. */
                if(mapRejected.count(tx.hashPrevTx))
                {
                    mapRejected.insert(hashTx);
                    return false;
                    //return debug::error(FUNCTION, "part of rejected transaction orphan chain");
                }

                /* If we are already an ORPHAN, skip over. */
                if(mapOrphans.count(tx.hashPrevTx))
                    return true;

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

                /* Check for duplicate coinbase or coinstake. */
                if(tx.IsHybrid())
                {
                    mapRejected.insert(hashTx);
                    return debug::error(FUNCTION, "hybrid ", hashTx.SubString(), " not accepted in pool");
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
                        mapOrphansByIndex[hashTx] = tx;

                        /* Increment consecutive orphans. */
                        if(pnode)
                            ++pnode->nConsecutiveOrphans;

                        /* Ask for our previous transaction now. */
                        if(LLP::TRITIUM_SERVER)
                        {
                            /* Get a random node in case we have an unreliable node that gave us an ORPHAN */
                            std::shared_ptr<LLP::TritiumNode> pCheck =
                                LLP::TRITIUM_SERVER->RandomConnection();

                            /* Ask the random node for our orphan data. */
                            pCheck->PushMessage(LLP::TritiumNode::ACTION::GET, uint8_t(LLP::TritiumNode::TYPES::TRANSACTION), tx.hashPrevTx);
                        }

                        return false;
                    }

                    /* True double-spend against a live mempool tip: another
                     * in-pool transaction already claims this hashPrevTx.
                     * Option C: this is a ROOT conflict (direct tip disagreement). */
                    if(mapClaimed.count(tx.hashPrevTx))
                    {
                        /* We only need to output debug info and insert if this is a new conflict.
                         * [B2] Matches upstream Nexusoft/LLL-TAO: avoids repeated ERROR-level log
                         * spam for a conflict that has already been recorded, and relays the
                         * conflicted transaction so peers (and our own re-sync logic) can resolve
                         * it once the fork/reorg settles instead of it becoming a silent dead end. */
                        if(!mapConflicts.count(hashTx))
                        {
                            debug::error(FUNCTION, "CONFLICT: prev tx CLAIMED ", tx.hashPrevTx.SubString());
                            AddConflictRoot(tx);

                            /* Relay the conflict if we are running over tritium protocol. */
                            if(pnode && LLP::TRITIUM_SERVER)
                            {
                                LLP::TRITIUM_SERVER->Relay
                                (
                                    LLP::TritiumNode::ACTION::NOTIFY,
                                    uint8_t(LLP::TritiumNode::TYPES::TRANSACTION),
                                    hashTx
                                );
                            }
                        }

                        return false;
                    }

                    /* Predecessor is itself a conflict DAG node (root OR parked
                     * dependent). Option C:
                     *
                     * Historical behavior cascaded every descendant into
                     * mapConflicts with an ERROR log + NOTIFY relay. That made a
                     * handful of seed conflicts (visible as mempool_conflicts=N
                     * on BESTCHAIN while mempool_size=0) explode into a multi-
                     * second ERROR flood when peers offered long sigchain tails,
                     * and the relay amplified the same storm across the mesh.
                     * Best-chain advancement is unaffected (conflicts are
                     * mempool-only), which is why operators see the node "power
                     * through" the spam while BESTCHAIN keeps moving — and why a
                     * restart (which wipes the DAG) clears the symptom.
                     *
                     * Correct handling:
                     *  1. If the conflicted/dependent predecessor is now confirmed
                     *     on disk, drop the stale DAG markers and continue Accept
                     *     so the child can validate against ReadLast normally;
                     *     also drain any parked dependents of that parent.
                     *  2. Otherwise soft-park the child as a DEPENDENT (not a
                     *     root) WITHOUT ERROR and WITHOUT relaying. Roots stay in
                     *     mapConflicts for Check() reconciliation; dependents
                     *     re-evaluate via ProcessConflictDependents when the
                     *     root resolves, or when peers re-offer after eviction. */
                    if(IsConflictNode(tx.hashPrevTx))
                    {
                        if(LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::BLOCK))
                        {
                            debug::log(1, FUNCTION, "stale CONFLICT DAG marker for confirmed prev ",
                                tx.hashPrevTx.SubString(), "; clearing and re-evaluating");

                            /* Drop any stale marker for THIS tx first so the
                             * dependent walk cannot re-enter Accept(hashTx)
                             * while we are already accepting it. Arm the
                             * tail guard so a later Accept failure cannot
                             * leave grandchildren stranded under hashTx. */
                                                        EraseConflictRoot(hashTx);
                                                        if(mapConflictDependentsByIndex.count(hashTx))
                                                        {
                               const TAO::Ledger::Transaction& txSelf =
                                   mapConflictDependentsByIndex[hashTx];
                               mapConflictDependents.erase(txSelf.hashPrevTx);
                               mapConflictDependentsByIndex.erase(hashTx);
                               depTailGuard.Arm();
                                                        }

                                                        /* Prev may be a root or a parked dependent. */
                                                        if(mapConflicts.count(tx.hashPrevTx))
                                                        {
                               EraseConflictRoot(tx.hashPrevTx);
                               ProcessConflictDependents(tx.hashPrevTx);
                                                        }
                                                        else if(mapConflictDependentsByIndex.count(tx.hashPrevTx))
                                                        {
                               const TAO::Ledger::Transaction& txPrevDep =
                                   mapConflictDependentsByIndex[tx.hashPrevTx];
                               mapConflictDependents.erase(txPrevDep.hashPrevTx);
                               mapConflictDependentsByIndex.erase(tx.hashPrevTx);
                               ProcessConflictDependents(tx.hashPrevTx);
                                                        }
                                                        /* Fall through to ReadLast / Verify path. */
                        }
                        else
                        {
                            ParkConflictDependent(tx);
                            return false;
                        }
                    }

                    /* Get the last hash. */
                    uint512_t hashLast = 0;
                    if(!LLD::Ledger->ReadLast(tx.hashGenesis, hashLast, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "tx ", hashTx.SubString(), " REJECTED: Failed to read hash last");

                    /* Check for conflicts against the live tip — ROOT only. */
                    if(tx.hashPrevTx != hashLast)
                    {
                        /* We only need to output debug info and insert if this is a new conflict. [B2] */
                        if(!mapConflicts.count(hashTx))
                        {
                            /* Option C: root-only insert (no descendant cascade). */
                            AddConflictRoot(tx);

                            /* Relay the conflict if we are running over tritium protocol, so the
                             * genesis's canonical last-hash and the conflicted tx are both
                             * re-announced. This gives peers (and this node, via its own GET
                             * follow-up) a chance to re-sync the sigchain once the fork resolves,
                             * rather than leaving the transaction permanently stranded. */
                            if(pnode && LLP::TRITIUM_SERVER)
                            {
                                LLP::TRITIUM_SERVER->Relay
                                (
                                    LLP::TritiumNode::ACTION::NOTIFY,
                                    uint8_t(LLP::TritiumNode::TYPES::TRANSACTION),
                                    hashLast
                                );

                                /* Relay the transaction notification. */
                                LLP::TRITIUM_SERVER->Relay
                                (
                                    LLP::TritiumNode::ACTION::NOTIFY,
                                    uint8_t(LLP::TritiumNode::TYPES::TRANSACTION),
                                    hashTx
                                );
                            }
                        }

                        return false;
                    }
                }
                else if(tx.IsFirst() && LLD::Ledger->HasFirst(tx.hashGenesis))
                {
                    /* Duplicate genesis is a ROOT conflict. [B2] */
                    if(!mapConflicts.count(hashTx))
                    {
                        debug::error(FUNCTION, "CONFLICT: duplicate genesis-id ", tx.hashGenesis.SubString());
                        AddConflictRoot(tx);
                    }

                    return false;
                }

                /* Begin an ACID transction for internal memory commits. */
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

                    /* Check if Transaction::Connect() classified this as a
                     * local-state-dependent failure (e.g. coinbase appears
                     * immature at our stale local height but would pass at
                     * the peer-advertised best height).  In that case do NOT
                     * add the tx to mapRejected — the blacklist would prevent
                     * re-admission once the node catches up, turning a
                     * transient staleness into a permanent wedge.  Simply
                     * return false; the peer will re-offer the tx, and once
                     * our height advances the next Accept() call will succeed. */
                    const AdmissibilityClass nClass = TakeLastConnectClass();
                    if(nClass == AdmissibilityClass::DEFERRED_LOCAL_STATE)
                    {
                        debug::warning(FUNCTION,
                            "=== STRANDED_STATE_DETECTED === tx ", hashTx.SubString(),
                            " deferred (local state stale): ", debug::GetLastError(),
                            " — will retry when height advances");
                        return false;
                    }

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

                /* Success path owns the tail drain; do not drop on scope exit. */
                depTailGuard.Disarm();

                /* Debug output. */
                debug::log(0, FUNCTION, "tx ", hashTx.SubString(), " ACCEPTED in ", std::dec, timer.ElapsedMilliseconds(), " ms");

                /* Process orphan queue. */
                ProcessOrphans(hashTx);

                /* Drain any parked conflict-dependent tail now that this tx is
                 * live (covers intermediate dependents that Accept themselves
                 * after a stale DAG marker was cleared). No-op when empty. */
                ProcessConflictDependents(hashTx);

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

                /* Get the previous hash. */
                const uint512_t hashThis = tx.GetHash();

                /* Debug output. */
                debug::log(0, FUNCTION, "PROCESSING ORPHAN tx ", hashThis.SubString());

                /* Check if this is already in our mempool. */
                if(mapLedger.count(hashThis))
                {
                    /* Erase the transaction. */
                    mapOrphans.erase(hashTx);
                    setOrphansByIndex.erase(hashThis);
                    mapOrphansByIndex.erase(hashThis);

                    /* Set the hashTx. */
                    hashTx = hashThis;

                    continue;
                }

                /* Set our internal cached hash. */
                tx.hashCache = hashThis;

                /* Accept the transaction into memory pool. */
                if(!Accept(tx))
                {
                    //mapRejected.insert(hashTx);
                    debug::log(0, FUNCTION, "ORPHAN tx ", hashTx.SubString(), " REJECTED");

                    return;
                }

                /* Erase the transaction. */
                mapOrphans.erase(hashTx);
                setOrphansByIndex.erase(hashThis);
                mapOrphansByIndex.erase(hashThis);

                /* Set the hashTx. */
                hashTx = hashThis;
            }
        }


        /* Gets a transaction from mempool */
        bool Mempool::Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted) const
        {
            RECURSIVE(MUTEX);

            /* Check in conflict ROOT memory. */
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

            /* Check in parked conflict DEPENDENT memory (Option C). */
            if(mapConflictDependentsByIndex.count(hashTx))
            {
                tx = mapConflictDependentsByIndex.at(hashTx);
                fConflicted = true;

                tx.hashCache = hashTx;

                debug::log(2, FUNCTION, "CONFLICT-DEPENDENT TRANSACTION: ", hashTx.SubString());

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

            /* Check in conflict ROOT memory. */
            if(mapConflicts.count(hashTx))
            {
                /* Get from conflicts map. */
                tx = mapConflicts.at(hashTx);

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

                return true;
            }

            /* Check in parked conflict DEPENDENT memory (Option C). */
            if(mapConflictDependentsByIndex.count(hashTx))
            {
                tx = mapConflictDependentsByIndex.at(hashTx);
                tx.hashCache = hashTx;
                return true;
            }

            /* Check in orphans memory. */
            if(mapOrphansByIndex.count(hashTx))
            {
                /* Get from orphans map. */
                tx = mapOrphansByIndex.at(hashTx);

                /* Set our internal cached hash. */
                tx.hashCache = hashTx;

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

            /* Option C: include parked conflict dependents so HasTx(MEMPOOL)
             * treats them as known (not missing orphans) while still keeping
             * them out of the live mapLedger tip used by ReadLast/Get(genesis). */
            return mapLedger.count(hashTx)
                || mapLegacy.count(hashTx)
                || mapConflicts.count(hashTx)
                || mapConflictDependentsByIndex.count(hashTx)
                || mapOrphansByIndex.count(hashTx);
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

            /* Erase from conflict ROOT memory and drop its parked tail so
             * dependents cannot remain stranded without a Check() trigger. */
            if(mapConflicts.count(hashTx))
                DropConflictTree(hashTx);

            /* Erase from parked conflict DEPENDENT memory, including any
             * descendant chain keyed under this transaction's hash. */
            if(mapConflictDependentsByIndex.count(hashTx))
            {
                const TAO::Ledger::Transaction& txDep =
                    mapConflictDependentsByIndex[hashTx];
                const auto itDep = mapConflictDependents.find(txDep.hashPrevTx);
                if(itDep != mapConflictDependents.end() &&
                   itDep->second.GetHash() == hashTx)
                    mapConflictDependents.erase(itDep);

                mapConflictDependentsByIndex.erase(hashTx);
                DropConflictDependents(hashTx);
            }

            /* Erase from rejected memory. */
            if(mapRejected.count(hashTx))
                mapRejected.erase(hashTx);

            /* Erase from legacy conflicted memory. */
            if(mapLegacyConflicts.count(hashTx))
                mapLegacyConflicts.erase(hashTx);

            /* Erase from orphans memory. */
            if(setOrphansByIndex.count(hashTx))
                setOrphansByIndex.erase(hashTx);

            /* Erase from orphans index. mapOrphans is keyed by hashPrevTx, so
             * drop the prev-keyed slot only when it still points at this tx —
             * otherwise a newer orphan sharing the same missing parent would
             * be deleted incorrectly. */
            if(mapOrphansByIndex.count(hashTx))
            {
                const TAO::Ledger::Transaction& txOrphan = mapOrphansByIndex[hashTx];
                const auto itOrphan = mapOrphans.find(txOrphan.hashPrevTx);
                if(itOrphan != mapOrphans.end() && itOrphan->second.GetHash() == hashTx)
                    mapOrphans.erase(itOrphan);

                mapOrphansByIndex.erase(hashTx);
            }

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
            for(auto& rTransaction : mapTransactions)
            {
                /* Get reference of the vector. */
                std::vector<TAO::Ledger::Transaction>& vtx = rTransaction.second;

                /* Sort the list by sequence numbers. */
                std::sort(vtx.begin(), vtx.end());

                /* Add the hashes into list. */
                uint512_t hashLastDisk = 0;
                if(!LLD::Ledger->ReadLast(rTransaction.first, hashLastDisk))
                    continue;

                /* Loop through transaction by genesis. */
                uint512_t hashLast = hashLastDisk; //we make a copy here so we can know when we reached end of chain.
                for(uint32_t n = 0; n < vtx.size(); ++n)
                {
                    /* We don't run this check on our first transaction. */
                    if(!vtx[n].IsFirst())
                    {
                        /* Start a ACID transaction (to be disposed). */
                        LLD::TxnBegin(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::MEMORY);

                        /* Check the contracts for our root transaction to make sure it's valid. */
                        bool fContractInvalid = false;
                        for(const auto& rContract : vtx[n].Contracts())
                        {
                            /* Sanitize the contract. */
                            if(!rContract.Sanitize())
                            {
                                fContractInvalid = true;
                                break;
                            }
                        }

                        /* Abort the mempool ACID transaction once the contract is sanitized */
                        LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::MEMORY);

                        /* Check that transaction is in sequence. */
                        if(vtx[n].hashPrevTx != hashLast || fContractInvalid)
                        {
                            /* Debug information. */
                            if(fContractInvalid)
                                debug::notice(FUNCTION, "ORPHAN REJECTED AT INDEX ", n, ": invalid orphan chain ", vtx[n].hashPrevTx.SubString());
                            else
                                debug::notice(FUNCTION, "ORPHAN DETECTED AT INDEX ", n, ": last hash mismatch ", vtx[n].hashPrevTx.SubString());

                            /* Begin the memory transaction. */
                            LLD::TxnBegin(FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY);

                            /* Track whether the LLD transaction is still active (not aborted). */
                            bool fTxnActive = true;

                            /* Disconnect all transactions in reverse order. */
                            for(auto tx = vtx.rbegin(); tx != vtx.rend(); ++tx)
                            {
                                /* Find the transaction in pool. */
                                const uint512_t hashTx = tx->GetHash();

                                /* Check for our stop hash. */
                                if(hashTx == hashLast)
                                {
                                    debug::notice(FUNCTION, "REACHED HASH LAST ", hashLast.SubString());
                                    break;
                                }

                                /* Debug output tx. */
                                tx->print();

                                /* Check for ending of sequence. */
                                const bool fRoot = (n == 0);

                                /* Reset memory states to disk indexes. */
                                if(!tx->Disconnect(fRoot ? FLAGS::ERASE : FLAGS::MEMPOOL))
                                {
                                    /* Revert any partial LLD ACID state changes. */
                                    LLD::TxnAbort(FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY);
                                    fTxnActive = false;

                                    /* Force-evict this stuck orphan so it doesn't loop forever.
                                     * TxnAbort has already reverted any partial memory changes,
                                     * so removing it from mapLedger restores consistency. */
                                    debug::warning(FUNCTION, "evicting unrollbackable orphan tx ",
                                        hashTx.SubString(), " after failed Disconnect/Rollback");
                                    Remove(hashTx);
                                    break;
                                }

                                /* Erase from the memory map. */
                                Remove(hashTx);

                                /* Remove the API sessions indexes if disconnecting a mempool transaction. */
                                if(LLD::Sessions->Active(tx->hashGenesis))
                                {
                                    /* Get a reference of our transaction. */
                                    TAO::API::Transaction wtx =
                                        TAO::API::Transaction(*tx);

                                    /* Make sure indexes are deleted. */
                                    if(wtx.Delete(hashTx))
                                        debug::log(0, FUNCTION, "DELETED API session indexes for ", hashTx.SubString());
                                }

                                /* Write the txid of deleted transactions. */
                                debug::notice(FUNCTION, "DELETED ", hashTx.SubString());

                                /* Special output for our root orphan. */
                                if(fRoot)
                                    debug::notice(FUNCTION, "ROOT ORPHAN: disconnected root with FLAGS::ERASE: ", hashTx.SubString());
                            }

                            /* Only commit if the LLD transaction was not already aborted. */
                            if(fTxnActive)
                                LLD::TxnCommit(FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY);

                            break;
                        }
                    }

                    /* Set last hash. */
                    hashLast = vtx[n].GetHash();
                }
            }

            /* Evict conflicted transctions from mempool by checking they have been orphaned. */
            std::map<uint256_t, std::vector<TAO::Ledger::Transaction> > mapConflicted;

            /* Loop through all our conflicted transactions. */
            for(const auto& tx : mapConflicts)
            {
                /* Cache the genesis. */
                const uint256_t& hashGenesis = tx.second.hashGenesis;

                /* Check in map for push back. */
                if(!mapConflicted.count(hashGenesis))
                    mapConflicted[hashGenesis] = std::vector<TAO::Ledger::Transaction>();

                /* Push to back of map. */
                mapConflicted[hashGenesis].push_back(tx.second);
            }

            /* Loop transctions map by genesis. */
            for(auto& rTransaction : mapConflicted)
            {
                /* Get reference of the vector. */
                std::vector<TAO::Ledger::Transaction>& vtx = rTransaction.second;

                /* Sort the list by sequence numbers. */
                std::sort(vtx.begin(), vtx.end());

                /* Add the hashes into list. */
                uint512_t hashLastDisk = 0;
                if(!LLD::Ledger->ReadLast(rTransaction.first, hashLastDisk))
                    continue; /* Mirror the mapLedger orphan loop: one genesis's
                               * missing disk tip must not abort reconciliation
                               * for every subsequent genesis in this sweep. */

                /* Check if our conflict chain needs to be evicted. */
                if(vtx[0].hashPrevTx != hashLastDisk)
                {
                    const uint256_t& hashGenesis = rTransaction.first;
                    const ForkDivergenceInfo tInfo =
                        ComputeForkDivergence(hashGenesis, vtx[0].hashPrevTx);

                    /* Classify the conflict using the admissibility framework.
                     *
                     * DEFERRED_LOCAL_STATE: ancestor is on our main chain —
                     *   the sigchain is idle (has not published a transaction
                     *   in some time) and the predecessor our conflicted tx
                     *   expects is on the canonical chain.  This is NOT a fork:
                     *   it is local chain-state staleness.  Retain and retry.
                     *   Note: tInfo.nDepth measures sigchain idle time in blocks
                     *   NOT fork divergence depth — must NOT drive eviction.
                     *
                     * UNKNOWN: the conflicting predecessor's block could not
                     *   be located on disk at all (!tInfo.fAncestorFound).
                     *   This is NOT proof of a genuine fork -- it is exactly
                     *   what a node still catching up to a peer's canonical
                     *   branch looks like (the ancestor simply has not been
                     *   synced yet). Treating this the same as a confirmed
                     *   off-chain ancestor was the root cause of nodes
                     *   getting permanently wedged on a stale/orphaned local
                     *   branch: the correct chain's own transaction was being
                     *   evicted before the block sync that would have
                     *   resolved it could catch up. Retain, retry with a
                     *   larger budget, and actively re-request the missing
                     *   predecessor transaction so the gap can close.
                     *
                     * INVALID_ABSOLUTE: ancestor was found but confirmed off
                     *   our main chain — a genuine, resolved fork conflict.
                     *   Evict permanently. */
                    if(tInfo.fAncestorFound && tInfo.fAncestorOnMainChain)
                    {
                        /* DEFERRED_LOCAL_STATE path.
                         * Emit once-per-genesis operator diagnostic so operators
                         * can see long-idle sigchains without being spammed. */
                        if(!setStrandedGeneses.count(hashGenesis))
                        {
                            if(setStrandedGeneses.size() >= MAX_CONFLICTS_MAP_ENTRIES)
                                setStrandedGeneses.clear();
                            setStrandedGeneses.insert(hashGenesis);

                            debug::warning(FUNCTION, ANSI_COLOR_BRIGHT_YELLOW,
                                "=== STRANDED_STATE_DETECTED ===", ANSI_COLOR_RESET,
                                " class=DEFERRED_LOCAL_STATE",
                                " genesis=", hashGenesis.SubString(),
                                " divergence_depth=", tInfo.nDepth,
                                " (diagnostic only — measures sigchain idle time, NOT fork depth)",
                                " ancestor=", tInfo.hashAncestorBlock.SubString(),
                                " action=retain_and_retry");
                        }
                        else
                        {
                            debug::log(2, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW,
                                "FORK CONFLICT DIAGNOSTIC:", ANSI_COLOR_RESET,
                                " genesis=", hashGenesis.SubString(),
                                " divergence_depth=", tInfo.nDepth,
                                " ancestor=", tInfo.hashAncestorBlock.SubString(),
                                " action=retain_and_retry (DEFERRED_LOCAL_STATE)");
                        }

                        /* Bound and increment the retry counter for this genesis. */
                        if(mapConflictRetries.size() >= MAX_CONFLICTS_MAP_ENTRIES)
                            mapConflictRetries.clear();
                        const uint32_t nRetries = ++mapConflictRetries[hashGenesis];

                        if(nRetries > MAX_CONFLICT_STALE_RETRIES)
                        {
                            /* Budget exhausted: evict to stop consuming sweep cycles.
                             * This removes the conflict DAG tree for these roots — it
                             * does NOT roll back the active best chain or invalidate
                             * accepted blocks. The conflict can be re-evaluated if
                             * peers relay the tx again. A persistent
                             * DEFERRED_LOCAL_STATE may indicate a long-idle sigchain
                             * or a fork whose resolution stalled; worth investigating
                             * but is not itself an active-chain rollback or corruption
                             * event. */
                            debug::warning(FUNCTION,
                                "MEMPOOL_CONFLICT_EVICTED class=DEFERRED_LOCAL_STATE",
                                " genesis=", hashGenesis.SubString(),
                                " retries=", nRetries,
                                " action=evict_conflict_dag",
                                " active_chain_affected=false",
                                " (conflict roots+dependents removed; tx re-evaluates if relayed again)");

                            ClearGenesisConflictState(hashGenesis);
                            for(const auto& tx : vtx)
                                DropConflictTree(tx.GetHash());
                        }
                        /* else: retain in mapConflicts — do not erase */
                    }
                    else if(!tInfo.fAncestorFound)
                    {
                        /* UNKNOWN path: sync gap, not a confirmed fork.
                         * Emit once-per-genesis operator diagnostic so operators
                         * can see a stuck sync without being spammed. */
                        if(!setUnknownAncestorGeneses.count(hashGenesis))
                        {
                            if(setUnknownAncestorGeneses.size() >= MAX_CONFLICTS_MAP_ENTRIES)
                                setUnknownAncestorGeneses.clear();
                            setUnknownAncestorGeneses.insert(hashGenesis);

                            debug::warning(FUNCTION, ANSI_COLOR_BRIGHT_YELLOW,
                                "=== STRANDED_STATE_DETECTED ===", ANSI_COLOR_RESET,
                                " class=UNKNOWN",
                                " genesis=", hashGenesis.SubString(),
                                " conflicting_predecessor=", tInfo.hashPrevTx.SubString(),
                                " reason=", tInfo.strError.empty() ? "ancestor not found on disk" : tInfo.strError,
                                " action=retain_and_retry (fetching missing predecessor)");
                        }
                        else
                        {
                            debug::log(2, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW,
                                "FORK CONFLICT DIAGNOSTIC:", ANSI_COLOR_RESET,
                                " genesis=", hashGenesis.SubString(),
                                " conflicting_predecessor=", tInfo.hashPrevTx.SubString(),
                                " action=retain_and_retry (UNKNOWN)");
                        }

                        /* Actively re-request the missing predecessor transaction
                         * from a random connected peer.  This mirrors the GET
                         * request already issued for a brand-new orphan in
                         * Accept(), giving a stuck sync a concrete chance to
                         * close the gap rather than depending solely on the
                         * unrelated block-level sync catching up on its own. */
                        if(LLP::TRITIUM_SERVER)
                        {
                            const std::shared_ptr<LLP::TritiumNode> pRandom =
                                LLP::TRITIUM_SERVER->RandomConnection();

                            if(pRandom && pRandom->Connected())
                                pRandom->PushMessage(LLP::TritiumNode::ACTION::GET,
                                    uint8_t(LLP::TritiumNode::TYPES::TRANSACTION), tInfo.hashPrevTx);
                        }

                        /* Bound and increment the retry counter for this genesis. */
                        if(mapUnknownAncestorRetries.size() >= MAX_CONFLICTS_MAP_ENTRIES)
                            mapUnknownAncestorRetries.clear();
                        const uint32_t nRetries = ++mapUnknownAncestorRetries[hashGenesis];

                        if(nRetries > MAX_UNKNOWN_ANCESTOR_RETRIES)
                        {
                            /* Budget exhausted: evict to stop consuming sweep cycles.
                             * This removes the conflict DAG tree for these roots — it
                             * does NOT roll back the active best chain or invalidate
                             * accepted blocks. The conflict can be re-evaluated if
                             * peers relay the tx again. A persistent UNKNOWN ancestor
                             * may indicate a sync gap, a missing predecessor block, or
                             * a fork the node hasn't seen; worth investigating but is
                             * not itself an active-chain rollback. */
                            debug::warning(FUNCTION,
                                "MEMPOOL_CONFLICT_EVICTED class=UNKNOWN",
                                " genesis=", hashGenesis.SubString(),
                                " retries=", nRetries,
                                " reason=predecessor_not_found_on_disk",
                                " action=evict_conflict_dag",
                                " active_chain_affected=false",
                                " (conflict roots+dependents removed; tx re-evaluates if relayed again)");

                            ClearGenesisConflictState(hashGenesis);
                            for(const auto& tx : vtx)
                                DropConflictTree(tx.GetHash());
                        }
                        /* else: retain in mapConflicts — do not erase */
                    }
                    else
                    {
                        /* INVALID_ABSOLUTE: ancestor was found but is
                         * confirmed off the main chain — this is a genuine,
                         * resolved fork conflict.  Evict permanently. */
                        debug::warning(FUNCTION, "fork conflict INVALID_ABSOLUTE for genesis=",
                            hashGenesis.SubString(),
                            " reason=", tInfo.strError.empty() ? "ancestor not on main chain" : tInfo.strError);

                        /* Clear any stale retry state for this genesis. */
                        ClearGenesisConflictState(hashGenesis);

                        for(const auto& tx : vtx)
                            DropConflictTree(tx.GetHash());
                    }
                }

                /* [B3] The conflict has resolved (e.g. after a reorg/fork
                 * settled) and disk's last-hash for this genesis now matches
                 * the earliest conflicted transaction. Without this, the
                 * transaction(s) would remain stranded in mapConflicts
                 * forever: Get()/ReadTx() would keep returning them as
                 * permanently conflicted (fConflicted=true) even though they
                 * are valid again, and they would never be re-added to the
                 * live mempool or relayed to peers. Re-run full acceptance
                 * (in sequence order) so each transaction is re-validated,
                 * re-connected, and moved back into mapLedger. Option C also
                 * drains parked dependents of each resolved root. */
                else
                {
                    /* Conflict has resolved: clear retry state for this genesis
                     * so stale counts don't affect a future re-conflict. */
                    const uint256_t& hashGenesis = rTransaction.first;
                    ClearGenesisConflictState(hashGenesis);

                    /* Loop through our ROOT transactions in sequence order. */
                    for(const auto& tx : vtx)
                    {
                        /* Cache the hash so we can erase before re-accepting. */
                        const uint512_t hashTx = tx.GetHash();

                        /* Remove from conflict roots first so Accept() doesn't
                         * see this transaction's own prior conflicted state. */
                        EraseConflictRoot(hashTx);

                        /* Re-validate and re-admit into the live mempool. Only drain
                         * dependents after a successful root Accept — otherwise
                         * children evaluate without their parent in mapLedger
                         * and the remaining tail is dropped as unreachable.
                         * Accept() itself also drains conflict dependents on
                         * success; the explicit call here covers the case where
                         * Accept short-circuited because the root was already
                         * live. On failure drop the parked tail so it cannot
                         * sit detached after EraseConflictRoot. */
                        if(Accept(tx))
                        {
                            ProcessConflictDependents(hashTx);
                        }
                        else
                        {
                            debug::log(0, FUNCTION, "failed to re-admit resolved conflicted tx ", hashTx.SubString(),
                                " for genesis ", tx.hashGenesis.SubString(), ": ", debug::GetLastError());
                            DropConflictDependents(hashTx);
                        }
                    }
                }
            }
        }


        /* Read-only computation of how far the best chain has diverged from a
         * genesis's on-disk expected predecessor transaction. */
        ForkDivergenceInfo Mempool::ComputeForkDivergence(const uint256_t& hashGenesis, const uint512_t& hashPrevTx)
        {
            ForkDivergenceInfo tInfo;
            tInfo.hashPrevTx  = hashPrevTx;
            tInfo.nBestHeight = TAO::Ledger::ChainState::nBestHeight.load();

            /* Read our own disk-committed last transaction for this genesis
             * (bypassing the mempool cache entirely). Failure here means we
             * have never committed any transaction for this genesis at all. */
            if(!LLD::Ledger->ReadLast(hashGenesis, tInfo.hashOurLast))
            {
                tInfo.strError = "genesis has no committed last transaction on disk";
                return tInfo;
            }

            /* If disk has already caught up with what the conflicting
             * transaction expects, there is no divergence to report. */
            if(tInfo.hashOurLast == hashPrevTx)
            {
                tInfo.fResolved = true;
                return tInfo;
            }

            /* Locate the block that committed the predecessor transaction the
             * conflicting/canonical transaction expects. If we don't have that
             * transaction on disk at all, we have no safe common-ancestor
             * point to compute a divergence depth from. */
            if(!LLD::Ledger->ReadBlock(hashPrevTx, tInfo.stateAncestor))
            {
                tInfo.strError = debug::safe_printstr("conflicting predecessor ", hashPrevTx.SubString(),
                    " not found on disk");
                return tInfo;
            }

            tInfo.fAncestorFound     = true;
            tInfo.hashAncestorBlock  = tInfo.stateAncestor.GetHash();
            tInfo.nAncestorHeight    = tInfo.stateAncestor.nHeight;
            tInfo.fAncestorOnMainChain = tInfo.stateAncestor.IsInMainChain();

            if(!tInfo.fAncestorOnMainChain)
            {
                tInfo.strError = debug::safe_printstr("candidate ancestor ", tInfo.hashAncestorBlock.SubString(),
                    " is not in the main chain");
                return tInfo;
            }

            if(tInfo.nBestHeight < tInfo.nAncestorHeight)
            {
                tInfo.strError = debug::safe_printstr("candidate ancestor height ", tInfo.nAncestorHeight,
                    " is ahead of best height ", tInfo.nBestHeight);
                return tInfo;
            }

            tInfo.nDepth = tInfo.nBestHeight - tInfo.nAncestorHeight;

            return tInfo;
        }


        /* [C1] Overload that locates the earliest currently-conflicted
         * transaction for hashGenesis in mapConflicts itself, for callers that
         * only have the genesis-id (e.g. the checkforkrecovery RPC). */
        ForkDivergenceInfo Mempool::ComputeForkDivergence(const uint256_t& hashGenesis)
        {
            RECURSIVE(MUTEX);

            /* Option C: prefer the per-genesis earliest-root index. */
            const auto itRoot = mapConflictRootByGenesis.find(hashGenesis);
            if(itRoot != mapConflictRootByGenesis.end())
            {
                const auto itTx = mapConflicts.find(itRoot->second);
                if(itTx != mapConflicts.end())
                    return ComputeForkDivergence(hashGenesis, itTx->second.hashPrevTx);
            }

            /* Fallback: scan roots (index may be mid-repair). */
            std::vector<TAO::Ledger::Transaction> vtx;
            for(const auto& tx : mapConflicts)
                if(tx.second.hashGenesis == hashGenesis)
                    vtx.push_back(tx.second);

            if(vtx.empty())
            {
                ForkDivergenceInfo tInfo;
                tInfo.strError = "no conflicted transactions currently tracked in mempool for this genesis";
                return tInfo;
            }

            std::sort(vtx.begin(), vtx.end());

            return ComputeForkDivergence(hashGenesis, vtx[0].hashPrevTx);
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

                /* Loop transctions map by genesis. */
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
                /* Loop transctions map by genesis. */
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


        /* Gets the size of the conflict ROOT set (excludes parked dependents). */
        uint32_t Mempool::Conflicts()
        {
            RECURSIVE(MUTEX);

            return static_cast<uint32_t>(mapConflicts.size() + mapLegacyConflicts.size());
        }


        /* Gets the number of parked conflict dependents (Option C). */
        uint32_t Mempool::ConflictDependents()
        {
            RECURSIVE(MUTEX);

            return static_cast<uint32_t>(mapConflictDependentsByIndex.size());
        }


        /* True when Accept hard-rejected hashTx into mapRejected. */
        bool Mempool::Rejected(const uint512_t& hashTx) const
        {
            RECURSIVE(MUTEX);

            return mapRejected.find(hashTx) != mapRejected.end();
        }
    }
}
