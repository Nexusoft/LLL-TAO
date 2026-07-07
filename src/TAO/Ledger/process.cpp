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
#include <TAO/Ledger/include/stagepool.h>

#include <TAO/Ledger/types/locator.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/types/legacy.h>

#include <Util/include/runtime.h>

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


        /* [Option C] Timestamp of the most recent missing-transaction retry per
         * block hash. See declaration comment in include/process.h. */
        std::map<uint1024_t, uint64_t> mapLastMissingStamp;


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


        /* Mutex to protect checking more than one block at a time. */
        std::mutex PROCESSING_MUTEX;


        /* Sync timer value. */
        uint64_t nSynchronizationTimer = 0;


        /* Current sync node. */
        std::atomic<uint64_t> nSyncSession(0);


        /* Stats variable for syncing. */
        std::atomic<uint64_t> nProcessedContracts(0);


        /* Maximum number of unique incomplete-block hashes tracked in
         * mapLastMissing before the map is cleared to bound memory use. */
        /* MAX_MISSING_MAP_ENTRIES is declared in include/process.h */


        /* Processes a block incoming over the network. */
        static uint64_t nProcessedBlocks = 0;


        /* [Option C] Increment (or start, or decay-reset) the missing-transaction
         * retry counter for the given block hash, stamping the attempt time. */
        uint64_t UpdateMissingRetry(const uint1024_t& hashBlock)
        {
            const uint64_t nNow = runtime::timestamp();

            if(mapLastMissing.count(hashBlock))
            {
                /* Decay: if the last attempt is older than the cool-down window,
                 * reset the counter so recovery can resume instead of the block
                 * being silently dead-ended forever. */
                if(mapLastMissingStamp.count(hashBlock)
                && nNow > mapLastMissingStamp[hashBlock] + MISSING_RETRY_DECAY_SECONDS)
                {
                    debug::notice(FUNCTION, "retry counter for block ", hashBlock.SubString(),
                        " decayed after ", MISSING_RETRY_DECAY_SECONDS, "s; resuming missing-transaction requests");

                    mapLastMissing[hashBlock] = 1;
                }
                else
                    mapLastMissing[hashBlock]++;
            }
            else
            {
                /* Bound the map size before inserting a new entry.  Clearing the
                 * whole map is an intentional cheap DoS guard; legitimate
                 * incomplete blocks will have resolved long before 10 000 unique
                 * hashes accumulate. */
                if(mapLastMissing.size() >= MAX_MISSING_MAP_ENTRIES)
                {
                    mapLastMissing.clear();
                    mapLastMissingStamp.clear();
                }

                mapLastMissing[hashBlock] = 1;
            }

            /* Stamp this attempt. */
            mapLastMissingStamp[hashBlock] = nNow;

            return mapLastMissing[hashBlock];
        }


        /* [Option C] Check whether retries are exhausted and still cooling down. */
        bool MissingRetryExhausted(const uint1024_t& hashBlock)
        {
            const auto it = mapLastMissing.find(hashBlock);
            if(it == mapLastMissing.end())
                return false;

            /* Not exhausted yet. */
            if(it->second <= LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                return false;

            /* Exhausted, but past the cool-down: allow recovery to resume. */
            const auto itStamp = mapLastMissingStamp.find(hashBlock);
            if(itStamp == mapLastMissingStamp.end()
            || runtime::timestamp() > itStamp->second + MISSING_RETRY_DECAY_SECONDS)
                return false;

            return true;
        }


        /* [Option C] Remove all retry bookkeeping for the given block hash. */
        void EraseMissingRetry(const uint1024_t& hashBlock)
        {
            mapLastMissing.erase(hashBlock);
            mapLastMissingStamp.erase(hashBlock);
        }


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

                /* [Option C] Short-circuit repeated relays of a block whose
                 * missing-transaction retries are exhausted and still inside the
                 * decay cool-down.  Without this, every relay of the stuck block
                 * re-runs the full (expensive) block.Check() just to rediscover
                 * the same missing transactions, starving DataThreads (the
                 * "time budget exceeded" log spam).  Once the cool-down expires,
                 * processing (and re-requesting) resumes automatically. */
                if(!fSkipCheck && MissingRetryExhausted(hashBlock))
                {
                    nStatus |= PROCESS::INCOMPLETE;

                    /* hashMissing stays 0: the LLP layer must not re-request
                     * until the cool-down has expired. */
                    block.hashMissing = 0;
                    block.vMissing.clear();

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

                    /* [Option A] Register the missing tritium txids with the
                     * block-context staging pool so that when they arrive but are
                     * rejected by tip-relative mempool policy (e.g. "coinbase is
                     * immature"), the LLP layer can stage them for block-context
                     * validation instead of dropping them (which deadlocks block
                     * completion). Registration is safe: vMissing is only
                     * populated by a block that passed every other Check()
                     * validation, including proof of work. */
                    for(const auto& missing : block.vMissing)
                    {
                        if(missing.first == TAO::Ledger::TRANSACTION::TRITIUM)
                            StagePool::Register(missing.second);
                    }

                    /* [Option C] Track how many times we have re-requested this
                     * block's missing transactions so a permanently unresolvable
                     * tx can't wedge the node forever.  The counter time-decays
                     * (see UpdateMissingRetry) so hitting the cap is a temporary
                     * cool-down, not a permanent silent dead-end. */
                    const uint64_t nRetries = UpdateMissingRetry(hashBlock);

                    /* Check if we have reached limits. */
                    if(nRetries > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                    {
                        block.vMissing.clear(); //we want to clear so we don't keep re-requesting the transactions
                        block.hashMissing = 0;

                        /* Escalate visibility on the transition into exhaustion:
                         * a chain-wedging condition must not be silent. */
                        if(nRetries == LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1)
                            debug::notice(FUNCTION, "block ", hashBlock.SubString(), " exhausted ",
                                uint32_t(LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES),
                                " missing-transaction retries; cooling down for ",
                                MISSING_RETRY_DECAY_SECONDS, "s before retrying");
                    }

                    /* Give some debug info that we are missing some transactions here. */
                    else
                        debug::notice(FUNCTION, "missing ", block.vMissing.size(), " transactions");

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
                EraseMissingRetry(hashBlock);

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

                        /* [Option A] Register missing tritium txids for
                         * block-context staging, same as the main path. */
                        for(const auto& missing : pOrphan->vMissing)
                        {
                            if(missing.first == TAO::Ledger::TRANSACTION::TRITIUM)
                                StagePool::Register(missing.second);
                        }

                        /* [Option C] Track retries so a permanently unresolvable
                         * orphan tx can't wedge the node forever; the counter
                         * time-decays so exhaustion is a temporary cool-down. */
                        const uint64_t nRetries = UpdateMissingRetry(hashPrev);
                        if(nRetries > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
                        {
                            block.vMissing.clear();
                            block.hashMissing = 0;

                            if(nRetries == LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1)
                                debug::notice(FUNCTION, "orphan ", hashPrev.SubString(), " exhausted ",
                                    uint32_t(LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES),
                                    " missing-transaction retries; cooling down for ",
                                    MISSING_RETRY_DECAY_SECONDS, "s before retrying");
                        }
                        else
                            debug::notice(FUNCTION, "orphan missing ", pOrphan->vMissing.size(), " transactions");

                        return;
                    }

                    /* Accept each orphan. */
                    else if(!pOrphan->Accept())
                        return;

                    /* Orphan accepted — clear any missing-transaction retry counter. */
                    EraseMissingRetry(hashPrev);

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
