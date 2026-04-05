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

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{

    /** SessionStatusUtility
     *
     *  Shared utility for SESSION_STATUS ACK building with robust SessionManager
     *  tie-in.  Used by both the Legacy (miner.cpp, port 8323) and Stateless
     *  (stateless_miner_connection.cpp, port 9323) lanes.
     *
     *  Design:
     *  - Uses StatelessMinerManager::GetMinerContextBySessionID() as the
     *    authoritative session lookup for both lanes (legacy pattern preferred).
     *  - Pure functions: no internal mutexes, no side effects.
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
         *  Validate a SESSION_STATUS request against the SessionManager and build
         *  the 16-byte ACK response payload.
         *
         *  Uses StatelessMinerManager::GetMinerContextBySessionID() for robust
         *  session lookup that works across both lanes and survives reconnections.
         *
         *  @param[in] req         The parsed SESSION_STATUS request.
         *  @param[in] lane        Which lane this request arrived on:
         *                         LANE_PRIMARY_ALIVE (stateless) or
         *                         LANE_SECONDARY_ALIVE (legacy).
         *  @param[out] fValid     Set to true if session was found and ACK built.
         *
         *  @return std::vector<uint8_t> containing the 16-byte ACK payload.
         *          On failure, returns ACK with zero lane_health (invalid session).
         *
         **/
        inline std::vector<uint8_t> ValidateAndBuildAck(
            const SessionStatus::SessionStatusRequest& req,
            uint32_t nLaneFlag,
            bool& fValid)
        {
            fValid = false;

            /* Look up session by ID through the SessionManager.
             * This is the authoritative lookup used by both lanes. */
            auto optContext = StatelessMinerManager::Get().GetMinerContextBySessionID(req.session_id);
            if(!optContext.has_value())
            {
                debug::error(FUNCTION, "SESSION_STATUS: unknown session_id=0x",
                             std::hex, req.session_id, std::dec);
                return SessionStatus::BuildAckPayload(req.session_id, 0u, 0u, req.status_flags);
            }

            fValid = true;

            /* Build lane health flags */
            uint32_t nLaneHealth = 0;
            nLaneHealth |= nLaneFlag;
            nLaneHealth |= SessionStatus::LANE_AUTHENTICATED;

            /* Calculate session uptime */
            const uint32_t nUptime = static_cast<uint32_t>(
                optContext->GetSessionDuration(runtime::unifiedtimestamp()));

            return SessionStatus::BuildAckPayload(
                req.session_id, nLaneHealth, nUptime, req.status_flags);
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
