/*__________________________________________________________________________________________

		Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

		(c) Copyright The Nexus Developers 2014 - 2025

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

                return block.Check(true) && block.vMissing.empty()
                    && !block.fConflicted;
            }
        }


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

            if(fTransaction)
                LLD::TxnCommit(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);

            debug::log(0, FUNCTION, "activated validated candidate source=",
                (pszSource ? pszSource : "ledger"),
                " hash=", stateCandidate.GetHash().SubString(),
                " connect=", nConnectDepth,
                " disconnect=", nDisconnectDepth);
            return true;
        }


        bool AttemptPeerBestChainRecovery(const uint1024_t& hashPeerBest,
                                          uint32_t nPeerHeight,
                                          const char* pszSource)
        {
            if(hashPeerBest == 0 || hashPeerBest == ChainState::hashBestChain.load())
                return false;

            if(!config::GetBoolArg("-peerbestchainrecovery", true))
                return false;

            TAO::Ledger::BlockState statePeer;
            if(!LLD::Ledger->ReadBlock(hashPeerBest, statePeer))
                return false;

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

            /* We want to catch any exceptions that were thrown during processing and set REJECTED if exceptions are thrown. */
            try
            {
                /* Duplicate suppression is keyed by the block's own hash and must
                 * happen before parent availability is considered. */
                if(LLD::Ledger->HasBlock(hashBlock))
                {
                    nStatus |= PROCESS::DUPLICATE;
                    mapOrphans.Remove(hashBlock);
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

                    /* Check the checkpoint height. */
                    if(!config::fTestNet.load() && block.nHeight < TAO::Ledger::ChainState::nCheckpointHeight)
                    {
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

                            /* Ask for list of blocks. */
                            pnode->PushMessage(LLP::TritiumNode::ACTION::LIST,
                                uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                                uint8_t(LLP::TritiumNode::TYPES::LOCATOR),
                                TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                uint1024_t(block.hashPrevBlock)
                            );
                        }

                        /* Send a request to download the orphaned block.
                        pnode->PushMessage(LLP::TritiumNode::ACTION::GET,

                            #ifndef DEBUG_MISSING
                            uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS),
                            #endif

                            uint8_t(LLP::TritiumNode::TYPES::BLOCK), block.hashPrevBlock);
                        */
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
                            block.vMissing.clear(); //we want to clear so we don't keep re-requesting the transactions
                            block.hashMissing = 0;
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
                    }

                    return;
                }

                /* Check if valid in the chain. */
                else if(!block.Accept())
                {
                    /* Set the status. */
                    nStatus |= PROCESS::REJECTED;

                    return;
                }

                /* Set the status. */
                nStatus |= PROCESS::ACCEPTED;

                /* Block accepted — remove any missing-transaction retry counter so
                 * the genuine "transaction not seen yet" recovery path is
                 * unaffected and a future stuck block starts counting from zero. */
                if(mapLastMissing.count(hashBlock))
                    mapLastMissing.erase(hashBlock);

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
                                }

                                if(mapLastMissing[hashOrphan] >
                                    LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                                {
                                    block.vMissing.clear();
                                    block.hashMissing = 0;
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
                            debug::warning(FUNCTION, "removed rejected orphan subtree root=",
                                hashOrphan.SubString(), " count=", nPruned);
                            continue;
                        }

                        mapLastMissing.erase(hashOrphan);
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
