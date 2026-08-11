/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2026

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/create.h>

#include <Legacy/types/transaction.h>
#include <Legacy/types/legacy.h>

#include <LLC/types/bignum.h>
#include <TAO/Register/types/address.h>
#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/Ledger/include/ambassador.h>
#include <TAO/Ledger/include/developer.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/genesis_block.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/client.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <Util/include/convert.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>

#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <Util/include/runtime.h>

#include <atomic>
#include <chrono>
#include <optional>
#include <unordered_map>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    namespace
    {
        constexpr const char* kSoloMiningRewardLabel = "none (solo)";

        std::string DynamicRewardLabel(const uint256_t& hashDynamicGenesis)
        {
            return hashDynamicGenesis != 0 ? hashDynamicGenesis.SubString() : std::string(kSoloMiningRewardLabel);
        }

        bool SequenceDiagnosticsEnabled()
        {
            return config::GetBoolArg("-nseqdiag", false);
        }
    }

    /* Condition variable for private blocks. */
    std::condition_variable PRIVATE_CONDITION;


    /* Mining template cache entry — bundles the cached block with the
     * miner-specific finalization metadata so a single atomic store/load
     * publishes the whole tuple consistently.
     *
     * Prior implementation used three independent atomics
     * (tBlockCache, tBlockCacheDynamicGenesis, tBlockCacheExtraNonce).
     * Concurrent readers between the .load() calls could witness a torn
     * (block, dynamicGenesis, extraNonce) triple after another miner's
     * three-step .store() sequence. CachedMiningTemplateRequiresProducerFinalization
     * would then return the wrong answer for one cycle and serve a stale
     * producer to miner X with miner Y's reward address. */
    struct MiningTemplateCacheEntry
    {
        TAO::Ledger::TritiumBlock block;
        uint256_t                 hashDynamicGenesis;
        uint64_t                  nExtraNonce;

        MiningTemplateCacheEntry()
        : block()
        , hashDynamicGenesis(0)
        , nExtraNonce(0)
        {
        }
    };


    /* Option A — per-channel multi-entry template cache.
     *
     * Each mining channel now holds a small LRU table keyed by
     * hashDynamicGenesis instead of a single-slot atomic.  Two miners on
     * the same channel using different reward addresses no longer evict
     * each other's finalized producer on every alternating GET_BLOCK/PUSH:
     * the cache-hit path returns the entry whose hashDynamicGenesis matches
     * the requested reward, so CachedMiningTemplateRequiresProducerFinalization
     * returns false and the multi-hundred-millisecond CreateProducer call is
     * skipped.
     *
     * If no exact match exists, the most recently used entry is returned so
     * the caller can still reuse it as a base template and finalize a new
     * producer (parity with the previous single-slot behaviour).
     *
     * Capacity is operator-tunable via `-blockcache.entries` (default 256,
     * clamped to [1, 1024]).  Default raised from 4 to accommodate
     * deployments with up to a few hundred miners per channel using distinct
     * reward addresses — at ~8 KB per cached TritiumBlock, 256 entries cost
     * ~2 MB per channel × 4 channels ≈ 8 MB resident, which is trivial.
     *
     * Thread-safety: `std::shared_mutex` allows concurrent `Lookup` readers
     * (the common case — 100+ miners per channel calling on every PUSH and
     * GET_BLOCK) and serialises only `Store` writers.  Entries are held by
     * `std::shared_ptr<const MiningTemplateCacheEntry>` so `Lookup` only
     * copies an 8-byte shared_ptr (one ref-count bump) under the lock —
     * never the multi-KB `TritiumBlock` itself.  The block copy, if needed
     * by the caller, happens after the lock is released.
     */
    class MiningTemplateCacheTable
    {
    public:
        using EntryPtr = std::shared_ptr<const MiningTemplateCacheEntry>;
        struct InFlightBuild
        {
            std::mutex mutex;
            std::condition_variable cv;
            bool fComplete{false};
            EntryPtr pResult{nullptr};
            std::chrono::steady_clock::time_point tStarted;
            uint256_t hashDynamicGenesis{0};
            /* Tip the owner is building against.  Used by
             * BeginOrJoinInFlightBuild() to orphan stale handles whose
             * tip no longer matches the current chain — see Follow-up #1. */
            uint1024_t hashBuildTip{0};
        };
        using InFlightPtr = std::shared_ptr<InFlightBuild>;

        /** Look up the entry preferring an exact hashDynamicGenesis match.
         *  Falls back to the most-recently-stored entry on this channel if
         *  no exact match exists.  Returns nullptr only when the cache is
         *  empty.  Holds only a shared (reader) lock — concurrent readers
         *  on the same channel do not serialise against each other.
         *
         *  LRU bookkeeping is intentionally NOT performed here: the only
         *  reason a miner would re-Lookup the same hashDynamicGenesis is
         *  because the surrounding CreateBlock() call also passes the
         *  refreshed entry back through `Store()` (every cache hit path
         *  re-stores), so the MRU order is kept correct on Store without
         *  needing a writer lock on the read path.
         **/
        EntryPtr Lookup(const uint256_t& hashDynamicGenesis) const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            EntryPtr pFallback;
            for(const auto& p : m_entries)
            {
                /* Store() never inserts nullptr, so p is always non-null
                 * in normal operation.  The guard is purely defensive and
                 * cheap — it makes the iteration robust to any future
                 * refactor that adds clear-without-erase semantics. */
                if(!p)
                    continue;
                if(pFallback == nullptr)
                    pFallback = p; // first non-null = MRU
                if(p->hashDynamicGenesis == hashDynamicGenesis)
                    return p;
            }
            return pFallback;
        }

        /** Insert or replace by hashDynamicGenesis, with LRU eviction.
         *  Most-recently-stored entry becomes the new front; oldest entry
         *  is evicted once the capacity is exceeded.  Takes an exclusive
         *  (writer) lock; the new entry is allocated outside the lock to
         *  keep the critical section minimal.
         **/
        void Store(const MiningTemplateCacheEntry& entry)
        {
            const std::size_t cap = Capacity();
            auto pNew = std::make_shared<const MiningTemplateCacheEntry>(entry);

            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for(auto it = m_entries.begin(); it != m_entries.end(); ++it)
            {
                if(*it && (*it)->hashDynamicGenesis == entry.hashDynamicGenesis)
                {
                    *it = pNew;
                    if(it != m_entries.begin())
                        m_entries.splice(m_entries.begin(), m_entries, it);
                    return;
                }
            }
            m_entries.push_front(pNew);
            while(m_entries.size() > cap)
                m_entries.pop_back();
        }

        /** Test/diag: number of entries currently cached. */
        std::size_t Size() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_entries.size();
        }

        /** Test/diag: drop every cached entry on this channel. */
        void Clear()
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_entries.clear();
        }


        /** Evict a single entry keyed by hashDynamicGenesis, if present.
         *  Used to force the next CreateBlock() call for this (channel, reward)
         *  pair down the "block not cached" path -- i.e. a genuine fresh
         *  producer/merkle rebuild rather than a reuse of the existing cached
         *  producer, which by design does not vary with nExtraNonce. **/
        void Invalidate(const uint256_t& hashDynamicGenesis)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for(auto it = m_entries.begin(); it != m_entries.end(); ++it)
            {
                if(*it && (*it)->hashDynamicGenesis == hashDynamicGenesis)
                {
                    m_entries.erase(it);
                    return;
                }
            }
        }

        /* Standard singleflight/request-coalescing pattern: one owner builds,
         * all same-key waiters join and reuse the published result.
         *
         * Tip-aware key (Follow-up #1): an existing in-flight handle whose
         * `hashBuildTip` differs from the caller's `hashBuildTip` is treated
         * as ORPHANED — it would publish a born-stale template after the
         * chain tip advanced.  The orphan is evicted from `m_inflight` and a
         * fresh handle is installed for the new tip so this caller becomes
         * owner.  The original owner's eventual CompleteInFlightBuild() /
         * AbandonInFlightBuild() still notifies its own pre-orphan waiters
         * via the per-handle CV; the map-identity check at lines below
         * (`itInflight->second == pHandle`) prevents the orphaned owner
         * from inadvertently removing the new handle from the map.
         *
         * Pass `hashBuildTip == 0` to opt out of tip-awareness (used by
         * unit-test helpers that don't track a tip). */
        InFlightPtr BeginOrJoinInFlightBuild(const uint256_t& hashDynamicGenesis,
                                             const uint1024_t& hashBuildTip,
                                             bool& fIsOwner)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            const auto it = m_inflight.find(hashDynamicGenesis);
            if(it != m_inflight.end())
            {
                /* Tip match (or either side opted out with 0): join. */
                if(hashBuildTip == 0
                || it->second->hashBuildTip == 0
                || it->second->hashBuildTip == hashBuildTip)
                {
                    fIsOwner = false;
                    return it->second;
                }

                /* Tip mismatch — existing build is for a stale tip.
                 * Orphan it so this caller can build for the new tip. */
                m_inflight.erase(it);
            }

            auto pHandle = std::make_shared<InFlightBuild>();
            pHandle->tStarted = std::chrono::steady_clock::now();
            pHandle->hashDynamicGenesis = hashDynamicGenesis;
            pHandle->hashBuildTip = hashBuildTip;
            m_inflight.emplace(hashDynamicGenesis, pHandle);
            fIsOwner = true;
            return pHandle;
        }

        /* Backward-compat overload: tip-agnostic join, used by test helpers. */
        InFlightPtr BeginOrJoinInFlightBuild(const uint256_t& hashDynamicGenesis, bool& fIsOwner)
        {
            return BeginOrJoinInFlightBuild(hashDynamicGenesis, uint1024_t(0), fIsOwner);
        }

        EntryPtr WaitForInFlightBuild(const InFlightPtr& pHandle,
                                      const std::chrono::milliseconds nTimeout)
        {
            if(!pHandle)
                return nullptr;

            std::unique_lock<std::mutex> lock(pHandle->mutex);
            const bool fReady = pHandle->cv.wait_for(lock, nTimeout, [&pHandle] {
                return pHandle->fComplete;
            });
            if(!fReady)
                return nullptr;

            return pHandle->pResult;
        }

        void CompleteInFlightBuild(const InFlightPtr& pHandle, const MiningTemplateCacheEntry& finalEntry)
        {
            if(!pHandle)
                return;

            const std::size_t cap = Capacity();
            auto pNew = std::make_shared<const MiningTemplateCacheEntry>(finalEntry);

            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                bool fReplaced = false;
                for(auto it = m_entries.begin(); it != m_entries.end(); ++it)
                {
                    if(*it && (*it)->hashDynamicGenesis == finalEntry.hashDynamicGenesis)
                    {
                        *it = pNew;
                        if(it != m_entries.begin())
                            m_entries.splice(m_entries.begin(), m_entries, it);
                        fReplaced = true;
                        break;
                    }
                }
                if(!fReplaced)
                {
                    m_entries.push_front(pNew);
                    while(m_entries.size() > cap)
                        m_entries.pop_back();
                }

                const auto itInflight = m_inflight.find(pHandle->hashDynamicGenesis);
                if(itInflight != m_inflight.end() && itInflight->second == pHandle)
                    m_inflight.erase(itInflight);
            }

            {
                std::lock_guard<std::mutex> lock(pHandle->mutex);
                pHandle->pResult = pNew;
                pHandle->fComplete = true;
            }
            pHandle->cv.notify_all();
        }

        void AbandonInFlightBuild(const InFlightPtr& pHandle)
        {
            if(!pHandle)
                return;

            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                const auto itInflight = m_inflight.find(pHandle->hashDynamicGenesis);
                if(itInflight != m_inflight.end() && itInflight->second == pHandle)
                    m_inflight.erase(itInflight);
            }

            {
                std::lock_guard<std::mutex> lock(pHandle->mutex);
                pHandle->pResult = nullptr;
                pHandle->fComplete = true;
            }
            pHandle->cv.notify_all();
        }

#ifdef UNIT_TESTS
        std::size_t InFlightSize() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_inflight.size();
        }
#endif

        /** Resolved capacity from `-blockcache.entries`, clamped to a sane
         *  range.  Read on every Store() so operators can tune without a
         *  restart in unit/integration scenarios; the cost is one int64_t
         *  arg lookup which is negligible compared to producer signing.
         *
         *  Default 256 covers up to a few hundred miners per channel with
         *  distinct reward addresses at 100% hit rate.  Maximum 1024
         *  bounds resident memory at ~8 MB/channel even when an operator
         *  cranks the knob for a very large mining pool. */
        static std::size_t Capacity()
        {
            const int64_t n = config::GetArg("-blockcache.entries", 256);
            if(n <= 0)
                return 1;
            if(n > 1024)
                return 1024;
            return static_cast<std::size_t>(n);
        }

    private:
        mutable std::shared_mutex m_mutex;
        /* Front = most recent.  std::list provides O(1) splice-to-front
         * for LRU bookkeeping and stable iterators under concurrent
         * shared-lock readers (readers never mutate the list).
         *
         * Entries are immutable once stored (shared_ptr<const>) so a
         * Lookup reader copying a shared_ptr can outlive the writer that
         * evicts that entry from the list — no use-after-free even though
         * the writer holds an exclusive lock during eviction. */
        std::list<EntryPtr> m_entries;
        struct Uint256Hash
        {
            std::size_t operator()(const uint256_t& h) const noexcept
            {
                return static_cast<std::size_t>(h.Get64(0));
            }
        };
        std::unordered_map<uint256_t, InFlightPtr, Uint256Hash> m_inflight;
    };

    /* Indexed by mining channel: 0=PoS, 1=Prime, 2=Hash, 3=Private. */
    static MiningTemplateCacheTable tBlockCache[4];


    void ClearMiningTemplateCaches(const char* pszReason)
    {
        tBlockCache[1].Clear();
        tBlockCache[2].Clear();

        const std::string strReason =
            pszReason ? debug::safe_printstr(" reason=", pszReason) : std::string();

        debug::log(0, FUNCTION, "cleared PRIME/HASH mining template caches", strReason);
    }


    void InvalidateMiningTemplateCacheEntry(const uint32_t nChannel, const uint256_t& hashDynamicGenesis)
    {
        if(nChannel < 1 || nChannel > 2)
            return;

        tBlockCache[nChannel].Invalidate(hashDynamicGenesis);
    }

#ifdef UNIT_TESTS
    namespace
    {
        std::mutex g_inFlightHandleMutex;
        std::unordered_map<std::uint64_t, MiningTemplateCacheTable::InFlightPtr> g_inFlightHandles;
        std::atomic<std::uint64_t> g_nextInFlightToken{1};
    }
#endif


    /* Scans a block's already-selected vtx entries for the producer genesis. */
    bool FindProducerGenesisTxInVtx(const TAO::Ledger::TritiumBlock& block,
        const uint256_t& hashGenesis, TAO::Ledger::Transaction& txOut)
    {
        bool fFound = false;

        for(const auto& vtx : block.vtx)
        {
            /* Only tritium transactions carry a sigchain hashGenesis to match against. */
            if(vtx.first != TRANSACTION::TRITIUM)
                continue;

            /* The hash was just selected from mempool by AddTransactions(), but fall back
             * to the ledger database (already mined/indexed but not yet reflected via
             * ReadLast(), e.g. a just-committed block from another node) for robustness. */
            TAO::Ledger::Transaction tx;
            bool fHasConflict = false;
            if(!mempool.Get(vtx.second, tx) && !LLD::Ledger->ReadTx(vtx.second, tx, fHasConflict, FLAGS::MEMPOOL))
                continue;

            if(tx.hashGenesis != hashGenesis)
                continue;

            /* Track the highest-sequence match; AddTransactions() chains at most one
             * contiguous run per genesis, so this is normally a single hit. */
            if(!fFound || tx.nSequence > txOut.nSequence)
            {
                txOut  = tx;
                fFound = true;
            }
        }

        return fFound;
    }


    /* Option 3 (defense-in-depth): re-validate the producer's chaining predecessor
     * against the block's vtx immediately after producer creation/signing.  In the
     * common case this can never fail, since the producer was seeded from the same
     * FindProducerGenesisTxInVtx() lookup; it guards against future code paths that
     * might create/rebuild a producer without threading pKnownLast through, silently
     * reintroducing the TOCTOU race instead of failing loudly at Check() time deep
     * inside block validation.
     *
     * Returns true both when the producer correctly chains to a dependent transaction
     * in vtx, AND when there is no dependent transaction for this genesis in vtx at all
     * (nothing to validate against — the producer's predecessor is necessarily whatever
     * was already on disk/mempool before this block, which is outside vtx's scope). */
    static bool ValidateProducerAgainstVtx(const TAO::Ledger::TritiumBlock& block)
    {
        TAO::Ledger::Transaction txExpected;
        if(!FindProducerGenesisTxInVtx(block, block.producer.hashGenesis, txExpected))
            return true; // No dependent transaction for this genesis in the block: nothing to check.

        if(block.producer.hashPrevTx != txExpected.GetHash())
        {
            return debug::error(FUNCTION, "producer sequencing disagrees with block vtx:",
                " producer.hashPrevTx=", block.producer.hashPrevTx.SubString(),
                " expected(from vtx)=", txExpected.GetHash().SubString(),
                " genesis=", block.producer.hashGenesis.SubString());
        }

        return true;
    }


    /* Looks up the authoritative sigchain predecessor for a producer genesis from the
     * block's already-selected vtx, returning a pointer usable as CreateTransaction()'s/
     * CreateProducer()'s pKnownLast argument. The returned pointer aliases txOut, which
     * must outlive the call site (txOut is caller-owned storage). Factored out because
     * this exact lookup+pointer pattern is needed at every CreateBlock()/CreateStakeBlock()
     * producer-build call site. */
    static const TAO::Ledger::Transaction* ResolveKnownLastFromVtx(const TAO::Ledger::TritiumBlock& block,
        const uint256_t& hashGenesis, TAO::Ledger::Transaction& txOut)
    {
        return FindProducerGenesisTxInVtx(block, hashGenesis, txOut) ? &txOut : nullptr;
    }


    /* Create a new transaction object from signature chain. */
    bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& pin,
                           TAO::Ledger::Transaction& tx, const uint8_t nScheme, const TAO::Ledger::Transaction* pKnownLast)
    {
        const bool fSeqDiag = SequenceDiagnosticsEnabled();

        /* Get the genesis id of the sigchain. */
        const uint256_t hashGenesis =
            pCredentials->Genesis();

        /* Last sigchain transaction. */
        uint512_t hashLast = 0;
        std::string strSeqSource = "none";
        bool fFallbackToLedger = false;
        bool fUsedSessionIndex = false;
        bool fUsedMempool = false;

        /* Get the last transaction. */
        TAO::API::Transaction txPrev;

        /* FIX (producer-out-of-sequence race): when the caller has already selected this
         * sigchain's authoritative predecessor from the block's vtx (via
         * FindProducerGenesisTxInVtx()), use it verbatim instead of independently
         * re-querying sessions/mempool/disk below.  Those queries run at a *later* instant
         * than AddTransactions()'s own mempool read; if a new transaction for the same
         * genesis is submitted in that gap, the independent query would return it as "last"
         * even though it never made it into vtx, producing a producer.hashPrevTx that
         * Check() rejects with "producer transaction out of sequence" (or, for regular
         * sigchain producers, "transaction in sigchain out of sequence").  Deriving the
         * predecessor from the same snapshot Check() validates against makes the two sides
         * agree by construction.  This also aligns with upstream Nexusoft/LLL-TAO's simpler,
         * single-source sequencing contract instead of layering cascading fallbacks on top
         * of a second live query. */
        if(pKnownLast != nullptr && pKnownLast->hashGenesis == hashGenesis)
        {
            txPrev       = *pKnownLast;
            hashLast     = pKnownLast->GetHash();
            strSeqSource = "block_vtx";
            fUsedMempool = true;
        }
        else if(LLD::Sessions->ReadLast(hashGenesis, hashLast))
        {
            fUsedSessionIndex = true;

            /* Check that we can read the logical disk index. */
            if(!LLD::Sessions->ReadTx(hashLast, txPrev))
                debug::warning(FUNCTION, "could not read logical transaction index");
            else
                strSeqSource = "sessions";
        }

        /* If we don't have the indexes available we need to build from ledger state. */
        TAO::Ledger::Transaction txMem;
        if(pKnownLast == nullptr && mempool.Get(hashGenesis, txMem))
        {
            fUsedMempool = true;

            /* Check that the mempool transaction is greater than our logical database.
             *
             * FIX: When txPrev is still default-constructed (hashGenesis==0) and mempool
             * has a valid genesis (seq=0), the old check `nSequence > 0` was false, causing
             * the mempool genesis to be silently ignored.  The `else if` on the disk
             * fallback then prevented ReadLast from running, so the code fell through to the
             * IsFirst() branch and created a seq=0 producer — a duplicate genesis-id that
             * would be rejected at Connect time.
             *
             * The additional condition ensures that any valid mempool transaction overrides
             * a still-unset txPrev, fixing the first-mining-block bootstrap path for sigchains
             * whose genesis profile is still mempool-only. */
            if(txMem.nSequence > txPrev.nSequence
                || (txPrev.hashGenesis == 0 && txMem.hashGenesis != 0))
            {
                txPrev    = txMem;
                hashLast  = txMem.GetHash();    // CRITICAL: sync hashLast to mempool tx hash
                strSeqSource = (fUsedSessionIndex ? "mempool_override_sessions" : "mempool");
            }
        }

        /* Always check on-disk ledger state as well (not exclusive with mempool).
         *
         * FIX: Previously this was `else if`, meaning that when mempool.Get() returned
         * true but the sequence comparison above failed (e.g. both were seq=0 and the
         * additional condition caught it), the disk path was still skipped.  More
         * critically, when mempool.Get() returned true with a stale entry that lost the
         * comparison, the disk entry — which might be newer — was never consulted.
         *
         * Now we always check disk.  We use a separate hashDiskLast to avoid clobbering
         * a mempool-set hashLast.  The disk entry wins when its sequence is strictly
         * greater than whatever txPrev already holds, OR when txPrev is still unset
         * and disk has a real genesis (same Fix-1 pattern as the mempool branch).
         * Without the unset-txPrev arm, a ledger-only genesis (sessions empty,
         * mempool empty, disk seq=0) left hashLast=0 and fell through to IsFirst(),
         * attempting a duplicate genesis transaction (TIP-05 hygiene).
         *
         * Skipped entirely when pKnownLast was supplied: that value is already the
         * authoritative predecessor selected from the block's vtx, and consulting disk
         * here could only reintroduce the same TOCTOU this parameter exists to close. */
        if(pKnownLast == nullptr)
        {
            uint512_t hashDiskLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashDiskLast))
            {
                fFallbackToLedger = true;

                /* Get previous transaction */
                TAO::Ledger::Transaction txDisk;
                /* hashGenesis==0 is the unset-txPrev sentinel (mirrors mempool
                 * Fix 1). A real seq-0 genesis has hashGenesis!=0, so the
                 * second arm cannot overwrite an already-resolved predecessor. */
                if(LLD::Ledger->ReadTx(hashDiskLast, txDisk)
                && (txDisk.nSequence > txPrev.nSequence
                    || (txPrev.hashGenesis == 0 && txDisk.hashGenesis != 0)))
                {
                    txPrev = txDisk;
                    hashLast = hashDiskLast;
                    strSeqSource = (fUsedSessionIndex ? "ledger_override_sessions" : "ledger");
                }
            }
        }

        /* Check that we found a dependent and therefore it is not the first transaction. */
        if(txPrev.hashGenesis != 0)
        {
            if(fSeqDiag)
            {
                const uint512_t hashPrevComputed = txPrev.GetHash();
                debug::log(0, FUNCTION,
                    "[NSEQ_DIAG][CreateTransaction]"
                    " genesis=", hashGenesis.SubString(),
                    " source=", strSeqSource,
                    " used_sessions=", (fUsedSessionIndex ? "yes" : "no"),
                    " used_mempool=", (fUsedMempool ? "yes" : "no"),
                    " fallback_ledger=", (fFallbackToLedger ? "yes" : "no"),
                    " hashLast=", hashLast.SubString(),
                    " txPrev.hash=", hashPrevComputed.SubString(),
                    " hashLast_mismatch=", (hashLast != hashPrevComputed ? "yes" : "no"),
                    " txPrev.nSequence=", txPrev.nSequence,
                    " txPrev.nTimestamp=", txPrev.nTimestamp,
                    " chosen.nSequence=", txPrev.nSequence + 1);
            }

            /* Build new transaction object. */
            tx.nSequence    = txPrev.nSequence + 1;
            tx.hashGenesis  = txPrev.hashGenesis;
            tx.hashPrevTx   = hashLast;
            tx.nKeyType     = txPrev.nNextType;
            tx.hashRecovery = txPrev.hashRecovery;
            tx.nTimestamp   = std::max(runtime::unifiedtimestamp(), txPrev.nTimestamp);

            /* Check if we need to adjust our key type. */
            if(nScheme != txPrev.nNextType)
                tx.nNextType = nScheme;

            /* Set our next type from previous transaction type. */
            else
                tx.nNextType = txPrev.nNextType;
        }

        /* Set the initial and next key type for genesis transactions */
        else if(tx.IsFirst())
        {
            if(fSeqDiag)
            {
                debug::log(0, FUNCTION,
                    "[NSEQ_DIAG][CreateTransaction]"
                    " genesis=", hashGenesis.SubString(),
                    " source=first_transaction",
                    " used_sessions=", (fUsedSessionIndex ? "yes" : "no"),
                    " used_mempool=", (fUsedMempool ? "yes" : "no"),
                    " fallback_ledger=", (fFallbackToLedger ? "yes" : "no"),
                    " chosen.nSequence=0");
            }

            /* Set the next key type for the genesis transaction */
            tx.nKeyType    = nScheme; //this should use a default value
            tx.nNextType   = nScheme;
            tx.hashGenesis = hashGenesis;
            tx.nTimestamp  = runtime::unifiedtimestamp();

            /* Add our network-id if applicable.*/
            if(config::fHybrid.load())
            {
                /* Grab and set our hybrid network-id. */
                const std::string strHybrid = config::GetArg("-hybrid", "");
                tx.hashPrevTx = LLC::SK512(strHybrid.begin(), strHybrid.end());
            }
        }

        /* Set the transaction version based on the timestamp. */
        const uint32_t nCurrent =
            CurrentTransactionVersion();

        /* Check our activation timestamp. */
        if(TransactionVersionActive(tx.nTimestamp, nCurrent))
            tx.nVersion = nCurrent;
        else
            tx.nVersion = nCurrent - 1;

        /* Genesis Transaction. */
        tx.NextHash(pCredentials->Generate(tx.nSequence + 1, pin));

        return true;
    }


    /* Gets a list of transactions from memory pool for current block. */
    void AddTransactions(TAO::Ledger::TritiumBlock& block)
    {
        /* Clear the transactions. */
        block.vtx.clear();

        /* Check the memory pool. */
        std::vector<uint512_t> vMempool;
        mempool.List(vMempool);

        /* Start a ACID transaction (to be disposed). */
        LLD::TxnBegin(FLAGS::MINER);

        /* Loop through the list of transactions. */
        std::set<uint512_t> setDependents;

        /* Option B filter — tracks tx hashes already accepted into this candidate
         * block. Used by the mempool-only-predecessor gate below to preserve
         * in-block sigchain chaining (e.g. when both T(n) and T(n+1) for the same
         * sigchain are selected together, T(n+1)'s predecessor IS in this block
         * and must not be classified as mempool-only). Contract is exercised by
         * tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp. */
        std::set<uint512_t> setInBlock;

        for(const auto& hash : vMempool)
        {
            /* Check the Size limits of the Current Block. */
            if(::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 256 >= MAX_BLOCK_SIZE)
                break;

            /* Get the transaction from the memory pool. */
            TAO::Ledger::Transaction tx;
            if(!mempool.Get(hash, tx))
                continue;

            /* Don't add transactions that are coinbase or coinstake. */
            if(tx.IsCoinBase() || tx.IsCoinStake())
            {
                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - tx is coinbase/coinstake");
                continue;
            }

            /* Check for failed dependants. */
            if(setDependents.count(tx.hashPrevTx))
            {
                setDependents.insert(hash);

                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - INVALID dependent");
                continue;
            }

            /* Check for timestamp violations. */
            if(tx.nTimestamp > runtime::unifiedtimestamp() + runtime::maxdrift())
            {
                setDependents.insert(hash);

                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - timesamp too far in future");
                continue;
            }

            /* Check the pre-states and post-states. */
            if(!tx.Verify(FLAGS::MINER))
            {
                setDependents.insert(hash);

                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - failed to verify");
                continue;
            }

            /* Check to see if this transaction connects. */
            if(!tx.Connect(FLAGS::MINER))
            {
                setDependents.insert(hash);

                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - failed to connect");
                continue;
            }

            /* Check that the hashlast is on disk. If it is not, then the sig chain genesis must also be in this block.  If for
               any reason the genesis transaction should be in this block but failed one of the above rules, then we could end
               up with a subsequent transaction also in this block for which the genesis is not going to exist.  In which case
               we need to omit this transaction also. The simplest solution for this is to skip any transactions that are not
               the first in the sequence if the hash last is not currently on disk. If a sig chain transcation and subsequent
               transaction genuinely should be in the same block, then ths will just result in the subsequent transaction being
               left out of this block and included in the next.*/
            uint512_t hashLast = 0;
            if(!tx.IsFirst() && !LLD::Ledger->ReadLast(tx.hashGenesis, hashLast))
            {
                setDependents.insert(hash);

                debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(), " - genesis not on disk");
                continue;
            }

            /* Option B filter — mempool-only-predecessor gate (channel-agnostic).
             *
             * Reject any non-first tx whose hashPrevTx is neither on disk nor
             * already accepted earlier in this same candidate block. Such a
             * predecessor lives only in mempool and may be superseded between
             * template build and block sign, producing a dangling hashPrevTx
             * that ValidateVtxSigchainConsistency would reject as BLOCK_REJECTED.
             *
             * Genesis (IsFirst) is exempt — it has no predecessor by design,
             * and the IsFirst branch above never reaches this check.
             * In-block chaining is preserved — setInBlock contains hashes
             * accepted earlier in this loop iteration, which will be persisted
             * atomically with this tx.
             *
             * Applied here inside AddTransactions(), this single insertion
             * point protects every channel that builds a TritiumBlock: STAKE,
             * PRIME, HASH, and PRIVATE.
             *
             * Design contract pinned by:
             *   tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp
             */
            if(!tx.IsFirst())
            {
                const bool fInBlock = setInBlock.count(tx.hashPrevTx) > 0;
                const bool fOnDisk  = LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::BLOCK);
                if(!fInBlock && !fOnDisk)
                {
                    setDependents.insert(hash);

                    debug::log(2, FUNCTION, "Skipping transaction ", hash.SubString(),
                        " - predecessor is mempool-only (channel-agnostic filter)");
                    continue;
                }
            }

            /* Add the transaction to the block. */
            block.vtx.push_back(std::make_pair(TRANSACTION::TRITIUM, hash));
            setInBlock.insert(hash);
        }

        /* Abort the temporary ACID transaction. */
        LLD::TxnAbort(FLAGS::MINER);

        /* Clear for legacy. */
        vMempool.clear();

        /* Retrieve list of transaction hashes from mempool. Limit list to a sane size that would typically more than fill a
         * legacy block, rather than pulling entire pool if it is very large. */
        TAO::Ledger::mempool.List(vMempool, 100, true);

        /* Loop through the list of transactions. */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        for(const auto& hash : vMempool)
        {
            /* Check the Size limits of the Current Block. */
            if(::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION) + 256 >= MAX_BLOCK_SIZE)
                break;

            /* Get the transaction from the memory pool. */
            Legacy::Transaction tx;
            if(!mempool.Get(hash, tx))
            {
                debug::error(FUNCTION, "Unable to read transaction from mempool ", hash.SubString(10));
                continue;
            }

            /* Don't add transactions that are coinbase or coinstake. */
            if(tx.IsCoinBase() || tx.IsCoinStake())
            {
                debug::log(2, FUNCTION, "Mempool transaction is Coinbase/Coinstake ", hash.SubString(10));
                continue;
            }

            /* Check transaction for finality. */
            if(!tx.IsFinal())
            {
                debug::log(2, FUNCTION, "Mempool transaction is not Final ", hash.SubString(10));
                continue;
            }

            /* Check for timestamp violations. */
            if(tx.nTime > runtime::unifiedtimestamp() + runtime::maxdrift())
                continue;

            /* Retrieve tx inputs */
            std::map<uint512_t, std::pair<uint8_t, DataStream> > mapInputs;
            if(!tx.FetchInputs(mapInputs))
            {
                debug::log(2, FUNCTION, "Failed to get transaction inputs ", hash.SubString(10));
                continue;
            }

            /* Check tx fee meetes minimum fee requirement */
            int64_t nTxFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();
            if(nTxFees <  tx.GetMinFee(1000, false))
            {
                debug::log(2, FUNCTION, "Not enough fees ", hash.SubString(10));
                continue;
            }

            /* Check that transction can be connected. */
            if(!tx.Connect(mapInputs, tStateBest, FLAGS::MINER))
            {
                debug::log(2, FUNCTION, "Failed to connect inputs ", hash.SubString(10));
                continue;
            }

            /* Dump sequence on verbose 3 levels. */
            if(config::nVerbose >= 3)
                tx.print();

            /* Add the transaction to the block. */
            block.vtx.push_back(std::make_pair(TRANSACTION::LEGACY, hash));
        }
    }


    /* Populate block header data for a new block. */
    void AddBlockData(const TAO::Ledger::BlockState& tStateBest, const uint32_t nChannel, TAO::Ledger::TritiumBlock& block)
    {
        /* Calculate the merkle root (stake minter must handle channel 0 after completing coinstake producer setup) */
        if(nChannel != 0)
        {
            /* Add the transaction hashes. */
            std::vector<uint512_t> vHashes;
            for(const auto& tx : block.vtx)
                vHashes.push_back(tx.second);

            /* Producer transaction is last hash in list. */
            vHashes.push_back(block.producer.GetHash(true));

            /* Build the block's merkle root. */
            block.hashMerkleRoot = block.BuildMerkleTree(vHashes);
        }

        /* Add remaining block data */
        /* PRIMARY STALENESS ANCHOR: hashPrevBlock = hashBestChain at creation time.
         * Both mining servers (Legacy port 8323 and Stateless port 9323) compare
         * pBlock->hashPrevBlock against ChainState::hashBestChain at SUBMIT_BLOCK time.
         * This catches reorgs at the same integer height that nBestHeight comparison misses.
         * Both hashPrevBlock and nHeight are baked into the 216-byte serialized template
         * and are immutable after this point. ProofHash() for Prime hashes nVersion→nBits
         * which includes nHeight — never overwrite either field after serialization. */
        block.hashPrevBlock = tStateBest.GetHash();
        block.nChannel      = nChannel;

        /* Use UNIFIED height for block.nHeight — matches NexusMiner #169/#170 contract.
         *
         * TritiumBlock::Accept() validates: statePrev.nHeight + 1 == nHeight
         * where statePrev is the block at hashPrevBlock (the unified best-chain tip).
         * This means nHeight MUST equal the unified blockchain height of the new block,
         * not the channel-specific height.
         *
         * Channel-specific height (nChannelHeight) is computed separately by BlockState
         * during SetBest() via GetLastState() and stored in BlockState::nChannelHeight.
         * It is metadata only — never serialized into the 216-byte block template bytes.
         *
         * NexusMiner #169: block.nHeight = tStateBest.nHeight + 1 (UNIFIED)
         * NexusMiner #170: ValidateTemplate() compares nHeight vs nNodeUnified + 1
         */
        block.nHeight = tStateBest.nHeight + 1;

        debug::log(2, FUNCTION, "Creating block template for channel ", nChannel,
                   " at unified height ", block.nHeight);

        block.nBits         = GetNextTargetRequired(tStateBest, nChannel, false);
        debug::log(2, FUNCTION, "[NBITS_BAKED] channel=", nChannel,
                   " height=", block.nHeight,
                   " nBits=0x", std::hex, block.nBits, std::dec,
                   " hashPrevBlock=", block.hashPrevBlock.SubString(),
                   " tStateBest.nHeight=", tStateBest.nHeight);
        block.nNonce        = 1;
        block.nTime         = std::max(tStateBest.GetBlockTime() + 1, runtime::unifiedtimestamp());
    }


    /* Create a new block object from the chain. */
    bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
        const uint32_t nChannel, TAO::Ledger::TritiumBlock &rBlockRet, const uint64_t nExtraNonce, Legacy::Coinbase *pCoinbaseRecipients,
        const uint256_t& hashDynamicGenesis, bool* pfTipRaceRetry)
    {
        /* Cache key: always the signing wallet's genesis (node operator sigchain).
         * hashDynamicGenesis (miner reward address) flows separately to producer
         * finalization. Cached templates may be reused only as a base: when the
         * requested reward address differs from the cached finalization metadata,
         * the producer and merkle root are rebuilt below. */
        const uint256_t hashGenesis = user->Genesis();

        /* Only allow prime, hash, and private channels. */
        if(nChannel < 1 || nChannel > 3)
            return debug::error(FUNCTION, "Invalid channel: ", nChannel);

        /* Set the block to null. */
        rBlockRet.SetNull();

        /* Modulate the Block Versions if they correspond to their proper time stamp */
        /* Normally, if condition is true and block version is current version unless an activation is pending */
        uint32_t nCurrent = CurrentBlockVersion();
        if(BlockVersionActive(runtime::unifiedtimestamp(), nCurrent)) // --> New Block Version Activation Switch
            rBlockRet.nVersion = nCurrent;
        else
            rBlockRet.nVersion = nCurrent - 1;

        /* Retrieve currently cached block — Option A: per-channel multi-entry
         * LRU lookup preferring an exact hashDynamicGenesis match.  Lookup
         * returns a shared_ptr held under a shared (reader) lock, so 100+
         * concurrent miners on the same channel never serialise against
         * each other and the multi-KB TritiumBlock is never copied while
         * holding the lock. */
        static const MiningTemplateCacheEntry s_emptyCacheEntry{};
        const MiningTemplateCacheTable::EntryPtr pCachedEntry =
            tBlockCache[nChannel].Lookup(hashDynamicGenesis);
        const MiningTemplateCacheEntry& tCachedEntry =
            pCachedEntry ? *pCachedEntry : s_emptyCacheEntry;
        const TAO::Ledger::TritiumBlock& tBlockCached =
            tCachedEntry.block;
        const uint256_t hashCachedDynamicGenesis =
            tCachedEntry.hashDynamicGenesis;
        const uint64_t nCachedExtraNonce =
            tCachedEntry.nExtraNonce;

        /* Cache the best chain before processing. */
        const TAO::Ledger::BlockState tStateBest =
            ChainState::tStateBest.load();

        /* Event-driven cache invalidation: check each condition individually. */
        bool fNeedsNewBlock = false;

        /* Primary check: Has the blockchain advanced? */
        if(ChainState::hashBestChain.load() != tBlockCached.hashPrevBlock)
        {
            fNeedsNewBlock = true;
            debug::log(2, FUNCTION, "Block cache invalidated by chain advance, regenerating...");
        }

        /* Secondary check: Has the user/genesis changed? */
        if(hashGenesis != tBlockCached.producer.hashGenesis)
        {
            fNeedsNewBlock = true;
            debug::log(2, FUNCTION, "Block cache invalidated (genesis/user change), regenerating...");
        }

        /* Tertiary check: Time-based safety timeout.
         * Default 420 s (7 minutes) — ensures the block cache is rebuilt periodically
         * as a safety net during long dry spells on slow channels (e.g. Prime).
         * Operator can override via -blockrefresh command-line arg.
         * The computed default (420 s) is always positive, so the narrowing conversion from
         * int64_t to uint64_t is safe here. */
        const uint64_t nExpiration = static_cast<uint64_t>(config::GetArg("-blockrefresh", 420));
        if(runtime::unifiedtimestamp() >= tBlockCached.producer.nTimestamp + nExpiration)
        {
            fNeedsNewBlock = true;
            debug::log(0, FUNCTION, "Block cache timed out after ", nExpiration, " seconds (safety net), regenerating...");
        }

        /* Quaternary check: Has the producer's sigchain advanced on disk?
         * This catches the case where a different channel's block committed a producer
         * for the same sigchain (advancing WriteLast), but hashBestChain hasn't changed
         * yet from this channel's perspective — meaning checks 1-3 all pass and the
         * cache would serve a stale producer.
         * Defense-in-depth: ValidateProducerFreshness() at SUBMIT_BLOCK time remains the
         * authoritative backstop for any TOCTOU races. */
        if(!fNeedsNewBlock && tBlockCached.producer.hashGenesis != 0)
        {
            uint512_t hashDiskLast = 0;
            if(LLD::Ledger->ReadLast(tBlockCached.producer.hashGenesis, hashDiskLast))
            {
                if(hashDiskLast != tBlockCached.producer.hashPrevTx)
                {
                    fNeedsNewBlock = true;
                    debug::log(0, FUNCTION, "Block cache invalidated: producer sigchain advanced on disk"
                               " (disk last=", hashDiskLast.SubString(),
                               " != cached producer.hashPrevTx=", tBlockCached.producer.hashPrevTx.SubString(),
                               "), regenerating...");
                }
            }
        }

        /* Quinary check: Does the mempool contain a transaction for the producer's genesis?
         * If so, AddTransactions() will pick it up into block.vtx, and the producer must
         * follow it — but the cached producer was built before this mempool tx arrived.
         * Force a rebuild so CreateProducer() sees the mempool state and sequences correctly.
         * Note: mempool.Get() is O(1) hash lookup — negligible cost.
         * This is an early-exit companion to the post-cache mempool check below (line ~488).
         * Both use the same comparison logic; this one avoids entering the cache path at all
         * when a stale producer can be detected upfront. */
        if(!fNeedsNewBlock && tBlockCached.producer.hashGenesis != 0)
        {
            TAO::Ledger::Transaction txMempool;
            if(mempool.Get(tBlockCached.producer.hashGenesis, txMempool))
            {
                if(txMempool.GetHash() != tBlockCached.producer.hashPrevTx)
                {
                    fNeedsNewBlock = true;
                    debug::log(0, FUNCTION, "Block cache invalidated: mempool has newer sigchain tx"
                               " for producer genesis (mempool tx=", txMempool.GetHash().SubString(),
                               " != cached producer.hashPrevTx=", tBlockCached.producer.hashPrevTx.SubString(),
                               "), regenerating...");
                }
            }
        }

        /* Reuse cached block if no invalidation condition triggered. */
        if(!fNeedsNewBlock)
        {
            /* Set the block to cached block. */
            rBlockRet = tBlockCached;
            
            /* ✅ FIX: Ensure nChannel is set correctly from parameter (defense in depth)
             * While cached block should already have correct nChannel from when it was stored,
             * explicitly setting it here ensures correctness even if cache had issues. */
            rBlockRet.nChannel = nChannel;
            
            /* Diagnostic logging for template validation */
            debug::log(2, FUNCTION, "Using cached block template for channel ", nChannel);
            debug::log(2, FUNCTION, "[NBITS_CACHE_HIT]",
                       " channel=", nChannel,
                       " nBits=0x", std::hex, tBlockCached.nBits, std::dec,
                       " height=", tBlockCached.nHeight,
                       " hashPrevBlock=", tBlockCached.hashPrevBlock.SubString(),
                       " cached_reward=", hashCachedDynamicGenesis.SubString(),
                       " requested_reward=", hashDynamicGenesis.SubString());
            debug::log(2, FUNCTION, "  nChannel verified: ", rBlockRet.nChannel);

            /* Add new transactions. */
            AddTransactions(rBlockRet);

            /* Check that the producer isn't going to orphan any transactions,
             * and finalize miner-specific reward data when this cache hit is
             * being reused as a base template for another miner.
             *
             * FIX (producer-out-of-sequence race, Option 1/2): staleness detection and
             * the eventual rebuild both now derive the producer's predecessor from the
             * block's own vtx (FindProducerGenesisTxInVtx()) rather than an independent
             * mempool.Get(hashGenesis, ...) query.  The previous mempool-only check could
             * disagree with vtx if a newer sigchain transaction for the same genesis was
             * submitted between AddTransactions() and this check, causing either a false
             * "not stale" (leaving a producer that doesn't chain to vtx) or an unnecessary
             * rebuild racing against a transaction that never made it into the block.
             * Using vtx as the single source of truth for both decisions closes that gap. */
            TAO::Ledger::Transaction txVtxLast;
            const TAO::Ledger::Transaction* pKnownLast =
                ResolveKnownLastFromVtx(rBlockRet, rBlockRet.producer.hashGenesis, txVtxLast);

            const bool fProducerFinalizationRequired =
                CachedMiningTemplateRequiresProducerFinalization(
                    hashCachedDynamicGenesis, hashDynamicGenesis,
                    nCachedExtraNonce, nExtraNonce);
            const std::string strDynamicReward = DynamicRewardLabel(hashDynamicGenesis);

            if(fProducerFinalizationRequired)
            {
                debug::log(2, FUNCTION, "Cached block base requires producer finalization"
                           " for reward=", strDynamicReward,
                           " extra_nonce=", nExtraNonce);
            }

            if(fProducerFinalizationRequired
            || (pKnownLast != nullptr && rBlockRet.producer.hashPrevTx != pKnownLast->GetHash()))
            {
                /* Handle for STALE producer. */
                debug::log(0, FUNCTION, fProducerFinalizationRequired
                    ? "Producer is miner-specific, finalizing cached base..."
                    : "Producer is stale, rebuilding...");

                /* Create a new producer transaction for given block.
                 * Pass hashDynamicGenesis (miner reward address) so coinbase is routed
                 * to the remote miner, not the node operator. */
                debug::log(2, FUNCTION, "Rebuilding stale producer: reward address = ",
                    strDynamicReward);
                const auto tProducerStart = std::chrono::steady_clock::now();
                if(!CreateProducer(user, pin, rBlockRet.producer, tStateBest, rBlockRet.nVersion, nChannel, nExtraNonce, pCoinbaseRecipients, hashDynamicGenesis, pKnownLast))
                    return debug::error(FUNCTION, "Failed to create producer transactions.");
                const int64_t nProducerMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - tProducerStart).count();
                debug::log(1, FUNCTION, "CreateProducer cache-finalization duration_ms=", nProducerMs,
                           " channel=", nChannel,
                           " height=", tStateBest.nHeight + 1,
                           " reward=", strDynamicReward,
                           " extra_nonce=", nExtraNonce);

                /* Option 3 (defense-in-depth): confirm the freshly rebuilt producer
                 * actually chains to vtx before this template is signed and offered
                 * for mining. */
                if(!ValidateProducerAgainstVtx(rBlockRet))
                    return debug::error(FUNCTION, "Rebuilt producer failed vtx sequencing validation.");
            }

            /* Update the producer timestamp */
            UpdateProducerTimestamp(rBlockRet.producer);

            /* Sign the producer transaction. */
            rBlockRet.producer.Sign(user->Generate(rBlockRet.producer.nSequence, pin));

            /* Double check our next hash if -safemode enabled. */
            if(config::GetBoolArg("-safemode", false))
            {
                /* Re-calculate our next hash if safemode forcing not to use cache. */
                const uint256_t hashNext =
                    TAO::Ledger::Transaction::NextHash(user->Generate(rBlockRet.producer.nSequence + 1, pin, false), rBlockRet.producer.nNextType);

                /* Check that this next hash is what we are expecting. */
                if(rBlockRet.producer.hashNext != hashNext)
                    throw debug::exception("-safemode next hash mismatch, broadcast terminated");
            }

            /* Rebuild the merkle tree for updated block. */
            std::vector<uint512_t> vHashes;
            for(const auto& tx : rBlockRet.vtx)
                vHashes.push_back(tx.second);

            /* Producer transaction is last. */
            vHashes.push_back(rBlockRet.producer.GetHash(true));

            /* Build the block's merkle root. */
            rBlockRet.hashMerkleRoot = rBlockRet.BuildMerkleTree(vHashes);

            /* Store the finalized template and metadata as a single atomic
             * triple. Later cache hits may reuse this as a base, but only
             * same reward/extra-nonce requests may reuse the producer
             * without rebuilding it. */
            {
                MiningTemplateCacheEntry tNewEntry;
                tNewEntry.block              = rBlockRet;
                tNewEntry.hashDynamicGenesis = hashDynamicGenesis;
                tNewEntry.nExtraNonce        = nExtraNonce;
                tBlockCache[nChannel].Store(tNewEntry);
            }
        }
        else //block not cached, set up new block
        {
            /* Capture the build tip up-front so the singleflight key includes
             * (reward, tip) and waiters won't join builds for stale tips
             * (Follow-up #1).  This is the same tip captured at line 824
             * above into tStateBest; recomputing here keeps intent local. */
            const uint1024_t hashBuildTip = tStateBest.GetHash();

            bool fSingleflightOwner = false;
            auto pInFlight = tBlockCache[nChannel].BeginOrJoinInFlightBuild(
                hashDynamicGenesis, hashBuildTip, fSingleflightOwner);

            auto TryApplyPublishedTemplate = [&rBlockRet, &nChannel, &nExtraNonce, &hashDynamicGenesis](
                const MiningTemplateCacheTable::EntryPtr& pPublished, const int64_t nWaitMs) -> bool
            {
                if(!(pPublished && pPublished->hashDynamicGenesis == hashDynamicGenesis))
                    return false;

                const uint1024_t hashBestChain = ChainState::hashBestChain.load();
                if(pPublished->block.hashPrevBlock != hashBestChain)
                {
                    debug::log(2, FUNCTION, "[SINGLEFLIGHT] rejected stale published template for reward=",
                               hashDynamicGenesis.SubString(),
                               " published_prev=", pPublished->block.hashPrevBlock.SubString(),
                               " current_tip=", hashBestChain.SubString(),
                               " owner_extra_nonce=", pPublished->nExtraNonce,
                               " requested_extra_nonce=", nExtraNonce,
                               " (waited ", nWaitMs, " ms)");
                    return false;
                }

                /* Per PR #598: extra-nonce differences are intentionally ignored
                 * in this joined-template path to avoid redundant producer rebuild/
                 * signing work; producer is keyed by (tip, reward) only. */
                rBlockRet = pPublished->block;
                debug::log(2, FUNCTION, "[SINGLEFLIGHT] joined in-flight build for reward=",
                           hashDynamicGenesis.SubString(),
                           " owner_extra_nonce=", pPublished->nExtraNonce,
                           " requested_extra_nonce=", nExtraNonce,
                           " — skipped duplicate CreateProducer (waited ", nWaitMs, " ms)");

                rBlockRet.UpdateTime();
                if(rBlockRet.nChannel != nChannel)
                    rBlockRet.nChannel = nChannel;
                return true;
            };

            if(!fSingleflightOwner)
            {
                const auto tWaitStart = std::chrono::steady_clock::now();
                auto pPublished = tBlockCache[nChannel].WaitForInFlightBuild(pInFlight, std::chrono::milliseconds(2000));
                const int64_t nWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - tWaitStart).count();
                if(TryApplyPublishedTemplate(pPublished, nWaitMs))
                    return true;

                /* Timed out (or observed a stale in-flight handle with no published
                 * result): re-check the cache first, then re-register with singleflight so this caller can
                 * either join a current owner or become the new owner. */
                auto pCachedRetry = tBlockCache[nChannel].Lookup(hashDynamicGenesis);
                if(TryApplyPublishedTemplate(pCachedRetry, nWaitMs))
                    return true;

                pInFlight = tBlockCache[nChannel].BeginOrJoinInFlightBuild(
                    hashDynamicGenesis, hashBuildTip, fSingleflightOwner);
                if(!fSingleflightOwner)
                {
                    const auto tRetryWaitStart = std::chrono::steady_clock::now();
                    pPublished = tBlockCache[nChannel].WaitForInFlightBuild(pInFlight, std::chrono::milliseconds(2000));
                    const int64_t nRetryWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - tRetryWaitStart).count();
                    if(TryApplyPublishedTemplate(pPublished, nRetryWaitMs))
                        return true;
                }
            }

            struct InFlightOwnerGuard
            {
                MiningTemplateCacheTable* pCache{nullptr};
                MiningTemplateCacheTable::InFlightPtr pHandle;
                bool fOwner{false};
                bool fCompleted{false};

                ~InFlightOwnerGuard()
                {
                    if(fOwner && !fCompleted && pCache != nullptr)
                        pCache->AbandonInFlightBuild(pHandle);
                }
            } guard{&tBlockCache[nChannel], pInFlight, fSingleflightOwner, false};


            /* Must add transactions first, before creating producer, so producer is sequenced last if user has tx in block */
            AddTransactions(rBlockRet);

            /* FIX (producer-out-of-sequence race, Option 1/2): derive the producer's
             * chaining predecessor from the block's own vtx rather than letting
             * CreateTransaction() independently re-query mempool/disk afterwards. See
             * FindProducerGenesisTxInVtx() for the full rationale. */
            TAO::Ledger::Transaction txVtxLast;
            const TAO::Ledger::Transaction* pKnownLast = ResolveKnownLastFromVtx(rBlockRet, hashGenesis, txVtxLast);

            /* Create the new producer transaction for given block.
             * Pass hashDynamicGenesis (miner reward address) so coinbase is routed
             * to the remote miner, not the node operator. */
            const std::string strDynamicReward = DynamicRewardLabel(hashDynamicGenesis);
            debug::log(2, FUNCTION, "Creating fresh producer: reward address = ",
                strDynamicReward);
            const auto tProducerStart = std::chrono::steady_clock::now();
            if(!CreateProducer(user, pin, rBlockRet.producer, tStateBest, rBlockRet.nVersion, nChannel, nExtraNonce, pCoinbaseRecipients, hashDynamicGenesis, pKnownLast))
                return debug::error(FUNCTION, "Failed to create producer transactions.");
            const int64_t nProducerMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - tProducerStart).count();
            debug::log(1, FUNCTION, "CreateProducer fresh-template duration_ms=", nProducerMs,
                       " channel=", nChannel,
                       " height=", tStateBest.nHeight + 1,
                       " reward=", strDynamicReward,
                       " extra_nonce=", nExtraNonce);

            /* Option 3 (defense-in-depth): confirm the freshly built producer actually
             * chains to vtx before this template is signed and offered for mining. */
            if(!ValidateProducerAgainstVtx(rBlockRet))
                return debug::error(FUNCTION, "Fresh producer failed vtx sequencing validation.");


            /* Follow-up #2: tip-advance abandonment.  Falcon signing took
             * nProducerMs; if the chain tip moved during that window, the
             * block we built has a stale hashPrevBlock and cannot be mined.
             * Abandon the in-flight handle so any pre-orphan waiters fall
             * through to a fresh build for the new tip, and return failure
             * so the caller retries with the current tStateBest. */
            if(fSingleflightOwner)
            {
                const uint1024_t hashCurrentTip = ChainState::hashBestChain.load();
                if(hashCurrentTip != hashBuildTip)
                {
                    debug::log(0, FUNCTION, "[SINGLEFLIGHT] tip advanced during fresh-template build,"
                               " abandoning to avoid publishing stale template",
                               " build_tip=", hashBuildTip.SubString(),
                               " current_tip=", hashCurrentTip.SubString(),
                               " reward=", strDynamicReward,
                               " producer_ms=", nProducerMs);
                    tBlockCache[nChannel].AbandonInFlightBuild(pInFlight);
                    guard.fCompleted = true; // prevent guard dtor from re-abandoning
                    if(pfTipRaceRetry != nullptr)
                        *pfTipRaceRetry = true;
                    return debug::error(FUNCTION, "Tip advanced during template build, retry required.");
                }
            }

            /* Update the producer timestamp */
            UpdateProducerTimestamp(rBlockRet.producer);

            /* Sign the producer transaction. */
            rBlockRet.producer.Sign(user->Generate(rBlockRet.producer.nSequence, pin));

            /* Populate the block metadata */
            AddBlockData(tStateBest, nChannel, rBlockRet);
            
            /* Diagnostic logging for template validation */
            debug::log(2, FUNCTION, "Created new block template for channel ", nChannel);
            debug::log(2, FUNCTION, "  nChannel verified: ", rBlockRet.nChannel);
            debug::log(2, FUNCTION, "  nHeight: ", rBlockRet.nHeight);
            debug::log(2, FUNCTION, "  hashPrevBlock: ", rBlockRet.hashPrevBlock.SubString());

            /* Store the cached block and miner-specific finalization
             * metadata as a single atomic triple. */
            {
                MiningTemplateCacheEntry tNewEntry;
                tNewEntry.block              = rBlockRet;
                tNewEntry.hashDynamicGenesis = hashDynamicGenesis;
                tNewEntry.nExtraNonce        = nExtraNonce;
                if(fSingleflightOwner)
                    tBlockCache[nChannel].CompleteInFlightBuild(pInFlight, tNewEntry);
                else
                    tBlockCache[nChannel].Store(tNewEntry);
                guard.fCompleted = true;
            }
        }

        /* Update the time for the newly created block. */
        rBlockRet.UpdateTime();
        
        /* ✅ FINAL VERIFICATION: Ensure nChannel is correct before returning
         * This catches any unexpected issues in the CreateBlock() flow */
        if(rBlockRet.nChannel != nChannel)
        {
            debug::error(FUNCTION, "❌ CRITICAL: nChannel mismatch detected before return!");
            debug::error(FUNCTION, "   Expected nChannel: ", nChannel);
            debug::error(FUNCTION, "   Actual nChannel: ", rBlockRet.nChannel);
            debug::error(FUNCTION, "   This indicates a bug in CreateBlock() - forcing correction");
            rBlockRet.nChannel = nChannel;  // Force correct value
        }
        
        debug::log(2, FUNCTION, "✓ CreateBlock() returning: nChannel=", rBlockRet.nChannel, 
                   " nHeight=", rBlockRet.nHeight);

        return true;
    }

#ifdef UNIT_TESTS
    namespace Testing
    {
        SingleflightToken BeginOrJoinMiningTemplateInFlight(const uint32_t nChannel,
                                                            const uint256_t& hashDynamicGenesis,
                                                            bool& fIsOwner)
        {
            if(nChannel >= 4)
                return 0;

            auto pHandle = tBlockCache[nChannel].BeginOrJoinInFlightBuild(hashDynamicGenesis, fIsOwner);
            const std::uint64_t nToken = g_nextInFlightToken.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(g_inFlightHandleMutex);
            g_inFlightHandles[nToken] = pHandle;
            return nToken;
        }

        bool WaitForMiningTemplateInFlight(const uint32_t nChannel,
                                           const SingleflightToken nToken,
                                           const std::chrono::milliseconds nTimeout,
                                           uint256_t& hashOut)
        {
            if(nChannel >= 4)
                return false;

            MiningTemplateCacheTable::InFlightPtr pHandle;
            {
                std::lock_guard<std::mutex> lock(g_inFlightHandleMutex);
                const auto it = g_inFlightHandles.find(nToken);
                if(it == g_inFlightHandles.end())
                    return false;
                pHandle = it->second;
            }

            auto pEntry = tBlockCache[nChannel].WaitForInFlightBuild(pHandle, nTimeout);
            {
                std::lock_guard<std::mutex> lock(g_inFlightHandleMutex);
                g_inFlightHandles.erase(nToken);
            }
            if(!pEntry)
                return false;

            hashOut = pEntry->hashDynamicGenesis;
            return true;
        }

        void CompleteMiningTemplateInFlight(const SingleflightToken nToken,
                                            const uint32_t nChannel,
                                            const uint256_t& hashDynamicGenesis,
                                            const uint64_t nExtraNonce)
        {
            if(nChannel >= 4)
                return;

            MiningTemplateCacheTable::InFlightPtr pHandle;
            {
                std::lock_guard<std::mutex> lock(g_inFlightHandleMutex);
                const auto it = g_inFlightHandles.find(nToken);
                if(it == g_inFlightHandles.end())
                    return;
                pHandle = it->second;
                g_inFlightHandles.erase(it);
            }

            MiningTemplateCacheEntry tEntry;
            tEntry.hashDynamicGenesis = hashDynamicGenesis;
            tEntry.nExtraNonce = nExtraNonce;
            tBlockCache[nChannel].CompleteInFlightBuild(pHandle, tEntry);
        }

        void AbandonMiningTemplateInFlight(const SingleflightToken nToken,
                                           const uint32_t nChannel)
        {
            if(nChannel >= 4)
                return;

            MiningTemplateCacheTable::InFlightPtr pHandle;
            {
                std::lock_guard<std::mutex> lock(g_inFlightHandleMutex);
                const auto it = g_inFlightHandles.find(nToken);
                if(it == g_inFlightHandles.end())
                    return;
                pHandle = it->second;
                g_inFlightHandles.erase(it);
            }

            tBlockCache[nChannel].AbandonInFlightBuild(pHandle);
        }

        void StoreMiningTemplateCacheEntryForTesting(const uint32_t nChannel,
                                                     const uint256_t& hashDynamicGenesis,
                                                     const uint64_t nExtraNonce)
        {
            if(nChannel >= 4)
                return;

            MiningTemplateCacheEntry tEntry;
            tEntry.hashDynamicGenesis = hashDynamicGenesis;
            tEntry.nExtraNonce = nExtraNonce;
            tBlockCache[nChannel].Store(tEntry);
        }

        void ClearMiningTemplateCacheForTesting(const uint32_t nChannel)
        {
            if(nChannel >= 4)
                return;
            tBlockCache[nChannel].Clear();
        }

        std::size_t MiningTemplateInFlightCountForTesting(const uint32_t nChannel)
        {
            if(nChannel >= 4)
                return 0;
            return tBlockCache[nChannel].InFlightSize();
        }
    }
#endif


    /* Create a producer transaction object from signature chain. */
    bool CreateProducer(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                           TAO::Ledger::Transaction &rProducer,
                           const TAO::Ledger::BlockState& tStateBest,
                           const uint32_t nBlockVersion,
                           const uint32_t nChannel,
                           const uint64_t nExtraNonce,
                           Legacy::Coinbase *pCoinbaseRecipients,
                           const uint256_t& hashDynamicGenesis,
                           const TAO::Ledger::Transaction* pKnownLast)
    {
        /* Defensively reset the producer to a default-constructed state.
         *
         * Bug history: the cache-hit + producer-finalization path in CreateBlock()
         * (above) does `rBlockRet = tBlockCached;` followed by CreateProducer(...,
         * rBlockRet.producer, ...).  The cached producer already has a fully written
         * 49-byte OP::COINBASE stream in vContracts[0].  CreateTransaction() below
         * only assigns scalar fields (nSequence, hashGenesis, hashPrevTx, ...) and
         * never touches vContracts.  The subsequent `rProducer[0] << OP::COINBASE; ...`
         * writes (lines below) therefore APPEND a second 49-byte stream onto the
         * existing one, producing a 98-byte malformed contract.  The block is then
         * signed and pushed to miners; on SUBMIT_BLOCK, TAO::Register::Verify fires
         * "can not verify PRIMITIVE per contract" because contract.End() is false
         * after consuming the first 49 bytes.  This was the root cause of the
         * mainnet rejection sequence documented in
         * docs/BURST_BLOCK_COINBASE_PRIMITIVE_OVERFLOW.md (PR #584); the size-check
         * gate added in that PR catches the symptom — this reset removes the cause.
         *
         * Trigger in production was CachedMiningTemplateRequiresProducerFinalization
         * returning true (different miner reward/extra-nonce vs cached entry), which
         * routes the call through the cache-hit rebuild branch with a stale producer.
         *
         * This unconditional reset is safe in both call sites:
         *  - Cache-hit rebuild: clears the inherited stale contracts (the fix).
         *  - Fresh-block path: rProducer is already a default-constructed member of
         *    a freshly SetNull()'d block, so the reset is a no-op.
         */
        rProducer = TAO::Ledger::Transaction();

        /* Setup the producer transaction. */
        if(!CreateTransaction(user, pin, rProducer, TAO::Ledger::SIGNATURE::BRAINPOOL, pKnownLast))
            return debug::error(FUNCTION, "Failed to create producer transactions.");

        /* Create the Coinbase Transaction if the Channel specifies. */
        if(nChannel == 1 || nChannel == 2)
        {
            /* Determine reward recipient for coinbase transaction.
             * hashDynamicGenesis MUST be a valid TritiumGenesis (UserType) sigchain hash.
             * Register Addresses are NOT valid coinbase recipients — Coinbase::Verify()
             * enforces this on all network peers. The caller (new_block()) validates the
             * type byte before reaching this point via ValidateRewardAddress() and the
             * defense-in-depth guard. */
            uint256_t hashRewardRecipient = user->Genesis();

            if(hashDynamicGenesis != 0)
            {
                /* Route rewards to dynamic address (miner's TritiumGenesis). */
                hashRewardRecipient = hashDynamicGenesis;
                debug::log(1, FUNCTION, "Reward routing: DYNAMIC to ", hashRewardRecipient.SubString(), " (miner)");
            }
            else
            {
                /* No dynamic routing - solo mining by node operator */
                debug::log(3, FUNCTION, "Reward routing: STATIC to ", hashRewardRecipient.SubString(), " (node operator)");
            }

            /* Output type 0 is mining/minting reward */
            uint64_t nBlockReward = GetCoinbaseReward(tStateBest, nChannel, 0);

            /* Create coinbase transaction. */
            rProducer[0] << uint8_t(TAO::Operation::OP::COINBASE);

            /* Add the spendable genesis - using dynamic routing if available */
            rProducer[0] << hashRewardRecipient;

            /* The total to be credited. */
            uint64_t nCredit = nBlockReward;

            /* If there are coinbase recipients, set the reward to the coinbase wallet reward. */
            if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
                nCredit = pCoinbaseRecipients->WalletReward();

            /* Check to make sure credit is non-zero. */
            if(nCredit == 0)
                return debug::error(FUNCTION, "Empty block producer reward.");

            rProducer[0] << nCredit;

            /* The extra nonce to coinbase. */
            rProducer[0] << nExtraNonce;

            debug::log(2, FUNCTION, "[COINBASE_STREAM] slot=0"
                " stream_size=", rProducer[0].Operations().size(),
                " channel=", nChannel,
                " recipient=", hashRewardRecipient.SubString());

            /* Add coinbase recipient amounts to block producer transaction if any. */
            if(pCoinbaseRecipients && !pCoinbaseRecipients->IsNull())
            {
                /* Ensure wallet reward and recipient amounts add up to correct block reward. */
                if(!pCoinbaseRecipients->IsValid())
                    return debug::error(FUNCTION, "Coinbase recipients contain invalid amounts.");

                /* Get the map of outputs for this coinbase. */
                std::map<std::string, uint64_t> mapOutputs = pCoinbaseRecipients->Outputs();
                uint32_t nTx = 1;

                for(const auto& entry : mapOutputs)
                {
                    /* Build the recipient address from a hex string. */
                    uint256_t hashGenesis = uint256_t(entry.first);

                    /* Ensure the address is valid. */
                    if(!LLD::Ledger->HasFirst(hashGenesis))
                        return debug::error(FUNCTION, "Invaild recipient address: ", entry.first, " (", nTx, ")");

                    /* Set coinbase operation. */
                    rProducer[nTx] << uint8_t(TAO::Operation::OP::COINBASE);

                    /* Set sigchain recipient. */
                    rProducer[nTx] << hashGenesis;

                    /* Set coinbase amount for associated recipent. */
                    rProducer[nTx] << entry.second;

                    /* The extra nonce to coinbase. */
                    rProducer[nTx] << nExtraNonce;

                    debug::log(2, FUNCTION, "[COINBASE_STREAM] slot=", nTx,
                        " stream_size=", rProducer[nTx].Operations().size(),
                        " channel=", nChannel,
                        " recipient=", hashGenesis.SubString());

                    ++nTx;
                }
            }

            /* Get the last state block for channel. */
            TAO::Ledger::BlockState statePrev = tStateBest;
            if(GetLastState(statePrev, nChannel))
            {
                /* Check for interval. */
                if(statePrev.nChannelHeight %
                    (config::fTestNet.load() ? AMBASSADOR_PAYOUT_THRESHOLD_TESTNET : AMBASSADOR_PAYOUT_THRESHOLD) == 0)
                {
                    /* Get the total in reserves. */
                    int64_t nBalance = statePrev.nReleasedReserve[1] - (33 * NXS_COIN); //leave 33 coins in the reserve
                    if(nBalance > 0)
                    {
                        /* Loop through the embassy sigchains. */
                        for(auto it = Ambassador(nBlockVersion).begin(); it != Ambassador(nBlockVersion).end(); ++it)
                        {
                            /* Make sure to push to end. */
                            const uint32_t nContract = rProducer.Size();

                            /* Create coinbase transaction. */
                            rProducer[nContract] << uint8_t(TAO::Operation::OP::COINBASE);
                            rProducer[nContract] << it->first;

                            /* The total to be credited. */
                            const uint64_t nCredit = (nBalance * it->second.second) / 1000;
                            rProducer[nContract] << nCredit;
                            rProducer[nContract] << uint64_t(0);

                            debug::log(2, FUNCTION, "[COINBASE_STREAM] slot=", nContract,
                                " stream_size=", rProducer[nContract].Operations().size(),
                                " channel=", nChannel, " (ambassador payout)");
                        }
                    }
                }


                /* Check for interval. */
                if(statePrev.nChannelHeight %
                    (config::fTestNet.load() ? DEVELOPER_PAYOUT_THRESHOLD_TESTNET : DEVELOPER_PAYOUT_THRESHOLD) == 0)
                {
                    /* Get the total in reserves. */
                    int64_t nBalance = statePrev.nReleasedReserve[2] - (3 * NXS_COIN); //leave 3 coins in the reserve
                    if(nBalance > 0)
                    {
                        /* Loop through the embassy sigchains. */
                        for(auto it = Developer(nBlockVersion).begin(); it != Developer(nBlockVersion).end(); ++it)
                        {
                            /* Make sure to push to end. */
                            const uint32_t nContract = rProducer.Size();

                            /* Create coinbase transaction. */
                            rProducer[nContract] << uint8_t(TAO::Operation::OP::COINBASE);
                            rProducer[nContract] << it->first;

                            /* The total to be credited. */
                            const uint64_t nCredit = (nBalance * it->second.second) / 1000;
                            rProducer[nContract] << nCredit;
                            rProducer[nContract] << uint64_t(0);

                            debug::log(2, FUNCTION, "[COINBASE_STREAM] slot=", nContract,
                                " stream_size=", rProducer[nContract].Operations().size(),
                                " channel=", nChannel, " (developer payout)");
                        }
                    }
                }
            }
        }
        else if(nChannel == 3)
        {
            /* Create an authorize producer. */
            rProducer[0] << uint8_t(TAO::Operation::OP::AUTHORIZE);

            /* Get the sigchain txid. */
            rProducer[0] << rProducer.hashPrevTx;
            rProducer[0] << rProducer.hashGenesis;
        }

        return true;
    }


    /* Create a new Proof of Stake (channel 0) block object from the chain. */
    bool CreateStakeBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                          TAO::Ledger::TritiumBlock& block, const bool fGenesis)
    {
        /* Proof of stake has channel-id of 0. */
        const uint32_t nChannel = 0;

        /* Set the block to null. */
        block.SetNull();

        /* Modulate the Block Versions if they correspond to their proper time stamp */
        /* Normally, if condition is true and block version is current version unless an activation is pending */
        uint32_t nCurrent = CurrentBlockVersion();
        if(BlockVersionActive(runtime::unifiedtimestamp(), nCurrent)) // --> New Block Version Activation Switch
            block.nVersion = nCurrent;
        else
            block.nVersion = nCurrent - 1;

        /* Cache the best chain before processing. */
        const TAO::Ledger::BlockState tStateBest = ChainState::tStateBest.load();

        /* Add the transactions to the block. */
        /* Solo Genesis has no transactions, but pool Genesis does to calculate proofs (pool Genesis won't hash this block) */
        /* Must add transactions first, before creating producer, so producer is sequenced last if user has tx in block */
        if(!fGenesis || config::fPoolStaking.load())
            AddTransactions(block);

        /* FIX (producer-out-of-sequence race, Option 1/2): derive the producer's
         * chaining predecessor from the block's own vtx rather than letting
         * CreateTransaction() independently re-query mempool/disk afterwards. See
         * FindProducerGenesisTxInVtx() for the full rationale. */
        TAO::Ledger::Transaction txVtxLast;
        const TAO::Ledger::Transaction* pKnownLast = ResolveKnownLastFromVtx(block, user->Genesis(), txVtxLast);

        /* Create the producer transaction. */
        TAO::Ledger::Transaction rProducer;
        if(!CreateTransaction(user, pin, rProducer, TAO::Ledger::SIGNATURE::BRAINPOOL, pKnownLast))
            return debug::error(FUNCTION, "failed to create producer transactions");

        /* Update the producer timestamp */
        UpdateProducerTimestamp(rProducer);

        /* Add block producer to block */
        block.producer = rProducer;

        /* Option 3 (defense-in-depth): confirm the producer actually chains to vtx
         * before this block is handed to the stake minter. */
        if(!ValidateProducerAgainstVtx(block))
            return debug::error(FUNCTION, "producer failed vtx sequencing validation");

        /* NOTE: The remainder of Coinstake producer not configured here. Stake minter must handle it. */

        /* Populate the block metadata */
        AddBlockData(tStateBest, nChannel, block);

        return true;
    }


    /*  Creates the genesis block. */
    bool CreateGenesis()
    {
        /* Get the genesis hash. */
        uint1024_t hashGenesis = 0;

        /* Check the ledger database for hybrid genesis. */
        if(config::fHybrid.load())
            LLD::Ledger->ReadHybridGenesis(hashGenesis);
        else
            hashGenesis = TAO::Ledger::ChainState::Genesis();

        /* Check for genesis from disk. */
        if(!LLD::Ledger->ReadBlock(hashGenesis, ChainState::tStateGenesis))
        {
            /* Check for client mode. */
            BlockState state;
            if(config::fClient.load())
            {
                /* Create the tritium genesis block. */
                if(!config::fTestNet.load())
                    state = TritiumGenesis();
                else
                    state = LegacyGenesis();


                /* Write the block to disk. */
                if(!LLD::Client->WriteBlock(hashGenesis, ClientBlock(state)))
                    return debug::error(FUNCTION, "genesis didn't commit to disk");

                /* Write the best chain to the database. */
                if(!LLD::Client->WriteBestChain(hashGenesis))
                    return debug::error(FUNCTION, "couldn't write best chain.");
            }
            else
            {
                /* Create the genesis block. */
                if(config::fHybrid.load())
                {
                    /* Create the new block state. */
                    state = HybridGenesis();

                    /* Assign current genesis hash to newly minted block. */
                    hashGenesis = hashGenesisHybrid;
                }
                else
                    state = LegacyGenesis();

                /* Write the block to disk. */
                if(!LLD::Ledger->WriteBlock(hashGenesis, state))
                    return debug::error(FUNCTION, "genesis didn't commit to disk");

                /* Write the best chain to the database. */
                if(!LLD::Ledger->WriteBestChain(hashGenesis))
                    return debug::error(FUNCTION, "couldn't write best chain.");
            }

            /* Check that the genesis hash is correct. */
            if(state.GetHash() != hashGenesis)
                return debug::error(FUNCTION, "genesis hash does not match");

            /* Set the proper chain state variables. */
            ChainState::tStateGenesis = state;

            /* Set the best block. */
            ChainState::hashBestChain = hashGenesis;
            ChainState::tStateBest     = ChainState::tStateGenesis;
        }
        else if(config::fHybrid.load())
            hashGenesisHybrid = hashGenesis; //we need to set our new genesis hash here

        return true;
    }


    /* Handles the creation of a private block chain. */
    void ThreadGenerator()
    {
        /* Check for our generation credentials. */
        if(!config::fHybrid.load() || !config::HasArg("-generate"))
            return;

        /* Build new session object. */
        TAO::API::Authentication::Session tSession =
            TAO::API::Authentication::Session("generate", config::GetArg("-generate", "").c_str(), TAO::API::Authentication::Session::LOCAL);

        /* Get the current genesis-id of session. */
        const uint256_t hashGenesis = tSession.Genesis();

        /* Check for duplicates in ledger db. */
        TAO::Ledger::Transaction txPrev;
        if(LLD::Ledger->HasFirst(hashGenesis))
        {
            /* Get the last transaction. */
            uint512_t hashLast;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
            {
                debug::notice(FUNCTION, "No previous transaction found... closing");
                return;
            }

            /* Get previous transaction */
            if(!LLD::Ledger->ReadTx(hashLast, txPrev))
            {
                debug::notice(FUNCTION, "No previous transaction found... closing");
                return;
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.nNextType = txPrev.nNextType;
            tx.NextHash(tSession.Credentials()->Generate(txPrev.nSequence + 1, "1234"));

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
            {
                debug::notice(FUNCTION, "Invalid credentials... closing");
                return;
            }
        }

        /* Push the new session to auth. */
        TAO::API::Authentication::Insert(TAO::API::Authentication::SESSION::PRIVATE, tSession);

        /* Extract our latency parameter. */
        const uint64_t nLatency =
            config::GetArg("-latency", 5000); //default value of 5 seconds

        /* Startup Debug. */
        debug::log(0, FUNCTION, "Generator Thread Started at latency ", nLatency, "ms");

        /* Initialize our indexing session. */
        TAO::API::Indexing::Initialize(TAO::API::Authentication::SESSION::PRIVATE);

        std::mutex MUTEX;
        while(!config::fShutdown.load())
        {
            std::unique_lock<std::mutex> CONDITION_LOCK(MUTEX);
            PRIVATE_CONDITION.wait(CONDITION_LOCK, []{ return config::fShutdown.load() || mempool.Size() > 0; });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                break;

            /* Keep block production to five seconds. */
            runtime::sleep(nLatency);

            /* Create the block object. */
            runtime::timer TIMER;
            TIMER.Start();

            /* Get our credentials object. */
            const auto& pCredentials =
                TAO::API::Authentication::Credentials(uint256_t(TAO::API::Authentication::SESSION::PRIVATE));

            /* Build our block object now. */
            TAO::Ledger::TritiumBlock block;
            if(!TAO::Ledger::CreateBlock(pCredentials, "1234", 3, block))
                continue;

            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = pCredentials->Generate(block.producer.nSequence, "1234").GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(block.producer.nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret))
                        continue;

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        continue;

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret, true))
                        continue;

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        continue;

                    break;
                }
            }

            /* Debug output. */
            debug::log(0, FUNCTION, "Private Block CREATED in ", TIMER.ElapsedMilliseconds(), " ms");

            /* Verify the block object. */
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);

            /* Check the statues. */
            if(!(nStatus & PROCESS::ACCEPTED))
                continue;

            /* Relay the block and bestchain. */
            const uint1024_t hashBlock = block.GetHash();
            LLP::TRITIUM_SERVER->Relay
            (
                LLP::TritiumNode::ACTION::NOTIFY,

                /* Relay BLOCK notification. */
                uint8_t(LLP::TritiumNode::TYPES::BLOCK),
                hashBlock,

                /* Relay BESTCHAIN notification. */
                uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN),
                hashBlock,

                /* Relay BESTHEIGHT notification. */
                uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT),
                block.nHeight
            );
        }
    }


    /* Updates the producer timestamp, making sure it is not earlier than the previous block. */
    void UpdateProducerTimestamp(TAO::Ledger::Transaction &rProducer)
    {
        /* Update the producer timestamp, making sure it is not earlier than the previous block.  However we can't simply
        set the timstamp to be last block time + 1, in case there is a long gap between blocks, as there is a consensus
        rule that the producer timestamp cannot be more than 3600 seconds before the current block time. */
        if(ChainState::tStateBest.load().GetBlockTime() + 1 > runtime::unifiedtimestamp())
            rProducer.nTimestamp = std::max(rProducer.nTimestamp, ChainState::tStateBest.load().GetBlockTime() + 1);
        else
            rProducer.nTimestamp = std::max(rProducer.nTimestamp, runtime::unifiedtimestamp());

        /* Since we have updated the producer transaction timestamp, we now also need to set the transaction version again as
           the version is based on the transaction time. Transaction version is current version unless an activation is pending */
        uint32_t nCurrent = CurrentTransactionVersion();
        if(TransactionVersionActive(rProducer.nTimestamp, nCurrent))
            rProducer.nVersion = nCurrent;
        else
            rProducer.nVersion = nCurrent - 1;
    }


    /* Updates the producer timestamp, making sure it is not earlier than the previous block. */
    void UpdateProducerTimestamp(TAO::Ledger::TritiumBlock& block)
    {
        /* Pass new producer transaction to update timestamp. */
        UpdateProducerTimestamp(block.producer);
    }
}
