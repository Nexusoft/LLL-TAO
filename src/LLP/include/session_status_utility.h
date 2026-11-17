/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_STATUS_UTILITY_H
#define NEXUS_LLP_INCLUDE_SESSION_STATUS_UTILITY_H

#include <cstdint>
#include <optional>
#include <vector>

#include <LLP/include/session_status.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/node_session_registry.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{

    /** SessionStatusUtility
     *
     *  Shared utility for SESSION_STATUS ACK building with canonical session
     *  registry tie-in.  Used by both the Legacy (miner.cpp, port 8323) and Stateless
     *  (stateless_miner_connection.cpp, port 9323) lanes.
     *
     *  Design:
     *  - Uses NodeSessionRegistry as the authoritative session lookup for both lanes.
     *  - SESSION_STATUS is a soft health probe; unknown session IDs are diagnostic.
     *  - If the wire session_id misses but the current authenticated hashKeyID is
     *    still known, the utility reconciles to the canonical session instead of
     *    treating the miner as bad.
     *  - Callers handle lane-specific response framing (8-bit vs 16-bit opcodes).
     *
     *  Session Disconnection Scenarios:
     *  ┌──────────────────────────────────────────────────────────────┐
     *  │ Miner disconnects on Port A, reconnects on Port B:         │
     *  │                                                              │
     *  │ Using GetMinerContextBySessionID():                          │
     *  │   - Session persists in StatelessMinerManager (keyed by      │
     *  │     address, not port).                                      │
     *  │   - Both lanes look up by session_id → finds same context.   │
     *  │   - If miner shuts down for 15s and reconnects, the session  │
     *  │     survives the CleanupInactive() sweep window.             │
     *  │     (See StatelessMinerManager::CleanupInactive())           │
     *  │   - Re-auth on new port creates fresh context → old session  │
     *  │     is orphaned and cleaned up during the next sweep.        │
     *  │                                                              │
     *  │ Using direct context.nSessionId (old stateless pattern):     │
     *  │   - Only works on the connection that created the session.   │
     *  │   - Cross-port SESSION_STATUS would fail (session_id         │
     *  │     mismatch) because the other lane's context is different. │
     *  │   - Miner reconnecting on different port would get rejected. │
     *  │                                                              │
     *  │ VERDICT: GetMinerContextBySessionID() is more robust.        │
     *  └──────────────────────────────────────────────────────────────┘
     *
     **/
    namespace SessionStatusUtility
    {

        /** ValidateAndBuildAck
         *
         *  Validate a SESSION_STATUS request against the canonical session registry and build
         *  the 16-byte ACK response payload.
         *
         *  Uses NodeSessionRegistry first, with authenticated hashKeyID-based
         *  reconciliation when the wire session_id misses.
         *
         *  @param[in] req         The parsed SESSION_STATUS request.
         *  @param[in] lane        Which lane this request arrived on:
         *                         LANE_PRIMARY_ALIVE (stateless) or
         *                         LANE_SECONDARY_ALIVE (legacy).
         *  @param[in] pCurrentContext Optional current connection snapshot for
         *                         authenticated session reconciliation.
         *  @param[out] fValid     Set to true if session was found and ACK built.
         *
         *  @return std::vector<uint8_t> containing the 16-byte ACK payload.
         *          On failure, returns ACK with zero lane_health (invalid session).
         *
         **/
        inline std::vector<uint8_t> ValidateAndBuildAck(
            const SessionStatus::SessionStatusRequest& req,
            uint32_t nLaneFlag,
            const MiningContext* pCurrentContext,
            bool& fValid)
        {
            fValid = false;

            std::optional<NodeSessionEntry> optEntry = NodeSessionRegistry::Get().Lookup(req.session_id);
            if(!optEntry.has_value() && pCurrentContext != nullptr
            && pCurrentContext->fAuthenticated && pCurrentContext->hashKeyID != 0)
            {
                const MiningContext& currentContext = *pCurrentContext;
                optEntry = NodeSessionRegistry::Get().LookupByKey(currentContext.hashKeyID);

                if(!optEntry.has_value() && currentContext.hashGenesis != 0)
                {
                    MiningContext canonicalContext = currentContext.WithSession(
                        MiningContext::DeriveSessionId(currentContext.hashKeyID));

                    auto [nCanonicalSessionId, fNewSession] = NodeSessionRegistry::Get().RegisterOrRefresh(
                        canonicalContext.hashKeyID,
                        canonicalContext.hashGenesis,
                        canonicalContext,
                        canonicalContext.nProtocolLane);

                    debug::log(1, FUNCTION,
                        "SESSION_STATUS: reconciled missing session via authenticated key=",
                        canonicalContext.hashKeyID.SubString(),
                        " requested=0x", std::hex, req.session_id,
                        " canonical=0x", nCanonicalSessionId, std::dec,
                        fNewSession ? " (re-registered)" : " (refreshed)");

                    /* Returning the canonical registry session ID here is the explicit
                     * reconciliation contract: the wire alias may drift, but the ACK
                     * repairs the miner back onto the registry-derived session. */
                    fValid = true;
                    return SessionStatus::BuildAckPayload(
                        nCanonicalSessionId,
                        nLaneFlag | SessionStatus::LANE_AUTHENTICATED,
                        static_cast<uint32_t>(canonicalContext.GetSessionDuration(runtime::unifiedtimestamp())),
                        req.status_flags);
                }

                if(optEntry.has_value() && req.session_id != optEntry->nSessionId)
                {
                    debug::log(1, FUNCTION,
                        "SESSION_STATUS: canonical session mismatch for key=",
                        pCurrentContext->hashKeyID.SubString(),
                        " requested=0x", std::hex, req.session_id,
                        " canonical=0x", optEntry->nSessionId, std::dec);
                }
            }

            if(!optEntry.has_value())
            {
                /* Intentional severity downgrade: SESSION_STATUS is a soft health probe,
                 * so a miss is diagnostic telemetry rather than a miner fault. */
                debug::log(1, FUNCTION, "SESSION_STATUS: diagnostic miss for session_id=0x",
                           std::hex, req.session_id, std::dec);
                return SessionStatus::BuildAckPayload(req.session_id, nLaneFlag, 0u, req.status_flags);
            }

            fValid = true;

            /* Build lane health flags */
            uint32_t nLaneHealth = nLaneFlag | SessionStatus::LANE_AUTHENTICATED;
            if(optEntry->fStatelessLive)
                nLaneHealth |= SessionStatus::LANE_PRIMARY_ALIVE;
            if(optEntry->fLegacyLive)
                nLaneHealth |= SessionStatus::LANE_SECONDARY_ALIVE;

            /* Calculate session uptime */
            const uint32_t nUptime = static_cast<uint32_t>(
                optEntry->context.GetSessionDuration(runtime::unifiedtimestamp()));

            return SessionStatus::BuildAckPayload(
                optEntry->nSessionId, nLaneHealth, nUptime, req.status_flags);
        }


        /** IsDegraded
         *
         *  Check if the miner is reporting DEGRADED status.
         *
         *  @param[in] req  The parsed SESSION_STATUS request.
         *
         *  @return True if MINER_DEGRADED flag is set.
         *
         **/
        inline bool IsDegraded(const SessionStatus::SessionStatusRequest& req)
        {
            return (req.status_flags & SessionStatus::MINER_DEGRADED) != 0;
        }

    }  /* namespace SessionStatusUtility */

}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_SESSION_STATUS_UTILITY_H */
