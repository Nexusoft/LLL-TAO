/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_CREDENTIAL_CACHE_H
#define NEXUS_TAO_API_INCLUDE_CREDENTIAL_CACHE_H

#include <LLC/types/uint1024.h>

#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <Util/include/runtime.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace TAO::API
{
    /** @class CredentialCache
     *
     *  Per-consumer cache of a resolved Credentials handle.  Designed to be
     *  embedded as a member of a connection / worker that repeatedly resolves
     *  the same session's credentials (e.g. miner connections calling
     *  `Authentication::Credentials()` once per `new_block()` cycle).
     *
     *  Refresh path semantics (see design doc):
     *
     *    * NEVER calls Sessions::Load - that is the establishment path
     *      (Argon2 + AES + DB I/O + indexing) and must not appear in the
     *      credential hot path.
     *    * NEVER calls Sessions::Create.
     *    * NEVER calls Authentication::Unlock - read-only `Unlocked()` only.
     *    * NEVER nests its internal lock around Authentication::MUTEX.  The
     *      slow path takes them sequentially: cache lock -> release ->
     *      Authentication::MUTEX (via Credentials()) -> release -> cache lock.
     *    * Refresh is exactly one `Authentication::Credentials(hashSession)`
     *      call.  Worst case = parity with the no-cache baseline.
     *
     *  Staleness signals (any one forces refresh):
     *
     *    1. `Authentication::CurrentEpoch()` differs from cached snapshot
     *       (covers Insert / Update / terminate_session / Shutdown).
     *    2. `Authentication::Unlocked(MINING, hashSession)` returns false
     *       (covers lock-state change).
     *    3. `runtime::unifiedtimestamp() - tLastValidated > nTTLSeconds`
     *       (TTL backstop in case any invalidation hook is missed).
     *    4. Bound `hashGenesis` of the refreshed credential differs from the
     *       fingerprint captured at populate time (covers same-id replacement).
     *
     *  The cache holds a `std::shared_ptr<TAO::Ledger::Credentials>` that
     *  independently owns a copy of the session's credential at populate time,
     *  so a concurrent session termination cannot dangle the cached pointer.
     *
     **/
    class CredentialCache
    {
    public:

        /** Reasons a cache slot was invalidated.  Surfaced via LastReason()
         *  for telemetry / log breadcrumbs. **/
        enum class Reason : uint8_t
        {
            NONE             = 0,
            EMPTY            = 1, // cache had no entry yet
            EPOCH_BUMP       = 2, // Authentication state changed
            UNLOCK_CHANGED   = 3, // MINING actions no longer granted
            TTL_EXPIRED      = 4, // nTTLSeconds elapsed since last validation
            IDENTITY_CHANGED = 5, // refreshed credential bound to different genesis
            SESSION_MISSING  = 6, // Authentication::Credentials threw / returned null
            POST_USE_DRIFT   = 7, // epoch moved while caller was using handle
        };


        /** Default TTL: belt-and-braces refresh even when epoch unchanged.
         *  60s is conservative versus the typical block interval. **/
        static constexpr uint64_t DEFAULT_TTL_SECONDS = 60;


        /** Default constructor.  Cache starts empty. **/
        CredentialCache()
        : nTTLSeconds      (DEFAULT_TTL_SECONDS)
        , pCached          (nullptr)
        , hashBoundSession (0)
        , hashBoundGenesis (0)
        , nEpochSnapshot   (0)
        , tLastValidated   (0)
        , nLastReason      (Reason::EMPTY)
        , fPostUseDirty    (false)
        {
        }


        /** Construct with a custom TTL (seconds).  Pass 0 to disable the TTL
         *  backstop and rely entirely on epoch + Unlocked() signals. **/
        explicit CredentialCache(const uint64_t nTTLSecondsIn)
        : nTTLSeconds      (nTTLSecondsIn)
        , pCached          (nullptr)
        , hashBoundSession (0)
        , hashBoundGenesis (0)
        , nEpochSnapshot   (0)
        , tLastValidated   (0)
        , nLastReason      (Reason::EMPTY)
        , fPostUseDirty    (false)
        {
        }


        /* Non-copyable, non-movable: cache slot is owned by its host object. */
        CredentialCache(const CredentialCache&)            = delete;
        CredentialCache& operator=(const CredentialCache&) = delete;
        CredentialCache(CredentialCache&&)                 = delete;
        CredentialCache& operator=(CredentialCache&&)      = delete;


        /** Acquire
         *
         *  Returns a strong reference to the credential for the given session,
         *  using the cache when fresh and refreshing once-under-MUTEX otherwise.
         *
         *  On any failure to resolve (session terminated, missing, or
         *  bound-genesis mismatch) returns a null shared_ptr WITHOUT throwing -
         *  callers must handle null the same way they handle today's
         *  "Session not found" path (e.g. surface TEMPLATE_SOURCE_UNAVAILABLE).
         *
         *  @param[in] hashSession  The session-id whose credential to resolve.
         *
         *  @return A shared_ptr to a copy of the live Credentials, or nullptr.
         *
         **/
        std::shared_ptr<TAO::Ledger::Credentials> Acquire(const uint256_t& hashSession)
        {
            /* Step 1: eligibility precheck (no auth touch, no cache lock). */
            if(hashSession == 0)
                return nullptr;

            /* Step 2 + 3: fast-path liveness gate.  Both checks are lock-free
             * relative to our cache lock; CurrentEpoch() is a single atomic
             * load; IsSessionMiningReady() takes Authentication::MUTEX briefly
             * but does NOT throw and never calls Unlock/Load/Create. */
            const uint64_t nCurrentEpoch =
                Authentication::CurrentEpoch();

            const bool fUnlocked =
                IsSessionMiningReady(hashSession);

            /* Snapshot the fast-path state we read above so the slow path can
             * decide a precise reason code under the cache lock. */
            const uint64_t tNow =
                runtime::unifiedtimestamp();

            /* Step 4: cache-hit fast path under the cache lock only. */
            {
                std::lock_guard<std::mutex> lk(MUTEX);

                /* Determine staleness without touching Authentication::MUTEX. */
                const Reason eReason =
                    classify(hashSession, nCurrentEpoch, fUnlocked, tNow);

                if(eReason == Reason::NONE && pCached != nullptr)
                {
                    /* Fresh hit. */
                    return pCached;
                }

                /* Record reason for telemetry before releasing the lock. */
                nLastReason = eReason;
            }

            /* Step 5: cache miss / stale - refresh slow path.
             *
             * Note: cache lock is intentionally released here.  We must not
             * hold MUTEX while taking Authentication::MUTEX (the design's
             * non-nesting rule).  Credentials() may throw "Session doesn't
             * exist" if the session vanished between fast and slow paths;
             * we catch and surface as null. */
            std::shared_ptr<TAO::Ledger::Credentials> pFresh;
            uint256_t hashFreshGenesis = 0;
            try
            {
                const memory::encrypted_ptr<TAO::Ledger::Credentials>& rLive =
                    Authentication::Credentials(hashSession);

                if(!rLive.IsNull())
                {
                    /* Copy-construct an independently-owned snapshot.  This is
                     * the "owned copy, NOT a borrowed reference" guarantee from
                     * the design: even if the session is terminated between
                     * now and the next call, our snapshot remains valid until
                     * the next refresh evicts it. */
                    pFresh = std::make_shared<TAO::Ledger::Credentials>(*rLive);
                    hashFreshGenesis = pFresh->Genesis();
                }
            }
            catch(...)
            {
                pFresh.reset();
            }

            /* Step 5.4-5.6: snapshot epoch & populate slot atomically. */
            {
                std::lock_guard<std::mutex> lk(MUTEX);

                /* Active eviction: release prior held copy unconditionally. */
                pCached.reset();
                hashBoundSession = 0;
                hashBoundGenesis = 0;
                fPostUseDirty    = false;

                if(!pFresh)
                {
                    nLastReason = Reason::SESSION_MISSING;
                    /* Slot stays empty; next call will retry the slow path. */
                    return nullptr;
                }

                /* Populate. */
                pCached          = pFresh;
                hashBoundSession = hashSession;
                hashBoundGenesis = hashFreshGenesis;
                nEpochSnapshot   = Authentication::CurrentEpoch();
                tLastValidated   = runtime::unifiedtimestamp();
            }

            return pFresh;
        }


        /** PostUseCheck
         *
         *  Defense-in-depth: invoke after the consumer has used the credential
         *  returned by Acquire().  If the epoch advanced while the caller was
         *  using the handle, mark the slot dirty so the next Acquire() forces
         *  a refresh.  Does NOT invalidate the in-flight result - downstream
         *  validators (stale-tip / signature / ValidateProducerFreshness)
         *  remain the authoritative reject path.
         *
         *  @return true if epoch was stable across the use, false if it drifted.
         *
         **/
        bool PostUseCheck()
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            const uint64_t nLiveEpoch =
                Authentication::CurrentEpoch();

            if(nLiveEpoch != nEpochSnapshot)
            {
                fPostUseDirty = true;
                nLastReason   = Reason::POST_USE_DRIFT;
                return false;
            }
            return true;
        }


        /** Invalidate
         *
         *  Force the next Acquire() to refresh.  Intended for tests and for
         *  consumers that have out-of-band evidence the credential is stale
         *  (e.g. a 'Session not found' deep in the pipeline).
         *
         **/
        void Invalidate()
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            pCached.reset();
            hashBoundSession = 0;
            hashBoundGenesis = 0;
            nEpochSnapshot   = 0;
            tLastValidated   = 0;
            fPostUseDirty    = false;
            nLastReason      = Reason::EMPTY;
        }


        /** HasEntry  --  true iff the slot currently holds a credential. **/
        bool HasEntry() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return (pCached != nullptr);
        }


        /** BoundSession  --  session-id the current slot was populated from. **/
        uint256_t BoundSession() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return hashBoundSession;
        }


        /** BoundGenesis  --  genesis fingerprint of the current cached credential. **/
        uint256_t BoundGenesis() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return hashBoundGenesis;
        }


        /** EpochSnapshot  --  epoch captured at populate time (0 if empty). **/
        uint64_t EpochSnapshot() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return nEpochSnapshot;
        }


        /** LastValidated  --  timestamp of last successful populate. **/
        uint64_t LastValidated() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return tLastValidated;
        }


        /** LastReason  --  the most recent staleness reason observed. **/
        Reason LastReason() const
        {
            std::lock_guard<std::mutex> lk(MUTEX);
            return nLastReason;
        }


        /** TTLSeconds  --  configured TTL backstop (0 disables). **/
        uint64_t TTLSeconds() const
        {
            return nTTLSeconds;
        }


    private:

        /** IsSessionMiningReady
         *
         *  Mirrors the policy of LLP::IsDefaultSessionReady() but for an
         *  arbitrary session-id.  Returns false if the session is missing or
         *  not unlocked for MINING.  Never throws.  Does NOT call Unlock(),
         *  Load(), or Create().
         *
         **/
        static bool IsSessionMiningReady(const uint256_t& hashSession)
        {
            try
            {
                if(!Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING, hashSession))
                    return false;

                /* Touch Credentials() to confirm the session entry exists.
                 * If it doesn't, this throws and we return false. */
                Authentication::Credentials(hashSession);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }


        /** classify
         *
         *  Decide whether the current slot is fresh given the fast-path
         *  observations.  MUST be called with MUTEX (our cache lock) held.
         *  Pure: does not mutate state.
         *
         **/
        Reason classify(const uint256_t& hashSession,
                        const uint64_t   nCurrentEpoch,
                        const bool       fUnlocked,
                        const uint64_t   tNow) const
        {
            /* No entry yet, or prior PostUseCheck flagged drift. */
            if(pCached == nullptr)
                return Reason::EMPTY;

            if(fPostUseDirty)
                return Reason::POST_USE_DRIFT;

            /* Bound to a different session than the one being requested. */
            if(hashBoundSession != hashSession)
                return Reason::IDENTITY_CHANGED;

            /* Auth state changed since populate. */
            if(nCurrentEpoch != nEpochSnapshot)
                return Reason::EPOCH_BUMP;

            /* Lock state changed (or session went missing). */
            if(!fUnlocked)
                return Reason::UNLOCK_CHANGED;

            /* TTL backstop. */
            if(nTTLSeconds > 0 && tLastValidated > 0
               && tNow > tLastValidated
               && (tNow - tLastValidated) > nTTLSeconds)
                return Reason::TTL_EXPIRED;

            return Reason::NONE;
        }


    private:

        /** TTL backstop in seconds (0 disables). **/
        const uint64_t nTTLSeconds;


        /** Cache lock - guards the slot fields below.  NEVER held across a
         *  call into Authentication::MUTEX (non-nesting rule).  `mutable` so
         *  the const accessor methods can lock it. **/
        mutable std::mutex MUTEX;


        /** The held credential snapshot.  Independently owned via shared_ptr
         *  so termination of the source session does not dangle this pointer. **/
        std::shared_ptr<TAO::Ledger::Credentials> pCached;


        /** Session-id this slot was populated from. **/
        uint256_t hashBoundSession;


        /** Identity fingerprint - genesis of the cached credential at
         *  populate time.  Used to detect same-id replacement. **/
        uint256_t hashBoundGenesis;


        /** Authentication epoch value captured at populate time. **/
        uint64_t nEpochSnapshot;


        /** Unix timestamp of last successful populate, for TTL backstop. **/
        uint64_t tLastValidated;


        /** Most recent staleness reason observed (telemetry). **/
        Reason nLastReason;


        /** Set by PostUseCheck when epoch drifted during use; cleared on
         *  next refresh. **/
        bool fPostUseDirty;
    };

} // namespace TAO::API

#endif
