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
    static constexpr std::size_t STALE_SYNC_WARNING_MAX_ENTRIES = 256;
    static constexpr uint64_t STALE_SYNC_WARNING_THROTTLE_SECONDS = 60;


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
            if(mapStates.size() >= nMaxEntries)
                mapStates.clear();

            it = mapStates.emplace(nStaleSession, StaleSyncWarningState{}).first;
        }

        StaleSyncWarningState& state = it->second;
        ++state.nSuppressedBlocks;

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


    inline bool ResetStaleSyncWarningEvent(
        std::map<uint64_t, StaleSyncWarningState>& mapStates,
        const uint64_t nStaleSession)
    {
        return mapStates.erase(nStaleSession) != 0;
    }
}

#endif
