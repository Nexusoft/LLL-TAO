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

#include <TAO/Ledger/types/locator.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/types/legacy.h>

#include <Util/include/runtime.h>

#include <vector>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* Static instantiation of orphan blocks in queue to process. */
        std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapOrphans;


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


            /** Maximum number of peer-best recovery cooldown entries tracked. */
            static const uint64_t MAX_PEER_BEST_RECOVERY_RECORDS = 256;


            /** Peer-best recovery cooldown; mirrors autofork recovery's tested bound. */
            static const uint64_t PEER_BEST_RECOVERY_COOLDOWN_SECONDS =
                GENESIS_CONFLICT_RECOVERY_COOLDOWN_SECONDS;


            /** Last automatic peer-best recovery attempt by advertised best hash. */
            std::map<uint1024_t, uint64_t> mapLastPeerBestRecoveryAttempt;


            /** Locally accepted mined blocks keyed by block hash. */
            std::map<uint1024_t, LocalMinedBlockRecord> mapLocalMinedBlocks;


            /** Mutex protecting local mined/recovery maps. */
            std::mutex LOCAL_MINED_MUTEX;


            bool BetterThanCurrentBest(const TAO::Ledger::BlockState& statePeer,
                                       const TAO::Ledger::BlockState& stateBest)
            {
                if(statePeer.GetHash() == stateBest.GetHash())
                    return false;

                static const uint32_t MIN_TRITIUM_WEIGHT_VERSION = 7;
                if(statePeer.nVersion >= MIN_TRITIUM_WEIGHT_VERSION && !statePeer.IsHybrid())
                {
                    uint8_t nEquals  = 0;
                    uint8_t nGreater = 0;

                    for(uint32_t n = 0; n < 3; ++n)
                    {
                        if(statePeer.nChannelWeight[n] == stateBest.nChannelWeight[n])
                            ++nEquals;

                        if(statePeer.nChannelWeight[n] > stateBest.nChannelWeight[n])
                            ++nGreater;
                    }

                    /* Mirrors BlockState::Accept(): in a two-channel battle,
                     * a branch that is more than one unified height ahead gets
                     * one extra "greater" vote so the heavier branch can win. */
                    if(statePeer.nHeight > stateBest.nHeight + 1
                    && (nEquals == 1 && nGreater == 1))
                        ++nGreater;

                    return ((nEquals == 2 && nGreater == 1) || nGreater > 1);
                }

                return statePeer.nChainTrust > stateBest.nChainTrust;
            }


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


            void EvictOldestPeerBestCooldown()
            {
                if(mapLastPeerBestRecoveryAttempt.empty())
                    return;

                auto itOldest = mapLastPeerBestRecoveryAttempt.begin();
                for(auto it = mapLastPeerBestRecoveryAttempt.begin();
                    it != mapLastPeerBestRecoveryAttempt.end(); ++it)
                {
                    if(it->second < itOldest->second)
                        itOldest = it;
                }

                mapLastPeerBestRecoveryAttempt.erase(itOldest);
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

            if(!BetterThanCurrentBest(statePeer, stateBest))
                return false;

            TAO::Ledger::BlockState stateAncestor;
            uint32_t nConnectDepth = 0;
            uint32_t nDisconnectDepth = 0;
            if(!FindCommonAncestor(statePeer, stateBest, stateAncestor, nConnectDepth, nDisconnectDepth))
                return false;

            if(nDisconnectDepth > MAX_AUTO_FORK_RECOVERY_DEPTH)
            {
                debug::warning(FUNCTION,
                    "peer best recovery refused: disconnect depth ", nDisconnectDepth,
                    " exceeds cap ", MAX_AUTO_FORK_RECOVERY_DEPTH,
                    " peer_best=", hashPeerBest.SubString(),
                    " current_best=", stateBest.GetHash().SubString());
                return false;
            }

            const uint64_t nNow = runtime::timestamp();
            {
                std::lock_guard<std::mutex> lock(LOCAL_MINED_MUTEX);
                const auto itCooldown = mapLastPeerBestRecoveryAttempt.find(hashPeerBest);
                if(itCooldown != mapLastPeerBestRecoveryAttempt.end()
                && (nNow - itCooldown->second) < PEER_BEST_RECOVERY_COOLDOWN_SECONDS)
                    return false;

                if(mapLastPeerBestRecoveryAttempt.size() >= MAX_PEER_BEST_RECOVERY_RECORDS)
                    EvictOldestPeerBestCooldown();
                mapLastPeerBestRecoveryAttempt[hashPeerBest] = nNow;
            }

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
                " action=SetBest");

            if(!statePeer.SetBest())
                return debug::error(FUNCTION, "peer best recovery failed to set best chain");

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
                /* Check for orphan. */
                if(!LLD::Ledger->HasBlock(block.hashPrevBlock))
                {
                    /* Set the status message. */
                    nStatus |= PROCESS::ORPHAN;

                    /* Skip if already in orphan queue. */
                    if(!mapOrphans.count(block.hashPrevBlock))
                    {
                        /* Check the checkpoint height. */
                        if(!config::fTestNet.load() && block.nHeight < TAO::Ledger::ChainState::nCheckpointHeight)
                        {
                            /* Set the status. */
                            nStatus |= PROCESS::IGNORED;

                            return;
                        }

                        /* Insert into orphans map. */
                        mapOrphans.insert
                        (
                            std::make_pair(block.hashPrevBlock,
                            std::unique_ptr<TAO::Ledger::Block>(block.Clone()))
                        );

                        /* Debug output. */
                        if(!ChainState::Synchronizing())
                            debug::log(0, FUNCTION, "ORPHAN height=", block.nHeight, " prev=", block.hashPrevBlock.SubString());
                    }

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

                /* Check if this is a duplicate block. */
                if(LLD::Ledger->HasBlock(block.GetHash()))
                {
                    nStatus |= PROCESS::DUPLICATE;
                    return;
                }

                /* Check if the block is valid. Skip when already validated by ValidateMinedBlock(). */
                if(!fSkipCheck && !block.Check())
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

                        if(block.Check())
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

                /* Process orphan if found. */
                uint1024_t hash = block.GetHash();
                while(mapOrphans.count(hash))
                {
                    /* Grab local copy of the pointer. */
                    const std::unique_ptr<TAO::Ledger::Block>& pOrphan = mapOrphans.at(hash);

                    /* Get the next hash backwards in the series. */
                    const uint1024_t hashPrev = pOrphan->GetHash();

                    /* Check if this is a duplicate block. */
                    if(LLD::Ledger->HasBlock(pOrphan->GetHash()))
                        continue;

                    /* Debug output. */
                    debug::log(0, FUNCTION, "processing ORPHAN prev=", hashPrev.SubString(), " size=", mapOrphans.size());

                    /* Check if the block is valid. */
                    if(!pOrphan->Check())
                        return;

                    /* Check for missing transactions for ORPHAN. A missing tx is
                     * a temporary incomplete condition, not a validation failure. */
                    if(pOrphan->vMissing.size() != 0)
                    {
                        /* Incomplete blocks can pass through orphan checks. */
                        nStatus |= PROCESS::INCOMPLETE;

                        /* Add the missing transactions to this current block. */
                        block.vMissing.insert(block.vMissing.end(), pOrphan->vMissing.begin(), pOrphan->vMissing.end());

                        /* Set hashMissing to the orphan's own hash so the LLP
                         * layer re-requests the correct block (not the map key). */
                        block.hashMissing = hashPrev;

                        /* Track retries so a permanently unresolvable orphan tx
                         * can't wedge the node forever. */
                        if(mapLastMissing.count(hashPrev))
                        {
                            mapLastMissing[hashPrev]++;

                            if(mapLastMissing[hashPrev] > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                            {
                                block.vMissing.clear();
                                block.hashMissing = 0;
                            }
                            else
                                debug::notice(FUNCTION, "orphan missing ", pOrphan->vMissing.size(), " transactions");
                        }
                        else
                        {
                            /* Same intentional clear-all DoS guard as the main path. */
                            if(mapLastMissing.size() >= MAX_MISSING_MAP_ENTRIES)
                                mapLastMissing.clear();
                            mapLastMissing[hashPrev] = 1;
                        }

                        return;
                    }

                    /* Accept each orphan. */
                    else if(!pOrphan->Accept())
                        return;

                    /* Orphan accepted — clear any missing-transaction retry counter. */
                    if(mapLastMissing.count(hashPrev))
                        mapLastMissing.erase(hashPrev);

                    /* [C2] Orphan resolved — clear its request throttle entry so a
                     * future, unrelated orphan chain starting at this same hash
                     * (unlikely, but cheap to guard) isn't throttled by stale
                     * bookkeeping. */
                    if(mapLastOrphanRequest.count(hash))
                        mapLastOrphanRequest.erase(hash);

                    /* Erase orphans from map. */
                    mapOrphans.erase(hash);
                    hash = hashPrev;
                }
            }
            catch(const std::exception& e)
            {
                nStatus |= PROCESS::REJECTED;
                return;
            }
        }
    }
}
