/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H
#define NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H

#include <LLC/types/uint1024.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/state.h>
#include <Legacy/types/transaction.h>

#include <Legacy/types/outpoint.h>

#include <Util/include/mutex.h>

namespace LLP
{
    class TritiumNode;
}

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Maximum number of unique conflicted transaction hashes
         *  tracked in Mempool::mapConflicts before the entire map is cleared to
         *  bound memory use. Under normal operation mapConflicts is pruned by
         *  Mempool::Check()'s reconciliation pass (see CONFLICTS_SWEEP_INTERVAL_SECONDS
         *  below for why that pass must not depend solely on new blocks
         *  connecting), but while the chain tip is stalled that pass runs rarely,
         *  allowing peers to relay an unbounded number of distinct conflicting
         *  transactions into this map. This is the same intentional cheap DoS
         *  guard already used for mapLastMissing/mapCheckRejects: clearing all
         *  entries at once is O(n) but avoids the complexity of LRU eviction,
         *  and any conflict wiped here that is still relevant will simply be
         *  re-added the next time the transaction is relayed/requested.
         *  10000 matches the existing MAX_MISSING_MAP_ENTRIES threshold
         *  used for mapLastMissing (see TAO/Ledger/include/process.h):
         *  it is chosen for consistency with that already-established
         *  cap rather than a distinct measurement, since both maps hold
         *  comparably small, transient, hash-keyed entries and are
         *  bounded for the same "cheap DoS guard" reason. **/
        static const uint32_t MAX_CONFLICTS_MAP_ENTRIES = 10000;


        /** Minimum number of seconds between periodic,
         *  chain-tip-independent calls to Mempool::Check() from
         *  TritiumNode's per-connection GENERIC event handler. Check()'s
         *  conflict-reconciliation/eviction pass previously only ran after a
         *  block successfully connected to the best chain, so while the tip is
         *  stalled (e.g. a stuck fork or a weak-network dry spell with no mined
         *  blocks), mapConflicts was never pruned or re-tested against disk.
         *  This periodic sweep is a global cooldown (guarded by a single shared
         *  timestamp, not per-connection) so it fires on a fixed cadence
         *  regardless of how many peers are connected. **/
        static const uint64_t CONFLICTS_SWEEP_INTERVAL_SECONDS = 30;
        /** [Option B] Number of consecutive Mempool::Check() reconciliation
         *  cycles a genesis's conflicted transaction(s) must fail to resolve
         *  against disk before AttemptForkRecovery() is triggered for that
         *  genesis. Check() runs on every accepted block plus periodically, so
         *  20 consecutive misses is well beyond the handful of cycles a normal
         *  transient reorg or mempool ordering race takes to settle on its own
         *  (typically 1-3), while still being low enough to recover promptly
         *  once a genuine structural divergence is detected. **/
        static const uint32_t GENESIS_CONFLICT_RECOVERY_THRESHOLD = 20;


        /** [Option B] Minimum number of seconds between automatic fork-recovery
         *  attempts for the same genesis, win or lose. Check() can run many
         *  times per minute during active sync, so without this cooldown a
         *  genesis whose conflict keeps failing to resolve (or whose rollback
         *  keeps being refused, e.g. because the computed ancestor isn't found
         *  on disk yet) could re-trigger a rollback attempt on almost every
         *  cycle. Ten minutes gives a prior rollback's reorg (disconnect/
         *  connect + mempool resurrection) time to fully settle, and gives a
         *  refused attempt time for the missing ancestor data to potentially
         *  arrive from peers, before trying again. **/
        static const uint64_t GENESIS_CONFLICT_RECOVERY_COOLDOWN_SECONDS = 600;


        /** [Option B] Maximum number of blocks AttemptForkRecovery() is allowed
         *  to roll the best chain back by in a single automatic attempt. This
         *  is a hard safety bound: even if a conflicting transaction's disk
         *  ancestor block is found, a rollback deeper than this is refused and
         *  logged for manual review (e.g. -revertblocks) instead of being
         *  performed automatically. 1440 is a deliberately generous upper bound
         *  (comparable to roughly a day of blocks at a ~1 minute average block
         *  time across channels) chosen to cover realistic sync-stall
         *  divergences while still making a runaway/malicious rollback
         *  (e.g. a spoofed conflicting transaction referencing a very old
         *  ancestor) impossible. **/
        static const uint32_t MAX_AUTO_FORK_RECOVERY_DEPTH = 1440;


        /** [C1] Read-only result of computing how far the best chain has
         *  diverged from a genesis's on-disk expected predecessor
         *  transaction. Populated by Mempool::ComputeForkDivergence(), which
         *  performs no rollback and has no side effects (no cooldown, no
         *  counter mutation), so it is safe to call at any time -- including
         *  from a diagnostic RPC -- to answer "how deep would an
         *  -revertblocks=N need to be right now for this genesis?" without
         *  guessing. **/
        struct ForkDivergenceInfo
        {
            /** True if disk's committed hashLast for the genesis already
             *  matches hashPrevTx, i.e. there is no divergence to report. **/
            bool fResolved = false;

            /** True if the block hosting hashPrevTx could be located on disk. **/
            bool fAncestorFound = false;

            /** True if that ancestor block is part of our current best chain. **/
            bool fAncestorOnMainChain = false;

            /** True if the computed depth exceeds MAX_AUTO_FORK_RECOVERY_DEPTH,
             *  i.e. AttemptForkRecovery() would refuse to auto-roll-back even if
             *  -autoforkrecovery were enabled. **/
            bool fExceedsCap = false;

            /** Disk-committed last transaction hash for the genesis. **/
            uint512_t hashOurLast = 0;

            /** The predecessor transaction hash the conflicting/canonical
             *  transaction expects (input to the computation). **/
            uint512_t hashPrevTx = 0;

            /** Hash of the block that committed hashPrevTx, if found. **/
            uint1024_t hashAncestorBlock = 0;

            /** Full block state of the ancestor block, if found. Cached here
             *  so a caller that proceeds to perform the rollback (e.g.
             *  AttemptForkRecovery()) doesn't need to re-read it from disk. **/
            TAO::Ledger::BlockState stateAncestor;

            /** Height of the ancestor block, if found. **/
            uint32_t nAncestorHeight = 0;

            /** Current best chain height at the time of computation. **/
            uint32_t nBestHeight = 0;

            /** Number of blocks a rollback to the ancestor would disconnect.
             *  Only meaningful when fAncestorFound && fAncestorOnMainChain. **/
            uint32_t nDepth = 0;

            /** Human-readable reason computation could not fully complete
             *  (empty if fResolved or a valid depth/ancestor was computed). **/
            std::string strError;
        };


        /** Mempool
         *
         *  The memory pool class where transactions are stored until they are validated
         *  and added to the ledger.
         *
         **/
        class Mempool
        {
        public:

            /* Mutex to local access to the mempool */
            mutable std::recursive_mutex MUTEX;

        private:

            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, Legacy::Transaction> mapLegacy;


            /** The transactions in conflicted legacy memory pool. */
            std::map<uint512_t, Legacy::Transaction> mapLegacyConflicts;


            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapLedger;


            /** The transactions in the conflicted ledger memory pool. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapConflicts;


            /** [Option 1] Bounds mapConflicts before inserting a new entry: if the
             *  map has reached MAX_CONFLICTS_MAP_ENTRIES, it is cleared first (same
             *  cheap DoS-guard rationale as mapLastMissing/mapCheckRejects). Call
             *  this immediately before every `mapConflicts[hash] = tx;` insertion
             *  so all three call sites in Accept() share one bound. **/
            void BoundConflictsMap();


            /** Oprhan transactions in queue. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapOrphans;


            /** Record of conflicted transactions in mempool. **/
            std::map<uint512_t, uint512_t> mapClaimed;


            /** Record of conflicted transactions in mempool. **/
            std::set<uint512_t> mapRejected;


            /** Record of legacy inputs in the mempool. **/
            std::map<Legacy::OutPoint, uint512_t> mapInputs;


            /** Set to keep track of duplicate orphans by index. **/
            std::set<uint512_t> setOrphansByIndex;


            /** [Option B] Tracks consecutive Check() cycles where a conflicted
             *  genesis's earliest queued transaction still doesn't match the
             *  disk-committed hashLast, keyed by hashGenesis. Unlike the
             *  per-block-hash counters elsewhere (mapCheckRejects), this survives
             *  across the many different orphan/candidate block hashes that a
             *  single stuck sigchain conflict produces, letting us detect a
             *  structurally diverged sigchain that the existing disk-based
             *  self-heal / reconciliation paths cannot resolve on their own. **/
            std::map<uint256_t, uint32_t> mapGenesisConflictMisses;


            /** [Option B] Last time (unix timestamp) an automatic fork-recovery
             *  rollback was attempted for a given genesis. Bounds how often we
             *  retry (and how much chain work we potentially discard) even if
             *  the sigchain keeps conflicting. **/
            std::map<uint256_t, uint64_t> mapLastForkRecoveryAttempt;

        public:

            /** Default Constructor. **/
            Mempool();


            /** Default Destructor. **/
            ~Mempool();


            /** AddUnchecked.
             *
             *  Add a transaction to the memory pool without validation checks.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool AddUnchecked(const TAO::Ledger::Transaction& tx);


            /** AddUnchecked
             *
             *  Add a legacy transaction to the memory pool without validation checks.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool AddUnchecked(const Legacy::Transaction& tx);


            /** Accept
             *
             *  Accepts a transaction with validation rules.
             *
             *  @param[in] tx The transaction to add.
             *  @param[in] pnode The node that transaction is accepted from.
             *
             *  @return true if added.
             *
             **/
            bool Accept(const TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode = nullptr);


            /** Accept
             *
             *  Accepts a legacy transaction with validation rules.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool Accept(const Legacy::Transaction& tx, LLP::TritiumNode* pnode = nullptr);


            /** ProcessOrphans
             *
             *  Process orphan transactions if triggered in queue.
             *
             *  @param[in] hash The hash of spent output
             *
             *  @return true if spent.
             *
             **/
            void ProcessOrphans(const uint512_t& hash);


            /** IsSpent
             *
             *  Checks if a given output is spent in memory.
             *
             *  @param[in] hash The hash of spent output
             *  @param[in] n The output number being checked
             *
             *  @return true if spent.
             *
             **/
            bool IsSpent(const uint512_t& hash, const uint32_t n);


            /** Get
             *
             *  Gets a transaction from mempool including conflicted memory.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The retrieved transaction
             *  @param[out] fConflicted Flag to determine if transaction is conflicted
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted) const;


            /** Get
             *
             *  Gets a transaction from mempool
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The retrieved transaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const;


            /** Get
             *
             *  Gets a transaction by genesis.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] vTx The list of retrieved transaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint256_t& hashGenesis, std::vector<TAO::Ledger::Transaction> &vTx) const;


            /** Get
             *
             *  Gets a transaction by genesis.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The last tx by genesistransaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint256_t& hashGenesis, TAO::Ledger::Transaction &tx) const;


            /** Get
             *
             *  Gets a legacy transaction from mempool
             *
             *  @param[in] hashTx Hash of legacy transaction to get.
             *
             *  @param[out] tx The retrieved legacy transaction
             *  @param[out] fConflicted Flag to determine if transaction is conflicted
             *
             *  @return true if pool contained legacy transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, Legacy::Transaction &tx, bool &fConflicted) const;


            /** Get
             *
             *  Gets a legacy transaction from mempool
             *
             *  @param[in] hashTx Hash of legacy transaction to get.
             *
             *  @param[out] tx The retrieved legacy transaction
             *
             *  @return true if pool contained legacy transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, Legacy::Transaction &tx) const;


            /** Has
             *
             *  Checks if a transaction exists.
             *
             *  @param[in] hashTx Hash of transaction to check.
             *
             *  @return true if transaction in mempool.
             *
             **/
            bool Has(const uint512_t& hashTx) const;


            /** Has
             *
             *  Checks if a genesis exists.
             *
             *  @param[in] hashGenesis Hash of genesis to check.
             *
             *  @return true if transaction in mempool.
             *
             **/
            bool Has(const uint256_t& hashGenesis) const;


            /** Remove
             *
             *  Remove a transaction from pool.
             *
             *  @param[in] hashTx Hash of transaction to remove.
             *
             *  @return true if removed.
             *
             **/
            bool Remove(const uint512_t& hashTx);


            /** Check
             *
             *  Check the memory pool for consistency.
             *
             **/
            void Check();


            /** List
             *
             *  List transactions in memory pool.
             *
             *  @param[out] vHashes List of transaction hashes.
             *  @param[in] nCount The total transactions to get.
             *
             *  @return true if list is not empty.
             *
             **/
            bool List(std::vector<uint512_t> &vHashes, uint32_t nCount = std::numeric_limits<uint32_t>::max(), bool fLegacy = false);


            /** Size
             *
             *  Gets the size of the memory pool.
             *
             **/
            uint32_t Size();


            /** Conflicts
             *
             *  Gets the size of the conflicts memory pool.
             *
             **/
            uint32_t Conflicts();


            /** AttemptForkRecovery
             *
             *  [Option B - EXPERIMENTAL, controlled via -autoforkrecovery, disabled by
             *  default] Attempts an automatic, bounded rollback of the best chain
             *  when a sigchain's transactions keep conflicting with the local
             *  mempool's cached view across many reconciliation cycles in a way
             *  that never resolves via the normal disk-based self-heal paths
             *  (TritiumBlock::Check()'s fSelfHealSequencing, Process()'s
             *  mapCheckRejects resync-and-retry, or this class's own Check()
             *  reconciliation pass). Rather than requiring a manual
             *  -revertblocks=N restart with a guessed depth, this computes the
             *  actual common-ancestor block (by locating the block that hosts the
             *  disk-committed predecessor the conflicting transaction expects)
             *  and rolls back to it directly, bounded by
             *  MAX_AUTO_FORK_RECOVERY_DEPTH so a spoofed/malicious conflict can
             *  never trigger an unbounded rollback.
             *
             *  @param[in] hashGenesis The genesis-id of the stuck sigchain.
             *  @param[in] hashPrevTx The hashPrevTx the still-conflicting
             *             transaction expects as its predecessor.
             *
             *  @return true if a rollback was performed.
             *
             **/
            bool AttemptForkRecovery(const uint256_t& hashGenesis, const uint512_t& hashPrevTx);


            /** ComputeForkDivergence
             *
             *  [C1] Read-only diagnostic: computes how far the best chain has
             *  diverged from a genesis's on-disk expected predecessor
             *  transaction, without performing any rollback and without any
             *  side effects (no cooldown timestamps or miss counters are
             *  touched). Shares the same ancestor-lookup logic
             *  AttemptForkRecovery() uses to decide whether a rollback would be
             *  performed and how deep it would be, so operators (or a
             *  diagnostic RPC) can answer "how deep would -revertblocks=N need
             *  to be right now?" without guessing.
             *
             *  @param[in] hashGenesis The genesis-id of the sigchain to check.
             *  @param[in] hashPrevTx The hashPrevTx the conflicting transaction
             *             expects as its predecessor.
             *
             *  @return populated ForkDivergenceInfo describing the divergence.
             *
             **/
            ForkDivergenceInfo ComputeForkDivergence(const uint256_t& hashGenesis, const uint512_t& hashPrevTx);


            /** ComputeForkDivergence
             *
             *  [C1] Overload that locates the earliest currently-conflicted
             *  transaction for hashGenesis in mapConflicts itself (rather than
             *  requiring the caller to already know hashPrevTx), for use by
             *  diagnostic callers such as the checkforkrecovery RPC that only
             *  have the genesis-id to go on.
             *
             *  @param[in] hashGenesis The genesis-id of the sigchain to check.
             *
             *  @return populated ForkDivergenceInfo describing the divergence;
             *          strError is set if no conflicted transaction is
             *          currently tracked for this genesis.
             *
             **/
            ForkDivergenceInfo ComputeForkDivergence(const uint256_t& hashGenesis);

        };

        extern Mempool mempool;
    }
}

#endif
