/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STALE_SYNC_DIAGNOSTICS_H
#define NEXUS_LLP_INCLUDE_STALE_SYNC_DIAGNOSTICS_H

#include <cstddef>
#include <cstdint>
#include <map>

namespace LLP
{
    inline constexpr std::size_t STALE_SYNC_WARNING_MAX_ENTRIES = 256;
    inline constexpr uint64_t STALE_SYNC_WARNING_THROTTLE_SECONDS = 60;


    struct StaleSyncWarningState
    {
        uint64_t nLastWarningTime = 0;
        uint64_t nSuppressedBlocks = 0;
        bool fHasEmittedWarning = false;
    };


    struct StaleSyncWarningDecision
    {
        bool fEmitWarning = false;
        uint64_t nSuppressedBlocks = 0;
    };


    /** RecordStaleSyncWarningEvent
     *
     *  Update the per-session throttling state for one rejected stale SYNC
     *  block and return whether the caller should emit a warning now.
     *
     *  - First event for a session emits immediately.
     *  - Subsequent events inside `nThrottleSeconds` are counted and suppressed.
     *  - The first event after the interval emits and reports the accumulated
     *    suppressed-block count since the previous report.
     *  - If `mapStates` is already at `nMaxEntries` when a new session arrives,
     *    the map is cleared before inserting the new entry. This intentionally
     *    mirrors the cheap clear-on-cap policy used by other LLP warning maps.
     *  - If `nNow` moves backward relative to the stored timestamp, the warning
     *    emits immediately and resets the timestamp to avoid indefinite silence.
     *  - If `nNow` jumps far forward, the interval comparison naturally makes
     *    the next event eligible immediately, after which throttling resumes
     *    from the new timestamp.
     *
     *  Thread safety: this function does not lock `mapStates`; callers must
     *  provide any synchronization required by their connection model. */
    inline StaleSyncWarningDecision RecordStaleSyncWarningEvent(
        std::map<uint64_t, StaleSyncWarningState>& mapStates,
        const uint64_t nStaleSession,
        const uint64_t nNow,
        const uint64_t nThrottleSeconds = STALE_SYNC_WARNING_THROTTLE_SECONDS,
        const std::size_t nMaxEntries = STALE_SYNC_WARNING_MAX_ENTRIES)
    {
        auto it = mapStates.find(nStaleSession);
        if(it == mapStates.end())
        {
            /* Intentional clear-on-cap policy: this mirrors the existing
             * bounded warning maps used in LLP hot paths and avoids an
             * O(n) oldest-entry scan on every first-seen stale session. */
            if(mapStates.size() >= nMaxEntries)
                mapStates.clear();

            it = mapStates.emplace(nStaleSession, StaleSyncWarningState{}).first;
        }

        StaleSyncWarningState& state = it->second;
        ++state.nSuppressedBlocks;

        /* If wall-clock time moves backward (for example after a system-time
         * adjustment), emit immediately and reset the stored timestamp so a
         * future-valued last-warning time cannot suppress diagnostics
         * indefinitely after the clock skew resolves. */
        if(!state.fHasEmittedWarning
        || nNow < state.nLastWarningTime
        || nNow - state.nLastWarningTime >= nThrottleSeconds)
        {
            StaleSyncWarningDecision decision;
            decision.fEmitWarning = true;
            decision.nSuppressedBlocks = state.nSuppressedBlocks;

            state.nLastWarningTime = nNow;
            state.nSuppressedBlocks = 0;
            state.fHasEmittedWarning = true;

            return decision;
        }

        return {};
    }


    /** ResetStaleSyncWarningEvent
     *
     *  Remove the throttling state for `nStaleSession`, typically on disconnect
     *  so a recycled or newly-active connection starts with a clean limiter.
     *
     *  @return true if an entry was present and erased, false otherwise.
     *
     *  Thread safety: this function does not lock `mapStates`; callers must
     *  provide any synchronization required by their connection model. */
    inline bool ResetStaleSyncWarningEvent(
        std::map<uint64_t, StaleSyncWarningState>& mapStates,
        const uint64_t nStaleSession)
    {
        return mapStates.erase(nStaleSession) != 0;
    }
}

#endif
