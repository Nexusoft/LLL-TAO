/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_TEMPLATE_DELIVERY_H
#define NEXUS_LLP_INCLUDE_MINING_TEMPLATE_DELIVERY_H

#include <LLP/include/mining_constants.h>
#include <LLP/include/round_state_utility.h>

#include <array>
#include <chrono>
#include <cstdint>

namespace LLP
{
    enum class TemplatePushDecisionReason : uint8_t
    {
        FIRST_PUSH = 0,
        FORCE_BYPASS,
        CHAIN_TIP_CHANGED,
        INTERVAL_ELAPSED,
        THROTTLED
    };


    struct TemplatePushDecision
    {
        bool fShouldSend = false;
        TemplatePushDecisionReason eReason = TemplatePushDecisionReason::FIRST_PUSH;
        int64_t nElapsedMs = 0;
    };


    inline const char* TemplatePushDecisionReasonCode(TemplatePushDecisionReason eReason)
    {
        switch(eReason)
        {
            case TemplatePushDecisionReason::FORCE_BYPASS:     return "FORCE_BYPASS";
            case TemplatePushDecisionReason::CHAIN_TIP_CHANGED:return "CHAIN_TIP_CHANGED";
            case TemplatePushDecisionReason::INTERVAL_ELAPSED: return "INTERVAL_ELAPSED";
            case TemplatePushDecisionReason::THROTTLED:        return "THROTTLED";
            default:                                           return "FIRST_PUSH";
        }
    }


    /** PushedTipHistory
     *
     *  Bounded time-windowed history of recently delivered chain tips, used by
     *  the push throttle to distinguish a *genuinely new* tip from one that we
     *  have already pushed during the current fork-resolution window.
     *
     *  Why a ring history (Option B) and not a single "last pushed" hash:
     *  --------------------------------------------------------------------
     *  A fork-resolution storm can cycle through many candidate tips in a few
     *  hundred milliseconds (e.g. 12 distinct tips in 302 ms observed in
     *  production logs).  The previous single-slot bypass keyed only on the
     *  *most recent* pushed hash; if a SetBest event later resolved back to a
     *  hash that was pushed earlier in the storm, the throttle's hash-bypass
     *  did NOT fire (hash matched the recorded "last pushed" because the storm
     *  cycled through it) AND the time-bypass also did NOT fire (still within
     *  the 1-second floor), so the miner was silently stranded on stale work.
     *
     *  This ring records every successful push for TTL_MS milliseconds.  The
     *  throttle bypasses the time floor only when the current tip is NOT in
     *  the ring — i.e. it has either never been pushed, or was last pushed
     *  long enough ago that re-pushing is appropriate.  Thus a recurring tip
     *  during a storm is correctly time-gated, while genuinely new tips
     *  (including post-storm settlement to a brand-new tip) are delivered
     *  immediately.
     */
    class PushedTipHistory
    {
    public:
        /** Number of recent tips remembered.  Sized to comfortably exceed the
         *  worst observed storm depth (~12 distinct tips). */
        static constexpr std::size_t CAPACITY = 16;

        /** Time window after which a recorded entry is considered aged-out
         *  and no longer suppresses re-delivery of the same hash.  5 s is
         *  comfortably longer than any observed fork-resolution storm
         *  (~hundreds of ms) and shorter than the real ~50 s block interval. */
        static constexpr int64_t TTL_MS = 5000;

        struct Entry
        {
            uint1024_t hash{};
            std::chrono::steady_clock::time_point tPushed{};
        };

        PushedTipHistory() = default;

        /** ContainsRecent — true if `hash` was recorded within the last TTL.
         *  Aged-out entries are treated as absent. */
        bool ContainsRecent(const uint1024_t& hash,
            std::chrono::steady_clock::time_point tNow) const
        {
            for(const Entry& e : m_entries)
            {
                if(e.tPushed == std::chrono::steady_clock::time_point{})
                    continue;
                const int64_t nAgeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    tNow - e.tPushed).count();
                if(nAgeMs < 0 || nAgeMs >= TTL_MS)
                    continue;
                if(e.hash == hash)
                    return true;
            }
            return false;
        }

        /** Record a successful push at time `tNow`.  Overwrites the oldest
         *  slot (round-robin) on overflow.  If the hash is already present,
         *  refreshes its timestamp in-place rather than duplicating it. */
        void Record(const uint1024_t& hash,
            std::chrono::steady_clock::time_point tNow)
        {
            /* Refresh existing entry if present — keeps the ring populated
             * with distinct hashes rather than duplicates on storm recurrence. */
            for(Entry& e : m_entries)
            {
                if(e.tPushed != std::chrono::steady_clock::time_point{} && e.hash == hash)
                {
                    e.tPushed = tNow;
                    return;
                }
            }

            /* Prefer reusing a never-used or aged-out slot before
             * overwriting a still-fresh one. */
            for(Entry& e : m_entries)
            {
                if(e.tPushed == std::chrono::steady_clock::time_point{})
                {
                    e.hash = hash;
                    e.tPushed = tNow;
                    return;
                }
            }
            for(Entry& e : m_entries)
            {
                if(e.tPushed == std::chrono::steady_clock::time_point{})
                    continue;
                const int64_t nAgeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    tNow - e.tPushed).count();
                /* Treat clock-skew (negative age) as "still fresh — do not evict",
                 * matching ContainsRecent which treats negative age as "do not
                 * match".  This keeps the two methods consistent under clock
                 * adjustments. */
                if(nAgeMs >= 0 && nAgeMs >= TTL_MS)
                {
                    e.hash = hash;
                    e.tPushed = tNow;
                    return;
                }
            }

            /* All slots fresh — round-robin overwrite. */
            m_entries[m_nextSlot].hash = hash;
            m_entries[m_nextSlot].tPushed = tNow;
            m_nextSlot = (m_nextSlot + 1) % CAPACITY;
        }

        /** Clear — drop all recorded entries.  Used on SUBMIT_BLOCK SUCCESS
         *  (Option E) so the post-submit SetBest fan-out is guaranteed to
         *  deliver, even if the storm pre-poisoned the ring with the
         *  post-submit tip hash. */
        void Clear()
        {
            for(Entry& e : m_entries)
            {
                e.hash = uint1024_t{};
                e.tPushed = std::chrono::steady_clock::time_point{};
            }
            m_nextSlot = 0;
        }

        /** Diagnostic: number of currently-fresh entries. */
        std::size_t SizeRecent(std::chrono::steady_clock::time_point tNow) const
        {
            std::size_t n = 0;
            for(const Entry& e : m_entries)
            {
                if(e.tPushed == std::chrono::steady_clock::time_point{})
                    continue;
                const int64_t nAgeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    tNow - e.tPushed).count();
                if(nAgeMs >= 0 && nAgeMs < TTL_MS)
                    ++n;
            }
            return n;
        }

    private:
        std::array<Entry, CAPACITY> m_entries{};
        std::size_t m_nextSlot{0};
    };


    inline TemplatePushDecision ApplyTemplatePushThrottle(
        std::chrono::steady_clock::time_point& tLastTemplatePushTime,
        bool& fForceNextPush,
        uint1024_t& hashLastPushedChain,
        PushedTipHistory& historyPushedTips,
        const uint1024_t& hashCurrentChain,
        const std::chrono::steady_clock::time_point& tNow = std::chrono::steady_clock::now())
    {
        TemplatePushDecision result;
        const bool fHadPriorPush = (tLastTemplatePushTime != std::chrono::steady_clock::time_point{});
        result.nElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            tNow - tLastTemplatePushTime).count();

        if(!fHadPriorPush)
        {
            fForceNextPush = false;
            tLastTemplatePushTime = tNow;
            hashLastPushedChain = hashCurrentChain;
            historyPushedTips.Record(hashCurrentChain, tNow);
            result.fShouldSend = true;
            result.eReason = TemplatePushDecisionReason::FIRST_PUSH;
            return result;
        }

        if(fForceNextPush)
        {
            fForceNextPush = false;
            tLastTemplatePushTime = tNow;
            hashLastPushedChain = hashCurrentChain;
            historyPushedTips.Record(hashCurrentChain, tNow);
            result.fShouldSend = true;
            result.eReason = TemplatePushDecisionReason::FORCE_BYPASS;
            return result;
        }

        /* Hash-change bypass — keyed on the time-windowed history rather than
         * on the single most-recent pushed hash.  A tip is treated as "new"
         * (and therefore bypasses the time floor) only if it is NOT present
         * in the recent-push ring.  This closes the storm pre-poison hole
         * where SetBest cycling A→B→C→D→A would re-deliver A as if it were
         * new, exhausting the bypass and then silently stranding the miner. */
        if(!historyPushedTips.ContainsRecent(hashCurrentChain, tNow))
        {
            tLastTemplatePushTime = tNow;
            hashLastPushedChain = hashCurrentChain;
            historyPushedTips.Record(hashCurrentChain, tNow);
            result.fShouldSend = true;
            result.eReason = TemplatePushDecisionReason::CHAIN_TIP_CHANGED;
            return result;
        }

        if(result.nElapsedMs < MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS)
        {
            result.fShouldSend = false;
            result.eReason = TemplatePushDecisionReason::THROTTLED;
            return result;
        }

        tLastTemplatePushTime = tNow;
        hashLastPushedChain = hashCurrentChain;
        historyPushedTips.Record(hashCurrentChain, tNow);
        result.fShouldSend = true;
        result.eReason = TemplatePushDecisionReason::INTERVAL_ELAPSED;

        return result;
    }


    struct TemplateRefreshDecision
    {
        bool fUnifiedHeightChanged = false;
        bool fReorgDetected = false;
        bool fTemplateStale = false;
    };


    inline TemplateRefreshDecision EvaluateTemplateRefresh(
        uint32_t nLastTemplateUnifiedHeight,
        const uint1024_t& hashLastBlock,
        const RoundStateUtility::ChainHeightSnapshot& snap)
    {
        TemplateRefreshDecision result;
        result.fUnifiedHeightChanged = RoundStateUtility::IsTemplateStale(
            nLastTemplateUnifiedHeight, snap);
        result.fReorgDetected = RoundStateUtility::IsReorgDetected(hashLastBlock, snap);
        result.fTemplateStale = (result.fUnifiedHeightChanged || result.fReorgDetected);
        return result;
    }
}

#endif
