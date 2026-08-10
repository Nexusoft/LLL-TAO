/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_PROCESS_H
#define NEXUS_TAO_LEDGER_INCLUDE_PROCESS_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/block.h>

#include <map>
#include <list>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

namespace LLP { class TritiumNode; }

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        class BlockState;


        /** Enum values for returning the block's state after processing. **/
        namespace PROCESS
        {
            enum
            {
                ORPHAN     = (1 << 1), //has no previous block
                DUPLICATE  = (1 << 2), //already in database
                ACCEPTED   = (1 << 3), //processed fully
                REJECTED   = (1 << 4), //block was rejected
                IGNORED    = (1 << 5), //ignore protocol requests
                INCOMPLETE = (1 << 6), //block contains missing transactions
            };
        }


        /** Maximum number of block orphans retained globally. **/
        static const uint64_t MAX_BLOCK_ORPHANS = 10000;


        /** OrphanPool
         *
         *  Bounded block-orphan graph indexed by both the orphan's own hash and
         *  its parent hash. Callers serialize access with PROCESSING_MUTEX.
         *
         **/
        class OrphanPool
        {
            std::map<uint1024_t, std::unique_ptr<TAO::Ledger::Block>> mapByHash;
            std::map<uint1024_t, std::set<uint1024_t>> mapByParent;
            std::list<uint1024_t> listInsertionOrder;
            std::map<uint1024_t, std::list<uint1024_t>::iterator> mapInsertion;

        public:
            bool Insert(const TAO::Ledger::Block& block, uint1024_t* pHashEvicted = nullptr);
            bool Contains(const uint1024_t& hashBlock) const;
            const TAO::Ledger::Block* Get(const uint1024_t& hashBlock) const;
            std::vector<uint1024_t> Children(const uint1024_t& hashParent) const;
            bool Remove(const uint1024_t& hashBlock);
            uint64_t RemoveSubtree(const uint1024_t& hashRoot);
            void Clear();
            uint64_t Size() const;
            bool Empty() const;
        };


        /** Static instantiation of the block orphan pool. **/
        extern OrphanPool mapOrphans;


        /** Track the times we have requested missing transactions for a block so
         *  we don't keep re-requesting the same unresolvable transactions. **/
        extern std::map<uint1024_t, uint64_t> mapLastMissing;

        /** Track the number of full branch-recovery escalation cycles that
         *  have occurred for a missing-transaction block hash. This counter is
         *  incremented each time mapLastMissing[hash] exceeds
         *  MAX_MISSING_TRANSACTIONS_RETRIES and is erased/reset only when the
         *  block (or orphan child) is eventually accepted or pruned. **/
        extern std::map<uint1024_t, uint32_t> mapMissingBranchEscalations;


        /** Rate-limit tracking: last timestamp (milliseconds since epoch) at
         *  which each known-incomplete block hash was fully processed in
         *  Process(). Guards re-entry into the expensive Check() + escalation
         *  path within a short window so PROCESSING_MUTEX is not taken dozens
         *  of times per second per peer for the same stuck block (directly
         *  addresses the DataThread time-budget overruns seen during fork-wedge
         *  conditions). **/
        extern std::map<uint1024_t, uint64_t> mapLastMissingProcessTime;


        /** Minimum number of milliseconds that must elapse between successive
         *  full-path reprocessing attempts for the same known-incomplete block
         *  hash.  Arrivals within this window return INCOMPLETE immediately
         *  without invoking Check() or updating any counters. **/
        static const uint64_t MISSING_REPROCESS_RATE_LIMIT_MS = 250;


        /** Hard terminal blacklist for blocks that have exhausted all
         *  branch-recovery paths.  Once a block hash is added here, Process()
         *  returns IGNORED immediately — before any LLD, orphan-pool, or
         *  Check() work — so the DataThread budget is not consumed on an
         *  unrecoverable block.  An entry is cleared if the block is later
         *  accepted (e.g. after a reorg that makes the missing tx available)
         *  or the orphan pool is purged. **/
        extern std::set<uint1024_t> setUnrecoverableBlocks;


        /** Maximum number of unique block hashes held in setUnrecoverableBlocks
         *  before the set is cleared.  Same intentional cheap DoS-guard
         *  rationale as MAX_MISSING_MAP_ENTRIES above. **/
        static const uint64_t MAX_UNRECOVERABLE_ENTRIES = 10000;


        /** Cache of the missing-transaction hash list captured the moment a
         *  block hash is inserted into setUnrecoverableBlocks (the last call
         *  where Check() actually ran and populated block.vMissing).  The
         *  terminal-blacklist early return at the top of Process() skips
         *  Check() entirely on every subsequent arrival, which would otherwise
         *  leave block.vMissing empty and disable the LLP layer's per-tx
         *  fanout recovery path.  Process() repopulates block.vMissing from
         *  this cache before returning so the fanout still has hashes to work
         *  with. Bounded to MAX_UNRECOVERABLE_ENTRIES and cleared alongside
         *  setUnrecoverableBlocks. **/
        extern std::map<uint1024_t, std::vector<std::pair<uint8_t, uint512_t> > > mapMissingTxCache;


        /** Maximum number of unique incomplete-block hashes tracked in
         *  mapLastMissing before the entire map is cleared to bound memory use.
         *  When the map reaches this size and a new (unseen) block hash would be
         *  inserted, the map is wiped and the new entry starts fresh.  This is an
         *  intentional cheap DoS guard: clearing all counters at once is O(n) but
         *  avoids the complexity of LRU eviction.  The vast majority of legitimate
         *  incomplete blocks will have resolved long before the cap is hit. **/
        static const uint64_t MAX_MISSING_MAP_ENTRIES = 10000;

        /** Maximum number of unique block hashes tracked in
         *  mapMissingBranchEscalations before the map is cleared. **/
        static const uint64_t MAX_MISSING_ESCALATION_MAP_ENTRIES = 10000;

        /** Maximum number of full branch-recovery escalation cycles allowed for
         *  a given block hash before LLP suppresses further branch-recovery
         *  network requests and emits an operator-facing warning. **/
        static const uint32_t MAX_BRANCH_RECOVERY_ESCALATIONS = 3;


        /** [C2] Track the last time we asked peers for an orphan ancestor's chain
         *  (keyed by the missing hashPrevBlock), so a fork/orphan storm that keeps
         *  delivering blocks descending from the same unknown ancestor doesn't
         *  cause a LIST re-request to be pushed out on every single one of those
         *  blocks. This is a lightweight fast path: it does not change what gets
         *  validated, only how often we re-announce that we're still missing the
         *  same ancestor, freeing DataThread/socket time for genuine chain-tip
         *  convergence during the storm. **/
        extern std::map<uint1024_t, uint64_t> mapLastOrphanRequest;


        /** Minimum number of seconds between LIST re-requests for the same missing
         *  orphan ancestor. **/
        static const uint64_t ORPHAN_REQUEST_THROTTLE_SECONDS = 3;


        /** Maximum number of unique ancestor hashes tracked in mapLastOrphanRequest
         *  before the map is cleared. Same intentional cheap DoS guard rationale as
         *  MAX_MISSING_MAP_ENTRIES above. **/
        static const uint64_t MAX_ORPHAN_REQUEST_MAP_ENTRIES = 10000;


        /** [Option C] Track consecutive Check()-rejections for the identical
         *  block hash. A genuinely invalid/malicious block fails Check() the
         *  same way on every attempt, but a block that is otherwise valid can
         *  spuriously fail sequencing checks if the local mempool holds a stale
         *  conflicted transaction for one of the sigchains involved (see the
         *  disk-backed self-heal in TritiumBlock::Check()). This counter lets
         *  Process() distinguish "give up immediately" (rare, targeted resync)
         *  from "this peer is sending us garbage" (escalate / ban). **/
        extern std::map<uint1024_t, uint32_t> mapCheckRejects;


        /** Number of consecutive Check() failures for the same block hash
         *  before we force an out-of-band mempool conflict-reconciliation pass
         *  (normally only triggered after a successful block Accept()) and
         *  retry Check() once. Re-checked on every multiple of this threshold
         *  (not just once) so a block that keeps arriving from new peers after
         *  the first resync attempt failed still gets periodic recovery
         *  attempts instead of being silently rejected forever. **/
        static const uint32_t CHECK_REJECT_RESYNC_THRESHOLD = 3;


        /** Number of consecutive Check() failures for the same block hash
         *  (including the one-shot resync attempts above) before the sending
         *  peer is treated as suspect and penalized via its DDOS score. **/
        static const uint32_t CHECK_REJECT_BAN_THRESHOLD = 6;


        /** DDOS score penalty applied to a peer once CHECK_REJECT_BAN_THRESHOLD
         *  is reached. Matches the penalty already used elsewhere in the LLP
         *  layer (see LLP::TritiumNode) for other "sent us something invalid
         *  repeatedly" conditions. **/
        static const uint32_t CHECK_REJECT_DDOS_SCORE = 50;


        /** Maximum number of unique block hashes tracked in mapCheckRejects
         *  before the map is cleared. Same intentional cheap DoS guard
         *  rationale as MAX_MISSING_MAP_ENTRIES above. **/
        static const uint64_t MAX_CHECK_REJECT_MAP_ENTRIES = 10000;


        /** Track the times we have requested processed missing transactions so we don't loop too much. **/
        extern std::map<uint1024_t, uint64_t> mapLastMissing;


        /** Mutex to protect checking more than one block at a time. **/
        extern std::mutex PROCESSING_MUTEX;


        /** Sync timer value. **/
        extern uint64_t nSynchronizationTimer;


        /** Current sync node. **/
        extern std::atomic<uint64_t> nSyncSession;


        /* Stats variable for syncing. */
        extern std::atomic<uint64_t> nProcessedContracts;

        /** Process Block Function
         *
         *  Processes a block incoming over the network.
         *
         *  @param[in] block The block being processed
         *  @param[out] nStatus The status flags returned.
         *  @param[out] pnode The node that block came from.
         *  @param[in] fSkipCheck Skip the block.Check() call when the block has
         *             already been validated by ValidateMinedBlock() prior to
         *             calling this function. Avoids redundant PoW verification
         *             for locally-mined blocks. Default is false (full validation).
         *
         **/
        void Process(const TAO::Ledger::Block& block, uint8_t &nStatus, LLP::TritiumNode* pnode = nullptr, bool fSkipCheck = false);


        /** TrackLocalMinedAcceptedBlock
         *
         *  Records a locally mined block that was accepted so later same-height
         *  sibling reorgs can emit explicit diagnostics and recovery logs.
         *
         **/
        void TrackLocalMinedAcceptedBlock(const TAO::Ledger::Block& block);


        /** MarkLocalMinedBlockDisconnected
         *
         *  Called during SetBest() when a block is disconnected.  If the block
         *  was locally mined, logs the orphaning event and removes it from the
         *  local-mined watch set.
         *
         **/
        void MarkLocalMinedBlockDisconnected(const TAO::Ledger::BlockState& state,
                                             const TAO::Ledger::BlockState& stateNewBest);


        /** FinalizeLocalMinedTrackingAfterSetBest
         *
         *  Prunes locally mined records that remained on the best chain after a
         *  later SetBest() and logs that they no longer need orphan tracking.
         *
         **/
        void FinalizeLocalMinedTrackingAfterSetBest(const TAO::Ledger::BlockState& stateNewBest);


        /** PurgeOrphanRecoveryState
         *
         *  Atomically clears the orphan pool and **all** correlated recovery
         *  state under PROCESSING_MUTEX, so that entries in the blacklist or
         *  the escalation / rate-limit maps that were computed against a now-
         *  discarded orphan graph do not persist as stale data.
         *
         *  Call this whenever the orphan pool is emptied as a DoS guard
         *  (e.g. the nConsecutiveOrphans >= 10 000 flush in the LLP layer).
         *
         *  @param[in] pszReason  Short label written to the warning log.
         *
         **/
        void PurgeOrphanRecoveryState(const char* pszReason = nullptr);


        /** ShouldSendBranchSyncRequest
         *
         *  Throttle-gated check for whether a locator-anchored branch-sync LIST
         *  should be sent for the missing ancestor identified by hashAncestor.
         *
         *  Canonical key: the *missing ancestor* hash (typically the orphan
         *  block's own hashPrevBlock).  Using hashPrevBlock as the key ensures
         *  that the orphan-drain BFS cleanup `mapLastOrphanRequest.erase(
         *  hashParent)` always removes entries regardless of which code path
         *  last wrote them, and that the throttle semantics are uniform across
         *  both the ledger and the LLP layer.
         *
         *  Returns true when at least ORPHAN_REQUEST_THROTTLE_SECONDS have
         *  elapsed since the last request for this ancestor (or no prior
         *  request exists), and records the current timestamp when returning
         *  true.
         *
         *  This helper owns its own PROCESSING_MUTEX lock.  Callers must NOT
         *  hold PROCESSING_MUTEX when calling this function.
         *
         *  @param[in] hashAncestor  The missing-ancestor hash to throttle on.
         *  @return true if a branch-sync LIST should be pushed now.
         *
         **/
        bool ShouldSendBranchSyncRequest(const uint1024_t& hashAncestor);


        /** Outcome of AttemptPeerBestChainRecovery.
         *
         *  Distinguishes "fetch not appropriate / not attempted" from "fetch
         *  already queued" and "fetch suppressed by the branch-sync throttle"
         *  so callers with a fallback LIST path (RequestMissingTxBranchRecovery)
         *  do not defeat ShouldSendBranchSyncRequest() by treating throttle
         *  denial as a green light for an unthrottled second LIST.
         **/
        enum class PeerBestRecoveryResult : uint8_t
        {
            SKIPPED,          /* early-out, disabled, or no action taken      */
            PROGRESS,         /* local best chain advanced                    */
            FETCH_QUEUED,     /* locator LIST successfully queued             */
            FETCH_THROTTLED,  /* would fetch, but ORPHAN_REQUEST throttle hit */
        };


        /** AttemptPeerBestChainRecovery
         *
         *  Recovery for cases where peers advertise a different known best hash.
         *  The advertised height is diagnostic only; activation requires a
         *  complete, fully checked, strictly heavier candidate branch.
         *
         *  When the peer's tip is not on disk:
         *    - If it IS in the orphan pool, walk the orphan graph backwards
         *      (capped at MAX_BLOCK_ORPHANS depth) to find the deepest ancestor
         *      whose own hashPrevBlock is on disk.  If a connectable ancestor is
         *      found it is fed through Process() so the existing BFS drain can
         *      connect the chain forward.
         *    - If it is NOT in the orphan pool (large gap), OR the orphan walk
         *      finds no connectable ancestor, a throttled locator-anchored
         *      branch-sync LIST (SPECIFIER::TRANSACTIONS) is issued via pnode
         *      (or a random connection if pnode is nullptr), with a one-peer
         *      fanout to a second distinct peer when available.
         *
         *  @param[in] pnode  Optional sending node.  If nullptr, falls back to
         *                    a TRITIUM_SERVER->RandomConnection() for the
         *                    branch-sync request.  Must NOT hold
         *                    PROCESSING_MUTEX.
         *  @param[out] pfBranchSyncQueued  Optional.  Set true when this helper
         *                    successfully queued a primary locator LIST on pnode
         *                    (or its random fallback).  Prefer the returned
         *                    PeerBestRecoveryResult for orchestration: a false
         *                    out-param alone cannot distinguish throttle denial
         *                    from "fallback LIST is still appropriate".
         *
         *  @return PeerBestRecoveryResult describing progress / fetch / throttle.
         *
         **/
        PeerBestRecoveryResult AttemptPeerBestChainRecovery(
                                          const uint1024_t& hashPeerBest,
                                          uint32_t nPeerHeight,
                                          const char* pszSource = nullptr,
                                          LLP::TritiumNode* pnode = nullptr,
                                          bool* pfBranchSyncQueued = nullptr);


        /** RequestMissingTxBranchRecovery
         *
         *  Missing-tx escalation coordination used when Process() has exhausted
         *  per-tx retries for an incomplete block:
         *
         *    1. Call AttemptPeerBestChainRecovery when hashPeerBest is a known
         *       foreign tip (may queue one locator LIST + TxResponseWindow on
         *       pnode).
         *    2. If step 1 was SKIPPED (not FETCH_QUEUED / FETCH_THROTTLED /
         *       PROGRESS), queue the same locator LIST on pnode as a fallback
         *       gated by ShouldSendBranchSyncRequest (stop hash = hashPeerBest
         *       if non-zero, else hashBlock).
         *
         *  Extracted so unit tests can exercise the combined path and assert
         *  that a successful or throttled recovery LIST is not followed by a
         *  duplicate fallback LIST (which would replace the peer's
         *  TxResponseWindow and defeat the three-second request throttle).
         *
         *  @param[in]  hashPeerBest  Peer's advertised best-chain hash (0 if unknown).
         *  @param[in]  hashBlock     Incomplete block that exhausted per-tx retries.
         *  @param[in]  nPeerHeight   Diagnostic peer height for recovery logs.
         *  @param[in]  pszSource     Log source tag.
         *  @param[in]  pnode         Peer that advertised the incomplete block.
         *  @param[out] pfBranchSyncQueued  Optional. Set true when either step
         *                    successfully queued a primary locator LIST on pnode.
         *
         *  @return true only when AttemptPeerBestChainRecovery reported PROGRESS.
         *
         **/
        bool RequestMissingTxBranchRecovery(const uint1024_t& hashPeerBest,
                                            const uint1024_t& hashBlock,
                                            uint32_t nPeerHeight,
                                            const char* pszSource,
                                            LLP::TritiumNode* pnode,
                                            bool* pfBranchSyncQueued = nullptr);


        /** RequestBestChainBranchRecovery
         *
         *  BESTCHAIN-notify coordination (TIP-01 / TIP-02):
         *
         *    1. Call AttemptPeerBestChainRecovery when policy allows:
         *       - known on-disk tips are always evaluated for heavier-chain
         *         activation (peer height is not a gate for that path);
         *       - unknown tips are fetched only when the peer is at or ahead
         *         of local height (historical BESTCHAIN height gate).  This
         *         prevents a behind peer's unknown hash from queuing LIST +
         *         TxResponseWindow + fanout + throttle before the fallback
         *         height check can run.
         *    2. Fallback LIST only when step 1 returned SKIPPED, the advertised
         *       tip is not yet the active local best, the peer is at/ahead of
         *       local height, and ShouldSendBranchSyncRequest allows it.
         *
         *  FETCH_QUEUED / FETCH_THROTTLED / PROGRESS all suppress the fallback
         *  LIST so chatty BESTCHAIN cannot thrash TxResponseWindow or defeat the
         *  three-second branch-sync throttle (the residual asymmetry left after
         *  #690/#691, which only hardened the missing-tx escalation path).
         *
         *  @param[in]  hashPeerBest  Peer's advertised best-chain hash.
         *  @param[in]  nPeerHeight   Peer height from the BESTCHAIN notify context.
         *  @param[in]  pszSource     Log source tag.
         *  @param[in]  pnode         Notifying peer (required for LIST paths).
         *  @param[out] pfBranchSyncQueued  Optional. Set true when a primary
         *                    locator LIST was successfully queued on pnode.
         *
         *  @return true only when AttemptPeerBestChainRecovery reported PROGRESS.
         *
         **/
        bool RequestBestChainBranchRecovery(const uint1024_t& hashPeerBest,
                                            uint32_t nPeerHeight,
                                            const char* pszSource,
                                            LLP::TritiumNode* pnode,
                                            bool* pfBranchSyncQueued = nullptr);


        /** IsBestChainSynchronized
         *
         *  Returns true only when the advertised peer best is the active local
         *  best-chain hash. Merely having the block on disk is insufficient.
         *
         **/
        bool IsBestChainSynchronized(const uint1024_t& hashPeerBest);

        /** Returns the current missing-tx branch-recovery escalation count for
         *  hashBlock. **/
        uint32_t MissingBranchRecoveryEscalations(const uint1024_t& hashBlock);

        /** Returns true once missing-tx branch-recovery escalation count has
         *  exceeded MAX_BRANCH_RECOVERY_ESCALATIONS for hashBlock. **/
        bool IsMissingBranchRecoveryCapped(const uint1024_t& hashBlock);


        /** ActivateCandidateBestChain
         *
         *  Validates a complete, strictly heavier candidate path and activates
         *  it through SetBest(). The transactional form is used by peer recovery;
         *  normal block acceptance already owns a ledger transaction.
         *
         **/
        bool ActivateCandidateBestChain(const TAO::Ledger::BlockState& stateCandidate,
                                        const char* pszSource,
                                        bool fTransaction);

    }
}

#endif
