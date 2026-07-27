/*__________________________________________________________________________________________

		Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

		(c) Copyright The Nexus Developers 2014 - 2026

		Distributed under the MIT software license, see the accompanying
		file COPYING or http://www.opensource.org/licenses/mit-license.php.

		"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>

#include <TAO/Ledger/types/locator.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/tritium.h>

#include <Legacy/types/legacy.h>

#include <Util/include/runtime.h>

#include <vector>
#include <queue>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* Static instantiation of block orphan graph. */
        OrphanPool mapOrphans;


        bool OrphanPool::Insert(const TAO::Ledger::Block& block, uint1024_t* pHashEvicted)
        {
            const uint1024_t hashBlock = block.GetHash();
            if(mapByHash.count(hashBlock))
                return false;

            if(pHashEvicted)
                *pHashEvicted = 0;

            if(mapByHash.size() >= MAX_BLOCK_ORPHANS)
            {
                const uint1024_t hashEvicted = listInsertionOrder.front();
                Remove(hashEvicted);
                if(pHashEvicted)
                    *pHashEvicted = hashEvicted;
            }

            mapByHash[hashBlock] = std::unique_ptr<TAO::Ledger::Block>(block.Clone());
            mapByParent[block.hashPrevBlock].insert(hashBlock);
            listInsertionOrder.push_back(hashBlock);
            mapInsertion[hashBlock] = std::prev(listInsertionOrder.end());
            return true;
        }


        bool OrphanPool::Contains(const uint1024_t& hashBlock) const
        {
            return mapByHash.count(hashBlock) != 0;
        }


        const TAO::Ledger::Block* OrphanPool::Get(const uint1024_t& hashBlock) const
        {
            const auto it = mapByHash.find(hashBlock);
            return it == mapByHash.end() ? nullptr : it->second.get();
        }


        std::vector<uint1024_t> OrphanPool::Children(const uint1024_t& hashParent) const
        {
            const auto it = mapByParent.find(hashParent);
            if(it == mapByParent.end())
                return {};

            return std::vector<uint1024_t>(it->second.begin(), it->second.end());
        }


        bool OrphanPool::Remove(const uint1024_t& hashBlock)
        {
            const auto it = mapByHash.find(hashBlock);
            if(it == mapByHash.end())
                return false;

            const uint1024_t hashParent = it->second->hashPrevBlock;
            const auto itParent = mapByParent.find(hashParent);
            if(itParent != mapByParent.end())
            {
                itParent->second.erase(hashBlock);
                if(itParent->second.empty())
                    mapByParent.erase(itParent);
            }

            const auto itInsertion = mapInsertion.find(hashBlock);
            if(itInsertion != mapInsertion.end())
            {
                listInsertionOrder.erase(itInsertion->second);
                mapInsertion.erase(itInsertion);
            }

            mapByHash.erase(it);
            return true;
        }


        uint64_t OrphanPool::RemoveSubtree(const uint1024_t& hashRoot)
        {
            uint64_t nRemoved = 0;
            std::queue<uint1024_t> queueHashes;
            queueHashes.push(hashRoot);

            while(!queueHashes.empty())
            {
                const uint1024_t hash = queueHashes.front();
                queueHashes.pop();

                const std::vector<uint1024_t> vChildren = Children(hash);
                for(const auto& hashChild : vChildren)
                    queueHashes.push(hashChild);

                if(Remove(hash))
                    ++nRemoved;
            }

            return nRemoved;
        }


        void OrphanPool::Clear()
        {
            mapByHash.clear();
            mapByParent.clear();
            listInsertionOrder.clear();
            mapInsertion.clear();
        }


        uint64_t OrphanPool::Size() const
        {
            return mapByHash.size();
        }


        bool OrphanPool::Empty() const
        {
            return mapByHash.empty();
        }


        /* Track the times we have requested missing transactions for a block so
         * we don't keep re-requesting the same unresolvable transactions forever.
         *
         * A block whose vMissing set references a transaction that can never pass
         * Transaction::Check() (e.g. a tx with a timestamp "too far in the
         * future") would otherwise be re-requested indefinitely.  This soft
         * retry counter is keyed by the offending block hash (hashMissing).  Once
         * the retry count exceeds MAX_MISSING_TRANSACTIONS_RETRIES we stop
         * re-requesting that block's transactions (vMissing is cleared) so the
         * node can keep advancing the real chain tip.  A successful ACCEPT clears
         * the entry so the genuine "transaction simply not seen yet" recovery
         * path is unaffected. */
        std::map<uint1024_t, uint64_t> mapLastMissing;

        /* Track how many full branch-recovery escalations each missing block
         * hash has gone through. */
        std::map<uint1024_t, uint32_t> mapMissingBranchEscalations;


        /* [Option C] Track consecutive Check()-rejections keyed by the exact
         * block hash. See declaration comment in include/process.h for the
         * rationale: this distinguishes a block that is genuinely invalid
         * (fails identically every time) from one that is spuriously failing
         * due to stale local mempool state (which a targeted resync + one
         * retry can resolve). */
        std::map<uint1024_t, uint32_t> mapCheckRejects;


        /* [C2] Track the last time we asked peers for an orphan ancestor's chain.
         * See declaration comment in include/process.h for rationale. */
        std::map<uint1024_t, uint64_t> mapLastOrphanRequest;


        /* Last-processed timestamp (ms) for each known-incomplete block hash.
         * Used to rate-limit re-entry into the expensive Check() + escalation
         * path so PROCESSING_MUTEX is not taken dozens of times per second
         * per peer for the same stuck block. */
        std::map<uint1024_t, uint64_t> mapLastMissingProcessTime;


        /* Hard terminal blacklist for blocks that have exhausted all
         * branch-recovery paths.  Checked at the top of Process() so an
         * unrecoverable block does not consume any DataThread budget. */
        std::set<uint1024_t> setUnrecoverableBlocks;


        /* Cache of the missing-tx hash list captured at the moment a block
         * hash is blacklisted, so the terminal-blacklist early return can
         * still hand block.vMissing to callers without running Check(). */
        std::map<uint1024_t, std::vector<std::pair<uint8_t, uint512_t> > > mapMissingTxCache;


        namespace
        {
            /** LocalMinedBlockRecord
             *
             *  Memory-only watch record for locally mined blocks that have been
             *  accepted.  These records are intentionally bounded and short-lived:
             *  they are removed once the block gains a best-chain descendant or
             *  is disconnected by a better branch.
             *
             **/
            struct LocalMinedBlockRecord
            {
                uint1024_t hashPrevBlock = 0;
                uint32_t nHeight = 0;
                uint32_t nChannel = 0;
                uint64_t nAcceptedAt = 0;
            };


            /** Maximum number of local mined blocks tracked at once.  Local
             *  mined blocks only need to stay watched until a descendant lands
             *  or a reorg disconnects them, so a small cap is sufficient and
             *  keeps oldest-entry eviction cheap. */
            static const uint64_t MAX_LOCAL_MINED_BLOCK_RECORDS = 256;


            /** Locally accepted mined blocks keyed by block hash. */
            std::map<uint1024_t, LocalMinedBlockRecord> mapLocalMinedBlocks;


            /** Mutex protecting local mined/recovery maps. */
            std::mutex LOCAL_MINED_MUTEX;


            void EvictOldestLocalMinedBlock()
            {
                if(mapLocalMinedBlocks.empty())
                    return;

                auto itOldest = mapLocalMinedBlocks.begin();
                for(auto it = mapLocalMinedBlocks.begin(); it != mapLocalMinedBlocks.end(); ++it)
                {
                    if(it->second.nAcceptedAt < itOldest->second.nAcceptedAt)
                        itOldest = it;
                }

                mapLocalMinedBlocks.erase(itOldest);
            }


            /* BlockState traversal intentionally takes copies: walking Prev()
             * mutates the cursor state while preserving the callers' states for
             * diagnostics and the eventual SetBest() call. */
            bool FindCommonAncestor(TAO::Ledger::BlockState stateA,
                                    TAO::Ledger::BlockState stateB,
                                    TAO::Ledger::BlockState& stateAncestor,
                                    uint32_t& nADepth,
                                    uint32_t& nBDepth)
            {
                nADepth = 0;
                nBDepth = 0;

                while(stateA.nHeight > stateB.nHeight)
                {
                    stateA = stateA.Prev();
                    /* BlockState::operator! reports an invalid/null traversal result. */
                    if(!stateA)
                        return false;
                    ++nADepth;
                }

                while(stateB.nHeight > stateA.nHeight)
                {
                    stateB = stateB.Prev();
                    /* BlockState::operator! reports an invalid/null traversal result. */
                    if(!stateB)
                        return false;
                    ++nBDepth;
                }

                while(stateA != stateB)
                {
                    stateA = stateA.Prev();
                    stateB = stateB.Prev();
                    /* BlockState::operator! reports an invalid/null traversal result. */
                    if(!stateA || !stateB)
                        return false;
                    ++nADepth;
                    ++nBDepth;
                }

                stateAncestor = stateA;
                return true;
            }


            /* stateDescendant is a by-value traversal cursor for the same reason:
             * the function walks it backward without mutating the caller's state. */
            bool IsAncestorOf(const TAO::Ledger::BlockState& stateAncestor,
                              TAO::Ledger::BlockState stateDescendant)
            {
                if(stateAncestor.nHeight > stateDescendant.nHeight)
                    return false;

                while(stateDescendant.nHeight > stateAncestor.nHeight)
                {
                    stateDescendant = stateDescendant.Prev();
                    if(!stateDescendant)
                        return false;
                }

                return stateDescendant.GetHash() == stateAncestor.GetHash();
            }


            bool ValidateStoredState(const TAO::Ledger::BlockState& state)
            {
                if(state.nVersion < 7 || state.vtx.empty()
                || state.vtx.back().first != TRANSACTION::TRITIUM)
                    return false;

                TAO::Ledger::TritiumBlock block;
                static_cast<TAO::Ledger::Block&>(block) =
                    static_cast<const TAO::Ledger::Block&>(state);
                block.nTime = state.nTime;
                block.vtx.assign(state.vtx.begin(), std::prev(state.vtx.end()));

                if(!LLD::Ledger->ReadTx(state.vtx.back().second, block.producer,
                    FLAGS::BLOCK))
                    return false;

                return block.CheckStored(true) && block.vMissing.empty()
                    && !block.fConflicted;
            }
        }


        /* Track the times we have requested processed missing transactions so we don't loop too much. */
        std::map<uint1024_t, uint64_t> mapLastMissing;


        /* Mutex to protect checking more than one block at a time. */
        std::mutex PROCESSING_MUTEX;


        /* Sync timer value. */
        uint64_t nSynchronizationTimer = 0;


        /* Current sync node. */
        std::atomic<uint64_t> nSyncSession(0);


        /* Stats variable for syncing. */
        std::atomic<uint64_t> nProcessedContracts(0);


        void TrackLocalMinedAcceptedBlock(const TAO::Ledger::Block& block)
        {
            const uint1024_t hashBlock = block.GetHash();

            std::lock_guard<std::mutex> lock(LOCAL_MINED_MUTEX);
            if(mapLocalMinedBlocks.size() >= MAX_LOCAL_MINED_BLOCK_RECORDS
            && !mapLocalMinedBlocks.count(hashBlock))
                EvictOldestLocalMinedBlock();

            LocalMinedBlockRecord record;
            record.hashPrevBlock = block.hashPrevBlock;
            record.nHeight       = block.nHeight;
            record.nChannel      = block.nChannel;
            record.nAcceptedAt   = runtime::timestamp();
            mapLocalMinedBlocks[hashBlock] = record;

            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "=== LOCAL_MINED_ACCEPTED ===", ANSI_COLOR_RESET,
                " height=", block.nHeight,
                " channel=", block.nChannel,
                " hash=", hashBlock.SubString(),
                " prev=", block.hashPrevBlock.SubString(),
                " best=", ChainState::hashBestChain.load().SubString());
        }


        void MarkLocalMinedBlockDisconnected(const TAO::Ledger::BlockState& state,
                                             const TAO::Ledger::BlockState& stateNewBest)
        {
            const uint1024_t hashBlock = state.GetHash();

            std::lock_guard<std::mutex> lock(LOCAL_MINED_MUTEX);
            const auto it = mapLocalMinedBlocks.find(hashBlock);
            if(it == mapLocalMinedBlocks.end())
                return;

            const bool fSameHeightSibling =
                (state.nHeight == stateNewBest.nHeight)
                && (it->second.hashPrevBlock == stateNewBest.hashPrevBlock);

            /* Orphaning a locally accepted mined block is operator-actionable in
             * the same-height race this recovery path targets, so it is emitted
             * as a warning while routine local acceptance remains an info log. */
            debug::warning(FUNCTION, ANSI_COLOR_BRIGHT_RED, "=== LOCAL_MINED_ORPHANED ===", ANSI_COLOR_RESET,
                " local_hash=", hashBlock.SubString(),
                " local_height=", state.nHeight,
                " local_channel=", it->second.nChannel,
                " peer_best=", stateNewBest.GetHash().SubString(),
                " peer_height=", stateNewBest.nHeight,
                " peer_channel=", stateNewBest.nChannel,
                " same_height_sibling=", (fSameHeightSibling ? "yes" : "no"),
                " accepted_age=", (runtime::timestamp() - it->second.nAcceptedAt), "s");

            mapLocalMinedBlocks.erase(it);
            ClearMiningTemplateCaches("local mined block orphaned by reorg");
        }


        void FinalizeLocalMinedTrackingAfterSetBest(const TAO::Ledger::BlockState& stateNewBest)
        {
            const uint1024_t hashNewBest = stateNewBest.GetHash();
            std::vector<uint1024_t> vConfirmed;

            {
                std::lock_guard<std::mutex> lock(LOCAL_MINED_MUTEX);
                for(const auto& item : mapLocalMinedBlocks)
                {
                    if(item.first == hashNewBest)
                        continue;

                    TAO::Ledger::BlockState stateLocal;
                    if(!LLD::Ledger->ReadBlock(item.first, stateLocal))
                    {
                        vConfirmed.push_back(item.first);
                        continue;
                    }

                    if(IsAncestorOf(stateLocal, stateNewBest))
                    {
                        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "=== LOCAL_MINED_CONFIRMED ===", ANSI_COLOR_RESET,
                            " local_hash=", item.first.SubString(),
                            " local_height=", item.second.nHeight,
                            " best_hash=", hashNewBest.SubString(),
                            " best_height=", stateNewBest.nHeight);
                        vConfirmed.push_back(item.first);
                    }
                }

                for(const auto& hash : vConfirmed)
                    mapLocalMinedBlocks.erase(hash);
            }
        }


        bool ActivateCandidateBestChain(const TAO::Ledger::BlockState& stateCandidate,
                                        const char* pszSource,
                                        bool fTransaction)
        {
            const TAO::Ledger::BlockState stateBest = ChainState::tStateBest.load();
            if(!stateCandidate.IsHeavierThan(stateBest) || stateCandidate.fConflicted)
                return false;

            TAO::Ledger::BlockState stateAncestor;
            uint32_t nConnectDepth = 0;
            uint32_t nDisconnectDepth = 0;
            if(!FindCommonAncestor(stateCandidate, stateBest, stateAncestor,
                nConnectDepth, nDisconnectDepth))
                return false;

            /* Preflight the complete connecting ancestry before any state change. */
            TAO::Ledger::BlockState stateCursor = stateCandidate;
            uint32_t nValidated = 0;
            while(stateCursor != stateAncestor)
            {
                if(stateCursor.fConflicted || !ValidateStoredState(stateCursor))
                    return debug::error(FUNCTION, "candidate preflight failed for ",
                        stateCursor.GetHash().SubString());

                const TAO::Ledger::BlockState statePrev = stateCursor.Prev();
                if(!statePrev || stateCursor.hashPrevBlock != statePrev.GetHash()
                || stateCursor.nHeight != statePrev.nHeight + 1)
                    return debug::error(FUNCTION, "candidate ancestry is inconsistent at ",
                        stateCursor.GetHash().SubString());

                stateCursor = statePrev;
                ++nValidated;
            }

            if(nValidated != nConnectDepth)
                return debug::error(FUNCTION, "candidate ancestry depth mismatch");

            if(fTransaction)
                LLD::TxnBegin(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);

            TAO::Ledger::BlockState stateActivation = stateCandidate;
            if(!stateActivation.SetBest())
            {
                if(fTransaction)
                    LLD::TxnAbort(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
                ChainState::fChainReorg.store(false);
                return false;
            }

            /* SetBest() commits the outer transaction internally when it succeeds.
             * When fTransaction=true we opened TxnBegin before calling SetBest(),
             * so the outer transaction was consumed by SetBest()'s internal TxnCommit.
             * Guard with HasOpenTransaction() so that a now-closed transaction is not
             * misreported as a commit failure. */
            if(fTransaction && LLD::HasOpenTransaction())
            {
                if(!LLD::TxnCommit(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS))
                    return debug::error(FUNCTION, "disk transaction commit failed for candidate activation");
            }

            debug::log(0, FUNCTION, "activated validated candidate source=",
                (pszSource ? pszSource : "ledger"),
                " hash=", stateCandidate.GetHash().SubString(),
                " connect=", nConnectDepth,
                " disconnect=", nDisconnectDepth);
            return true;
        }


        bool AttemptPeerBestChainRecovery(const uint1024_t& hashPeerBest,
                                          uint32_t nPeerHeight,
                                          const char* pszSource,
                                          LLP::TritiumNode* pnode)
        {
            /* Re-entrancy guard: this function releases PROCESSING_MUTEX and
             * calls Process(), which re-acquires it.  That is safe today
             * because nothing inside Process() calls back into this function.
             * Should a future code path introduce such a call on the same
             * thread, re-entering here would self-deadlock on the non-
             * recursive PROCESSING_MUTEX.  This thread_local flag makes that
             * one-way invariant explicit and turns a potential deadlock into
             * a harmless no-op instead. */
            thread_local bool fInPeerBestChainRecovery = false;
            if(fInPeerBestChainRecovery)
                return false;

            struct ReentrancyGuard
            {
                bool& fFlag;
                explicit ReentrancyGuard(bool& fFlagIn) : fFlag(fFlagIn) { fFlag = true; }
                ~ReentrancyGuard() { fFlag = false; }
            } guard(fInPeerBestChainRecovery);

            if(hashPeerBest == 0 || hashPeerBest == ChainState::hashBestChain.load())
                return false;

            if(!config::GetBoolArg("-peerbestchainrecovery", true))
                return false;

            TAO::Ledger::BlockState statePeer;
            if(!LLD::Ledger->ReadBlock(hashPeerBest, statePeer))
            {
                /* The peer's advertised tip is not on disk yet.  Walk the orphan
                 * graph backwards from hashPeerBest to find the deepest orphan
                 * whose own hashPrevBlock IS on disk (a "connectable ancestor").
                 * If found, feed it through Process() so the BFS drain can
                 * connect the chain forward to hashPeerBest.  If not found
                 * (genuine gap), issue a throttled locator-anchored branch
                 * request so the missing blocks can be downloaded. */

                std::unique_ptr<TAO::Ledger::Block> pConnectable;
                bool fInOrphanPool = false;

                /* Deepest hashPrevBlock reached by the walkback below.  This is
                 * the canonical throttle key (the missing ancestor hash) used
                 * for the gap-path ShouldSendBranchSyncRequest() call further
                 * down — never hashPeerBest, which would reintroduce the two-
                 * namespace key collision this helper was designed to
                 * eliminate.  Defaults to hashPeerBest itself so the throttle
                 * still has a sane key if the loop never executes. */
                uint1024_t hashDeepestAncestor = hashPeerBest;

                {
                    LOCK(PROCESSING_MUTEX);

                    fInOrphanPool = mapOrphans.Contains(hashPeerBest);
                    if(fInOrphanPool)
                    {
                        uint1024_t hashCurrent = hashPeerBest;
                        uint32_t nDepth = 0;

                        /* Guard against cycles in the orphan graph: a crafted
                         * orphan set with a short parent cycle could otherwise
                         * loop indefinitely rather than terminating via the
                         * depth cap. The visited-set bounds the walk to the
                         * actual chain length rather than relying solely on
                         * MAX_BLOCK_ORPHANS. */
                        std::set<uint1024_t> setVisited;

                        while(nDepth < MAX_BLOCK_ORPHANS)
                        {
                            if(!setVisited.insert(hashCurrent).second)
                                break; /* cycle detected in the orphan graph */

                            const TAO::Ledger::Block* pBlock = mapOrphans.Get(hashCurrent);
                            if(!pBlock)
                                break; /* gap — missing link in the orphan chain */

                            hashDeepestAncestor = pBlock->hashPrevBlock;

                            if(LLD::Ledger->HasBlock(pBlock->hashPrevBlock))
                            {
                                /* Clone so we can use it after releasing the lock,
                                 * then remove it from mapOrphans: Process() itself
                                 * treats any hash already present in mapOrphans as
                                 * a known ORPHAN and returns immediately without
                                 * running Check()/Accept(), so leaving the entry
                                 * in place would make the subsequent Process()
                                 * call below a no-op. */
                                pConnectable.reset(pBlock->Clone());
                                mapOrphans.Remove(hashCurrent);
                                break;
                            }

                            hashCurrent = pBlock->hashPrevBlock;
                            ++nDepth;
                        }
                    }
                }
                /* PROCESSING_MUTEX is released here — safe to call Process()
                 * or PushMessage below. */

                if(!fInOrphanPool)
                {
                    debug::log(0, FUNCTION,
                        ANSI_COLOR_BRIGHT_YELLOW, "=== PEER_BEST_RECOVERY ===", ANSI_COLOR_RESET,
                        " source=", (pszSource ? pszSource : "peer"),
                        " peer_best=", hashPeerBest.SubString(),
                        " peer_height=", nPeerHeight,
                        " not_on_disk=true in_orphan_pool=no",
                        " action=block-not-yet-received");
                    return false;
                }

                if(pConnectable)
                {
                    /* A connectable ancestor was found: feed it through the
                     * normal acceptance path.  If it is accepted the BFS orphan
                     * drain will connect all descendants up to hashPeerBest. */
                    debug::warning(FUNCTION,
                        ANSI_COLOR_BRIGHT_YELLOW, "=== PEER_BEST_RECOVERY ===", ANSI_COLOR_RESET,
                        " source=", (pszSource ? pszSource : "peer"),
                        " peer_best=", hashPeerBest.SubString(),
                        " peer_height=", nPeerHeight,
                        " not_on_disk=true in_orphan_pool=yes",
                        " connectable=", pConnectable->GetHash().SubString(),
                        " action=feeding-connectable-ancestor");

                    uint8_t nStatus = 0;
                    Process(*pConnectable, nStatus, pnode, false);

                    const bool fProgress = (nStatus & PROCESS::ACCEPTED) != 0;
                    if(fProgress)
                        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "=== PEER_BEST_RECOVERED ===",
                            ANSI_COLOR_RESET,
                            " best=", ChainState::hashBestChain.load().SubString(),
                            " height=", ChainState::nBestHeight.load(),
                            " source=orphan-pool-walkback");
                    return fProgress;
                }

                /* Gap in the orphan branch — we have some blocks in memory but not
                 * a contiguous path to disk.  Issue a throttled locator-anchored
                 * LIST so the missing segment can be downloaded, using hashPeerBest
                 * as the stop hash and SPECIFIER::TRANSACTIONS so the peer pushes
                 * inline txs then the block tagged SPECIFIER::TRITIUM.  SYNC would
                 * be rejected as "unsolicited" on an already-synced receiver. */
                debug::warning(FUNCTION,
                    ANSI_COLOR_BRIGHT_YELLOW, "=== PEER_BEST_RECOVERY ===", ANSI_COLOR_RESET,
                    " source=", (pszSource ? pszSource : "peer"),
                    " peer_best=", hashPeerBest.SubString(),
                    " peer_height=", nPeerHeight,
                    " not_on_disk=true in_orphan_pool=yes has_gap=true",
                    " action=locator-branch-sync-request");

                /* Use the calling node; fall back to a random connection. */
                LLP::TritiumNode* pSend = pnode;
                std::shared_ptr<LLP::TritiumNode> pRandom;
                if(!pSend && LLP::TRITIUM_SERVER)
                {
                    pRandom = LLP::TRITIUM_SERVER->RandomConnection();
                    pSend = pRandom.get();
                }

                /* Throttle keyed on the deepest walked ancestor's hashPrevBlock
                 * (the missing ancestor), not hashPeerBest — hashPeerBest is the
                 * branch tip and keying on it would reintroduce the two-namespace
                 * collision this helper was designed to eliminate: the drain-loop
                 * erase(hashParent) cleanup only ever clears hashPrevBlock keys. */
                if(pSend && ShouldSendBranchSyncRequest(hashDeepestAncestor))
                {
                    /* Use SPECIFIER::TRANSACTIONS (not SYNC): this is a post-sync fork-recovery
                     * path.  SYNC blocks are rejected as "unsolicited" once fSynchronized == true;
                     * TRANSACTIONS causes the peer to push inline txs then the block as TRITIUM,
                     * which the receiver accepts unconditionally. */
                    try
                    {
                        pSend->PushMessage(LLP::TritiumNode::ACTION::LIST,
                            config::fClient.load()
                                ? uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)
                                : uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS),
                            uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                            uint8_t(LLP::TritiumNode::TYPES::LOCATOR),
                            TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                            uint1024_t(hashPeerBest)
                        );
                    }
                    catch(const std::exception& e)
                    {
                        debug::error(FUNCTION, e.what());
                    }
                }

                return false;
            }

            LOCK(PROCESSING_MUTEX);

            const TAO::Ledger::BlockState stateBest = ChainState::tStateBest.load();
            if(hashPeerBest == stateBest.GetHash())
                return false;

            if(!statePeer.IsHeavierThan(stateBest))
                return false;

            TAO::Ledger::BlockState stateAncestor;
            uint32_t nConnectDepth = 0;
            uint32_t nDisconnectDepth = 0;
            if(!FindCommonAncestor(statePeer, stateBest, stateAncestor, nConnectDepth, nDisconnectDepth))
                return false;

            debug::warning(FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "=== PEER_BEST_RECOVERY ===", ANSI_COLOR_RESET,
                " source=", (pszSource ? pszSource : "peer"),
                " peer_best=", hashPeerBest.SubString(),
                " peer_height=", statePeer.nHeight,
                " advertised_height=", nPeerHeight,
                " current_best=", stateBest.GetHash().SubString(),
                " current_height=", stateBest.nHeight,
                " ancestor=", stateAncestor.GetHash().SubString(),
                " disconnect=", nDisconnectDepth,
                " connect=", nConnectDepth,
                " action=validated-activation");

            if(!ActivateCandidateBestChain(statePeer, pszSource, true))
                return debug::error(FUNCTION, "peer best recovery candidate validation failed");

            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "=== PEER_BEST_RECOVERED ===", ANSI_COLOR_RESET,
                " best=", ChainState::hashBestChain.load().SubString(),
                " height=", ChainState::nBestHeight.load(),
                " templates=flushed");

            return true;
        }


        bool IsBestChainSynchronized(const uint1024_t& hashPeerBest)
        {
            return hashPeerBest != 0
                && hashPeerBest == ChainState::hashBestChain.load();
        }


        uint32_t MissingBranchRecoveryEscalations(const uint1024_t& hashBlock)
        {
            LOCK(PROCESSING_MUTEX);

            const auto it = mapMissingBranchEscalations.find(hashBlock);
            if(it == mapMissingBranchEscalations.end())
                return 0;

            return it->second;
        }


        bool IsMissingBranchRecoveryCapped(const uint1024_t& hashBlock)
        {
            return MissingBranchRecoveryEscalations(hashBlock)
                > MAX_BRANCH_RECOVERY_ESCALATIONS;
        }


        bool ShouldSendBranchSyncRequest(const uint1024_t& hashAncestor)
        {
            LOCK(PROCESSING_MUTEX);

            /* Throttle: require at least ORPHAN_REQUEST_THROTTLE_SECONDS between
             * LIST requests for the same missing ancestor hash.  Keyed by the
             * ancestor hash (hashPrevBlock of the requesting block) so the drain-
             * loop cleanup mapLastOrphanRequest.erase(hashParent) is always
             * effective regardless of which code path last wrote the entry. */
            const uint64_t nNow = runtime::timestamp();
            const auto itReq = mapLastOrphanRequest.find(hashAncestor);
            if(itReq != mapLastOrphanRequest.end()
            && (nNow - itReq->second) < ORPHAN_REQUEST_THROTTLE_SECONDS)
                return false;

            /* Bound the throttle map size before inserting. */
            if(mapLastOrphanRequest.size() >= MAX_ORPHAN_REQUEST_MAP_ENTRIES)
                mapLastOrphanRequest.clear();

            mapLastOrphanRequest[hashAncestor] = nNow;
            return true;
        }


        void PurgeOrphanRecoveryState(const char* pszReason)
        {
            LOCK(PROCESSING_MUTEX);

            /* Clear the orphan graph and all correlated recovery state.  This is
             * called when the orphan pool is flushed as a DoS guard so that
             * blacklist and escalation entries computed against the now-discarded
             * orphan graph do not persist and mis-filter legitimate future blocks. */
            mapOrphans.Clear();
            setUnrecoverableBlocks.clear();
            mapMissingTxCache.clear();
            mapLastMissingProcessTime.clear();
            mapLastMissing.clear();
            mapMissingBranchEscalations.clear();
            mapLastOrphanRequest.clear();

            debug::warning(FUNCTION, "purged orphan recovery state",
                " reason=", (pszReason ? pszReason : "unknown"),
                " orphan_pool=cleared",
                " setUnrecoverableBlocks=cleared",
                " mapLastMissing=cleared",
                " mapMissingBranchEscalations=cleared",
                " mapLastMissingProcessTime=cleared",
                " mapLastOrphanRequest=cleared");
        }

        /* Maximum number of unique incomplete-block hashes tracked in
         * mapLastMissing before the map is cleared to bound memory use. */
        /* MAX_MISSING_MAP_ENTRIES is declared in include/process.h */


        /* Processes a block incoming over the network. */
        static uint64_t nProcessedBlocks = 0;
        void Process(const TAO::Ledger::Block& block, uint8_t &nStatus, LLP::TritiumNode* pnode, bool fSkipCheck)
        {
            LOCK(PROCESSING_MUTEX);

            /* Get the block's hash. */
            const uint1024_t hashBlock = block.GetHash();

            const auto incrementMissingEscalations = [](const uint1024_t& hash)
            {
                if(mapMissingBranchEscalations.size() >= MAX_MISSING_ESCALATION_MAP_ENTRIES
                && !mapMissingBranchEscalations.count(hash))
                    mapMissingBranchEscalations.clear();

                /* PROCESSING_MUTEX is already held for Process(). */
                ++mapMissingBranchEscalations[hash];
            };

            /* We want to catch any exceptions that were thrown during processing and set REJECTED if exceptions are thrown. */
            try
            {
                /* 0. Terminal blacklist: blocks that have exhausted all branch-
                 *    recovery paths are rejected before any expensive LLD, orphan-
                 *    pool, or Check() work.  This directly addresses the
                 *    DataThread time-budget overrun storm caused by multiple peers
                 *    continuously re-serving an unrecoverable block.
                 *
                 *    Report INCOMPLETE with hashMissing = 0 (not a silent IGNORED)
                 *    so the LLP capped-path branch is reached on every arrival, not
                 *    just the one where the blacklist entry was first inserted.
                 *    That keeps ShouldSendBranchSyncRequest() in the loop — its own
                 *    throttle bounds the actual outgoing traffic — instead of going
                 *    permanently silent after a single recovery attempt. Check() and
                 *    the escalation counter are still skipped entirely. */
                if(setUnrecoverableBlocks.count(hashBlock))
                {
                    nStatus |= PROCESS::INCOMPLETE;
                    block.hashMissing = 0;

                    /* Check() is skipped on this path, so block.vMissing would
                     * otherwise be empty and disable the LLP layer's per-tx
                     * fanout recovery.  Repopulate it from the cache captured
                     * at the moment this hash was blacklisted. */
                    const auto itCache = mapMissingTxCache.find(hashBlock);
                    if(itCache != mapMissingTxCache.end())
                        block.vMissing = itCache->second;

                    return;
                }

                /* 1. Rate-limit reprocessing of known-incomplete blocks.
                 *    When a block is already in mapLastMissing, it is an
                 *    INCOMPLETE block we have processed before and are still
                 *    waiting on.  Guard re-entry into the expensive Check()
                 *    path within a short window so PROCESSING_MUTEX is not
                 *    taken dozens of times per second when multiple peers keep
                 *    re-serving the same stuck block.
                 *
                 *    Important: this guard runs before duplicate / orphan
                 *    checks so that a truly-resolved block (now on disk) still
                 *    takes the normal DUPLICATE or ACCEPTED path — the first
                 *    thing Process() does after this guard is LLD::Ledger->
                 *    HasBlock(), which will catch blocks that were accepted
                 *    concurrently and clear their mapLastMissing entry. */
                if(mapLastMissing.count(hashBlock))
                {
                    const uint64_t nNow = runtime::timestamp(true);
                    const auto itTime = mapLastMissingProcessTime.find(hashBlock);
                    if(itTime != mapLastMissingProcessTime.end()
                    && (nNow - itTime->second) < MISSING_REPROCESS_RATE_LIMIT_MS)
                    {
                        nStatus |= PROCESS::INCOMPLETE;
                        return;
                    }
                    /* Update the rate-limit timestamp before calling Check(). */
                    if(mapLastMissingProcessTime.size() >= MAX_MISSING_MAP_ENTRIES)
                        mapLastMissingProcessTime.clear();
                    mapLastMissingProcessTime[hashBlock] = nNow;
                }

                /* Duplicate suppression is keyed by the block's own hash and must
                 * happen before parent availability is considered. */
                if(LLD::Ledger->HasBlock(hashBlock))
                {
                    nStatus |= PROCESS::DUPLICATE;
                    mapOrphans.Remove(hashBlock);
                    mapLastMissing.erase(hashBlock);
                    mapMissingBranchEscalations.erase(hashBlock);
                    setUnrecoverableBlocks.erase(hashBlock);
                    mapMissingTxCache.erase(hashBlock);
                    mapLastMissingProcessTime.erase(hashBlock);
                    return;
                }

                if(mapOrphans.Contains(hashBlock))
                {
                    nStatus |= PROCESS::ORPHAN;
                    return;
                }

                /* Check for orphan. */
                if(!LLD::Ledger->HasBlock(block.hashPrevBlock))
                {
                    /* Set the status message. */
                    nStatus |= PROCESS::ORPHAN;

                    /* Check the checkpoint height.  Upstream Nexusoft/LLL-TAO
                     * gates this skip behind -checkpoints so operators can opt
                     * in to strict height-based orphan rejection.  Without the
                     * gate, any orphan below nCheckpointHeight would be silently
                     * IGNORED during a deep reorg, potentially discarding
                     * legitimate branch blocks. */
                    if(!config::fTestNet.load()
                    && config::GetBoolArg("-checkpoints", false)
                    && block.nHeight < TAO::Ledger::ChainState::nCheckpointHeight)
                    {
                        /* Emit a single diagnostic so operators know why this
                         * block was discarded rather than silently dropped.
                         * Addresses the "stranded-state" defect class: local
                         * chain state treated as authoritative, no recovery signal. */
                        debug::warning(FUNCTION,
                            "=== STRANDED_STATE_DETECTED === block discarded: height below checkpoint",
                            " block_height=", block.nHeight,
                            " checkpoint_height=", TAO::Ledger::ChainState::nCheckpointHeight.load(),
                            " hash=", hashBlock.SubString(),
                            " class=INVALID_ABSOLUTE (checkpoint enforcement)");

                        nStatus |= PROCESS::IGNORED;
                        return;
                    }

                    uint1024_t hashEvicted = 0;
                    mapOrphans.Insert(block, &hashEvicted);

                    if(hashEvicted != 0)
                        debug::warning(FUNCTION, "orphan pool evicted oldest hash=",
                            hashEvicted.SubString(), " size=", mapOrphans.Size());

                    if(!ChainState::Synchronizing())
                        debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight,
                            " prev=", block.hashPrevBlock.SubString(),
                            " size=", mapOrphans.Size());

                    /* Check if we have an active node. */
                    if(pnode)
                    {
                        /* [C2] Throttle repeated LIST re-requests for the same
                         * missing ancestor. During a fork/orphan storm, many
                         * descendant blocks (from one or several peers) can all
                         * resolve to the same missing hashPrevBlock; without this
                         * guard every single one triggers another LIST round-trip,
                         * competing for DataThread/socket time with the real work
                         * of converging the chain tip (which is what actually
                         * resolves the orphan and unblocks fresh mining templates). */
                        const uint64_t nNow = runtime::timestamp();

                        bool fShouldRequest = true;
                        const auto itThrottle = mapLastOrphanRequest.find(block.hashPrevBlock);
                        if(itThrottle != mapLastOrphanRequest.end()
                        && (nNow - itThrottle->second) < ORPHAN_REQUEST_THROTTLE_SECONDS)
                            fShouldRequest = false;

                        if(fShouldRequest)
                        {
                            /* Bound the map size before inserting a new entry, same
                             * cheap DoS guard rationale as mapLastMissing. */
                            if(mapLastOrphanRequest.size() >= MAX_ORPHAN_REQUEST_MAP_ENTRIES)
                                mapLastOrphanRequest.clear();
                            mapLastOrphanRequest[block.hashPrevBlock] = nNow;

                            if(config::GetBoolArg("-syncorphans", false))
                            {
                                /* Direct-fetch fallback: targeted GET for the specific
                                 * missing ancestor block (upstream delta: restore the
                                 * commented-out ACTION::GET recovery route behind a
                                 * config flag so the direct-fetch path is available
                                 * when the locator-sync is too broad). */
                                pnode->PushMessage(LLP::TritiumNode::ACTION::GET,
                                    uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                                    block.hashPrevBlock);
                            }
                            else
                            {
                                /* Full locator-anchored branch sync.  Use SPECIFIER::TRANSACTIONS
                                 * (not SYNC) so the peer pushes inline txs then the block as
                                 * SPECIFIER::TRITIUM.  SYNC blocks are rejected as "unsolicited"
                                 * once the receiving node has completed initial sync
                                 * (fSynchronized == true), which is precisely when orphan-based
                                 * fork recovery fires. */
                                pnode->PushMessage(LLP::TritiumNode::ACTION::LIST,
                                    config::fClient.load()
                                        ? uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)
                                        : uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS),
                                    uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                                    uint8_t(LLP::TritiumNode::TYPES::LOCATOR),
                                    TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                    uint1024_t(block.hashPrevBlock)
                                );

                                /* Random-connection fallback: ask a second distinct
                                 * peer so the node that sent the orphan cannot also
                                 * be the sole source of the recovery path (upstream
                                 * delta: RandomConnection fallback for orphan LIST). */
                                if(LLP::TRITIUM_SERVER)
                                {
                                    std::shared_ptr<LLP::TritiumNode> pRandom =
                                        LLP::TRITIUM_SERVER->RandomConnection();
                                    if(pRandom && pRandom.get() != pnode)
                                    {
                                        try
                                        {
                                            /* Same TRANSACTIONS specifier — receiver is synced. */
                                            pRandom->PushMessage(LLP::TritiumNode::ACTION::LIST,
                                                config::fClient.load()
                                                    ? uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)
                                                    : uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS),
                                                uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                                                uint8_t(LLP::TritiumNode::TYPES::LOCATOR),
                                                TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                                uint1024_t(block.hashPrevBlock)
                                            );
                                        }
                                        catch(const std::exception& e)
                                        {
                                            debug::error(FUNCTION, e.what());
                                        }
                                    }
                                }
                            }
                        }
                    }

                    return;
                }

                /* Check if the block is valid. Skip when already validated by ValidateMinedBlock(). */
                if(!fSkipCheck && !block.Check(true))
                {
                    /* [Option C] Negative/retry cache for Check()-rejected blocks.
                     * Count consecutive rejections for this exact block hash. */
                    uint32_t nRejects = 1;
                    const auto itCheck = mapCheckRejects.find(hashBlock);
                    if(itCheck != mapCheckRejects.end())
                        nRejects = ++itCheck->second;
                    else
                    {
                        /* Bound the map size before inserting a new entry, same
                         * cheap DoS guard rationale as mapLastMissing. */
                        if(mapCheckRejects.size() >= MAX_CHECK_REJECT_MAP_ENTRIES)
                            mapCheckRejects.clear();
                        mapCheckRejects[hashBlock] = nRejects;
                    }

                    /* After a small number of natural rejections for the identical
                     * block hash, a genuinely invalid/malicious block would still
                     * fail identically, so repeated failure of the *same* hash is
                     * a strong signal of transient local mempool corruption (a
                     * stale conflicted transaction) rather than a bad block. Force
                     * the mempool's disk-based conflict-reconciliation pass to run
                     * out of band (normally only triggered after a successful
                     * Accept()) and retry Check() once. Re-trigger on every
                     * multiple of the threshold (not just the first time) so a
                     * block that keeps arriving from other peers after a failed
                     * resync attempt still gets periodic recovery attempts,
                     * rather than being silently rejected forever once nRejects
                     * passes the threshold. */
                    bool fRecovered = false;
                    if(nRejects % CHECK_REJECT_RESYNC_THRESHOLD == 0)
                    {
                        debug::warning(FUNCTION, "block ", hashBlock.SubString(), " failed Check() ", nRejects,
                            " times; forcing targeted mempool resync and retrying");

                        mempool.Check();

                        if(block.Check(true))
                        {
                            fRecovered = true;
                            debug::log(0, FUNCTION, "block ", hashBlock.SubString(),
                                " recovered via targeted mempool resync after ", nRejects, " Check() failures");
                        }
                    }

                    if(!fRecovered)
                    {
                        nStatus |= PROCESS::REJECTED;

                        /* Escalate to the sending peer once a block has failed
                         * well beyond our resync attempts: if disk-backed
                         * reconciliation didn't resolve it, this is most likely a
                         * genuinely invalid block rather than local corruption, so
                         * penalize the peer that sent it. */
                        if(pnode && nRejects >= CHECK_REJECT_BAN_THRESHOLD)
                        {
                            if(pnode->fDDOS.load() && pnode->DDOS)
                                pnode->DDOS->rSCORE += CHECK_REJECT_DDOS_SCORE;

                            mapCheckRejects.erase(hashBlock);
                        }

                        return;
                    }

                    /* Recovered via targeted resync: drop the counter so a future
                     * genuine failure of a different block starts counting fresh,
                     * and fall through to process this block normally. */
                    mapCheckRejects.erase(hashBlock);
                }


                /* Check for missing transactions. A missing transaction is a
                 * temporary "incomplete" condition, not a validation failure: the
                 * transactions are re-requested and the block re-processed once
                 * they arrive. */
                if(block.vMissing.size() != 0)
                {
                    /* Incomplete blocks can pass through orphan checks. */
                    nStatus |= PROCESS::INCOMPLETE;

                    /* Set the missing block. */
                    block.hashMissing = hashBlock;

                    /* Track how many times we have re-requested this block's
                     * missing transactions so a permanently unresolvable tx can't
                     * wedge the node forever. */
                    if(mapLastMissing.count(hashBlock))
                    {
                        mapLastMissing[hashBlock]++;

                        /* Increment and check if we have reached limits. */
                        if(mapLastMissing[hashBlock] > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                        {
                            /* Check whether we have already reached the branch-
                             * recovery escalation cap for this block.  When capped,
                             * do NOT increment the escalation counter again (that is
                             * what caused escalations=207 vs cap=3 in production):
                             * the cap is now truly terminal.  Instead, add the block
                             * to the hard blacklist so future arrivals are rejected
                             * at the top of Process() without any expensive work,
                             * and emit a single FORK_WEDGE_DETECTED warning. */
                            const uint32_t nEscalations =
                                mapMissingBranchEscalations.count(hashBlock)
                                ? mapMissingBranchEscalations[hashBlock] : 0;

                            if(nEscalations >= MAX_BRANCH_RECOVERY_ESCALATIONS)
                            {
                                /* Only emit the wedge warning, insert into the
                                 * blacklist, and increment the escalation counter
                                 * the first time we see the capped condition for
                                 * this hash.  Incrementing once here is intentional:
                                 * it advances the counter to exactly MAX+1 so that
                                 * IsMissingBranchRecoveryCapped() returns true and
                                 * the cap-invariant tests can assert a finite upper
                                 * bound.  Subsequent arrivals are dropped by the
                                 * setUnrecoverableBlocks guard at the top of
                                 * Process() before reaching this code, so the
                                 * counter can never exceed MAX+1. */
                                if(!setUnrecoverableBlocks.count(hashBlock))
                                {
                                    if(setUnrecoverableBlocks.size() >= MAX_UNRECOVERABLE_ENTRIES)
                                        setUnrecoverableBlocks.clear();
                                    setUnrecoverableBlocks.insert(hashBlock);

                                    /* Cache the missing-tx hash list now, while
                                     * block.vMissing is still populated from this
                                     * call's Check(), so future arrivals that hit
                                     * the terminal-blacklist early return can still
                                     * hand it to the LLP layer's per-tx fanout. */
                                    if(mapMissingTxCache.size() >= MAX_UNRECOVERABLE_ENTRIES)
                                        mapMissingTxCache.clear();
                                    mapMissingTxCache[hashBlock] = block.vMissing;

                                    /* Advance counter to MAX+1 and clean up the
                                     * per-tx retry map — same cleanup as the normal
                                     * escalation path. */
                                    incrementMissingEscalations(hashBlock);
                                    mapLastMissing.erase(hashBlock);

                                    debug::warning(FUNCTION,
                                        ANSI_COLOR_BRIGHT_RED,
                                        "=== FORK_WEDGE_DETECTED ===",
                                        ANSI_COLOR_RESET,
                                        " local_height=", block.nHeight,
                                        " block=", hashBlock.SubString(),
                                        " escalations=", nEscalations + 1,
                                        " cap=", MAX_BRANCH_RECOVERY_ESCALATIONS,
                                        " missing_tx_count=", block.vMissing.size(),
                                        " — ancestor-anchored branch sync triggered;"
                                        " no further per-tx spam will be emitted for this block");
                                    for(const auto& missing : block.vMissing)
                                        debug::warning(FUNCTION,
                                            "  missing_tx=", missing.second.SubString());
                                }

                                /* Set hashMissing = 0 so the LLP layer sends an
                                 * ancestor-anchored branch sync (escalation path)
                                 * rather than the per-tx re-request (normal path).
                                 * The blacklist entry will suppress future arrivals
                                 * from the very top of Process(). */
                                block.hashMissing = 0;
                            }
                            else
                            {
                                /* Normal escalation: erase the per-tx retry
                                 * counter so the next arrival starts a fresh
                                 * cycle, and signal the LLP layer to escalate
                                 * to full branch recovery. */
                                debug::warning(FUNCTION,
                                    "missing-tx retry limit exceeded for block ",
                                    hashBlock.SubString(),
                                    " height=", block.nHeight,
                                    "; resetting retry counter and escalating to branch recovery");

                                incrementMissingEscalations(hashBlock);
                                mapLastMissing.erase(hashBlock);
                                block.hashMissing = 0;
                            }
                        }

                        /* Give some debug info that we are missing some transactions here. */
                        else
                            debug::notice(FUNCTION, "missing ", block.vMissing.size(), " transactions");
                    }
                    else
                    {
                        /* Bound the map size before inserting a new entry.
                         * Clearing the whole map is an intentional cheap DoS
                         * guard; legitimate incomplete blocks will have resolved
                         * long before 10 000 unique hashes accumulate. */
                        if(mapLastMissing.size() >= MAX_MISSING_MAP_ENTRIES)
                            mapLastMissing.clear();
                        mapLastMissing[hashBlock] = 1;

                        /* Seed the rate-limit timestamp on the very first miss
                         * too, not just on subsequent re-entries.  The guard at
                         * the top of Process() only checks/updates this map when
                         * mapLastMissing already has an entry for hashBlock —
                         * which is not yet true on this, the first, arrival —
                         * so without seeding it here a second rapid arrival
                         * would find no timestamp and incorrectly run the full
                         * path again instead of being rate-limited. */
                        if(mapLastMissingProcessTime.size() >= MAX_MISSING_MAP_ENTRIES)
                            mapLastMissingProcessTime.clear();
                        mapLastMissingProcessTime[hashBlock] = runtime::timestamp(true);
                    }

                    return;
                }

                /* Check if valid in the chain. */
                else
                {
                    /* Print the block if it gets this far into processing. */
                    if(config::nVerbose >= 2)
                        debug::log(2, block.ToString());

                    /* Attempt to accept block when we have all the transactions. */
                    if(!block.Accept())
                    {
                        /* Set the status. */
                        nStatus |= PROCESS::REJECTED;

                        return;
                    }
                }

                /* Set the status. */
                nStatus |= PROCESS::ACCEPTED;

                /* Block accepted — remove any missing-transaction retry counter so
                 * the genuine "transaction not seen yet" recovery path is
                 * unaffected and a future stuck block starts counting from zero.
                 * Also clear the rate-limit timestamp and blacklist entry so if
                 * this exact hash ever re-appears (e.g. a reorg that reverses a
                 * previously unrecoverable decision) it is processed normally. */
                if(mapLastMissing.count(hashBlock))
                    mapLastMissing.erase(hashBlock);
                mapMissingBranchEscalations.erase(hashBlock);
                setUnrecoverableBlocks.erase(hashBlock);
                mapMissingTxCache.erase(hashBlock);
                mapLastMissingProcessTime.erase(hashBlock);

                /* Special meter for synchronizing. */
                uint64_t nElapsed = runtime::timestamp(true) - nSynchronizationTimer;
                if(nElapsed > 3000 && TAO::Ledger::ChainState::Synchronizing())
                {
                    /* Grab the current sync node. */
                    uint32_t nHours = 0, nMinutes = 0, nSeconds = 0;
                    if(LLP::TritiumNode::Syncing())
                    {
                        /* Get the current connected legacy node. */
                        std::shared_ptr<LLP::TritiumNode> pnode = LLP::TritiumNode::GetNode(nSyncSession.load());
                        try //we want to catch exceptions thrown by atomic_ptr in the case there was a free on another thread
                        {
                            /* Check for potential overflow if current height is not set. */
                            if(pnode && pnode->nCurrentHeight > ChainState::nBestHeight.load())
                            {
                                /* Get the total height left to go. */
                                uint32_t nRemaining = (pnode->nCurrentHeight - ChainState::nBestHeight.load());
                                uint32_t nTotalBlocks = (ChainState::nBestHeight.load() - LLP::TritiumNode::nSyncStart.load());

                                /* Calculate blocks per second. */
                                uint32_t nRate = nTotalBlocks / (LLP::TritiumNode::SYNCTIMER.Elapsed() + 1);
                                LLP::TritiumNode::nRemainingTime.store(nRemaining / (nRate + 1));

                                /* Get the remaining time. */
                                nHours   =  LLP::TritiumNode::nRemainingTime.load() / 3600;
                                nMinutes = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) / 60;
                                nSeconds = (LLP::TritiumNode::nRemainingTime.load() - (nHours * 3600)) % 60;
                            }
                        }
                        catch(const std::exception& e) {}
                    }

                    /* Debug output now. */
                    nProcessedBlocks = (ChainState::nBestHeight.load() - nProcessedBlocks);
                    debug::log(0, FUNCTION,
                        "Processed ", (nProcessedBlocks), " blocks in ", nElapsed, " ms [", std::setw(2),
                        TAO::Ledger::ChainState::PercentSynchronized(), " %]",
                        " height=", block.nHeight,
                        " [", (nProcessedContracts.load() * 1000) / nElapsed, " contracts/s]",
                        "[", std::setw(2), std::setfill('0'), nHours, ":",
                              std::setw(2), std::setfill('0'), nMinutes, ":",
                              std::setw(2), std::setfill('0'), nSeconds, " remaining]");

                    nSynchronizationTimer = runtime::timestamp(true);
                    nProcessedContracts   = 0;
                    nProcessedBlocks      = ChainState::nBestHeight.load();
                }

                /* Drain the orphan graph breadth-first. */
                std::queue<uint1024_t> queueParents;
                queueParents.push(hashBlock);

                bool fRecordedIncomplete = false;
                uint64_t nDrained = 0;
                while(!queueParents.empty())
                {
                    const uint1024_t hashParent = queueParents.front();
                    queueParents.pop();

                    const std::vector<uint1024_t> vChildren = mapOrphans.Children(hashParent);
                    for(const auto& hashOrphan : vChildren)
                    {
                        const TAO::Ledger::Block* pOrphan = mapOrphans.Get(hashOrphan);
                        if(!pOrphan)
                            continue;

                        if(LLD::Ledger->HasBlock(hashOrphan))
                        {
                            mapOrphans.Remove(hashOrphan);
                            queueParents.push(hashOrphan);
                            ++nDrained;
                            continue;
                        }

                        debug::log(0, FUNCTION, "processing ORPHAN hash=",
                            hashOrphan.SubString(), " size=", mapOrphans.Size());

                        if(!pOrphan->Check(true))
                        {
                            const uint64_t nPruned = mapOrphans.RemoveSubtree(hashOrphan);
                            mapLastMissing.erase(hashOrphan);
                            mapMissingBranchEscalations.erase(hashOrphan);
                            setUnrecoverableBlocks.erase(hashOrphan);
                            mapMissingTxCache.erase(hashOrphan);
                            mapLastMissingProcessTime.erase(hashOrphan);
                            debug::warning(FUNCTION, "removed invalid orphan subtree root=",
                                hashOrphan.SubString(), " count=", nPruned);
                            continue;
                        }

                        /* Retain incomplete children while processing independent
                         * siblings. Expose the first miss in deterministic order. */
                        if(!pOrphan->vMissing.empty())
                        {
                            nStatus |= PROCESS::INCOMPLETE;
                            if(!fRecordedIncomplete)
                            {
                                block.vMissing.insert(block.vMissing.end(),
                                    pOrphan->vMissing.begin(), pOrphan->vMissing.end());
                                block.hashMissing = hashOrphan;
                                fRecordedIncomplete = true;

                                if(mapLastMissing.count(hashOrphan))
                                    ++mapLastMissing[hashOrphan];
                                else
                                {
                                    if(mapLastMissing.size() >= MAX_MISSING_MAP_ENTRIES)
                                        mapLastMissing.clear();
                                    mapLastMissing[hashOrphan] = 1;

                                    /* Seed the rate-limit timestamp on the first
                                     * miss here too, mirroring the primary path:
                                     * without it, a subsequent top-level Process()
                                     * arrival of this same orphan hash would find
                                     * no mapLastMissingProcessTime entry and run
                                     * the full path again instead of being
                                     * rate-limited. */
                                    if(mapLastMissingProcessTime.size() >= MAX_MISSING_MAP_ENTRIES)
                                        mapLastMissingProcessTime.clear();
                                    mapLastMissingProcessTime[hashOrphan] = runtime::timestamp(true);
                                }

                                if(mapLastMissing[hashOrphan] >
                                    LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                                {
                                    /* Apply the same cap-terminal logic as the
                                     * primary missing-tx path: once the escalation
                                     * cap is reached for an orphan, add it to the
                                     * blacklist and do not increment further. */
                                    const uint32_t nEscalations =
                                        mapMissingBranchEscalations.count(hashOrphan)
                                        ? mapMissingBranchEscalations[hashOrphan] : 0;

                                    if(nEscalations >= MAX_BRANCH_RECOVERY_ESCALATIONS)
                                    {
                                        if(!setUnrecoverableBlocks.count(hashOrphan))
                                        {
                                            if(setUnrecoverableBlocks.size() >= MAX_UNRECOVERABLE_ENTRIES)
                                                setUnrecoverableBlocks.clear();
                                            setUnrecoverableBlocks.insert(hashOrphan);

                                            /* Cache the missing-tx list, mirroring the
                                             * primary path, so the terminal-blacklist
                                             * early return can still populate vMissing
                                             * on future arrivals. */
                                            if(mapMissingTxCache.size() >= MAX_UNRECOVERABLE_ENTRIES)
                                                mapMissingTxCache.clear();
                                            mapMissingTxCache[hashOrphan] = pOrphan->vMissing;

                                            /* Advance counter to MAX+1 and clean up the
                                             * per-tx retry map — mirrors the primary path. */
                                            incrementMissingEscalations(hashOrphan);
                                            mapLastMissing.erase(hashOrphan);

                                            debug::warning(FUNCTION,
                                                ANSI_COLOR_BRIGHT_RED,
                                                "=== FORK_WEDGE_DETECTED ===",
                                                ANSI_COLOR_RESET,
                                                " orphan local_height=", pOrphan->nHeight,
                                                " block=", hashOrphan.SubString(),
                                                " escalations=", nEscalations + 1,
                                                " cap=", MAX_BRANCH_RECOVERY_ESCALATIONS,
                                                " missing_tx_count=", pOrphan->vMissing.size());
                                            for(const auto& missing : pOrphan->vMissing)
                                                debug::warning(FUNCTION,
                                                    "  missing_tx=", missing.second.SubString());
                                        }
                                        block.hashMissing = 0;
                                    }
                                    else
                                    {
                                        /* Erase the entry so the next arrival of this
                                         * orphan child starts a fresh retry cycle rather
                                         * than being permanently silenced, and signal
                                         * the LLP layer to escalate to branch recovery. */
                                        debug::warning(FUNCTION,
                                            "missing-tx retry limit exceeded for orphan ",
                                            hashOrphan.SubString(),
                                            " height=", pOrphan->nHeight,
                                            "; resetting retry counter and escalating to branch recovery");

                                        incrementMissingEscalations(hashOrphan);
                                        mapLastMissing.erase(hashOrphan);
                                        block.hashMissing = 0;
                                    }
                                }
                                else
                                    debug::notice(FUNCTION, "orphan missing ",
                                        pOrphan->vMissing.size(), " transactions");
                            }
                            continue;
                        }

                        if(!pOrphan->Accept())
                        {
                            const uint64_t nPruned = mapOrphans.RemoveSubtree(hashOrphan);
                            mapLastMissing.erase(hashOrphan);
                            mapMissingBranchEscalations.erase(hashOrphan);
                            setUnrecoverableBlocks.erase(hashOrphan);
                            mapMissingTxCache.erase(hashOrphan);
                            mapLastMissingProcessTime.erase(hashOrphan);
                            debug::warning(FUNCTION, "removed rejected orphan subtree root=",
                                hashOrphan.SubString(), " count=", nPruned);
                            continue;
                        }

                        mapLastMissing.erase(hashOrphan);
                        mapMissingBranchEscalations.erase(hashOrphan);
                        setUnrecoverableBlocks.erase(hashOrphan);
                        mapMissingTxCache.erase(hashOrphan);
                        mapLastMissingProcessTime.erase(hashOrphan);
                        mapLastOrphanRequest.erase(hashParent);
                        mapOrphans.Remove(hashOrphan);
                        queueParents.push(hashOrphan);
                        ++nDrained;
                    }
                }

                if(nDrained > 0)
                    debug::log(0, FUNCTION, "drained ", nDrained,
                        " orphan block(s), remaining=", mapOrphans.Size());
            }
            catch(const std::exception& e)
            {
                nStatus |= PROCESS::REJECTED;
                return;
            }
        }
    }
}
