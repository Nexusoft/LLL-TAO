/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_LANE_HANDLER_H
#define NEXUS_LLP_INCLUDE_STATELESS_LANE_HANDLER_H

#include <LLP/include/packet_handler.h>
#include <LLP/include/auth_response_handler.h>
#include <LLP/include/session_start_handler.h>
#include <LLP/include/channel_reward_handler.h>
#include <LLP/include/stateless_opcodes.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** StatelessLaneHandler
     *
     *  Packet dispatch layer for the Stateless Mining lane (port 9323).
     *
     *  Owns a PacketHandlerRegistry with per-opcode handlers, each with
     *  independent mutexes.  This replaces the monolithic if/else dispatch
     *  chain for auth/session/config opcodes, providing MUTEX isolation
     *  between opcode services.
     *
     *  Usage:
     *    StatelessLaneHandler handler;
     *    if(handler.CanHandle(packet.HEADER))
     *    {
     *        ProcessResult result = handler.Dispatch(context, packet);
     *        // handle result...
     *    }
     *
     *  Opcodes NOT registered here (GET_BLOCK, SUBMIT_BLOCK, etc.) are
     *  handled by the connection layer directly, preserving existing
     *  block-management state patterns.
     *
     *  ═══════════════════════════════════════════════════════════════════
     *  ALIAS ANALYSIS: StatelessMiner::ProcessPacket() → m_laneHandler.Dispatch()
     *  ═══════════════════════════════════════════════════════════════════
     *
     *  Q: Should StatelessMiner::ProcessPacket() be aliased to
     *     m_laneHandler.Dispatch() on the stateless lane (port 9323)?
     *
     *  RECOMMENDATION: NO — not yet.  A thin compatibility shim is sound
     *  hygiene, but a direct alias is premature at this stage.  Here's why:
     *
     *  1. ARCHITECTURAL ROLE MISMATCH:
     *     StatelessMiner::ProcessPacket() is a static pure-functional router
     *     used by BOTH lanes.  The legacy lane already uses it as a fallback
     *     for opcodes LegacyLaneHandler doesn't cover (GET_BLOCK → "Unknown
     *     packet type" → ProcessPacketStateless).  Aliasing it on the
     *     stateless lane would break that shared fallback semantic.
     *
     *  2. HANDLER COVERAGE IS PARTIAL:
     *     StatelessLaneHandler only registers 6 opcodes (AUTH_INIT,
     *     AUTH_RESPONSE, SESSION_START, SESSION_KEEPALIVE, SET_CHANNEL,
     *     SET_REWARD).  The remaining ~12 cases in the switch (GET_BLOCK,
     *     SUBMIT_BLOCK, BLOCK_DATA, GET_HEIGHT, etc.) still need the
     *     StatelessMiner::ProcessPacket() switch for routing to the
     *     connection layer.  A full alias would require registering ALL
     *     opcodes — block operations included.
     *
     *  3. MIGRATION PATH — RECOMMENDED APPROACH:
     *     a. Phase 1 (DONE - Legacy lane): Wire m_laneHandler.Dispatch() for
     *        the 6 auth/session/config opcodes.  Unhandled opcodes fall through
     *        to StatelessMiner::ProcessPacket().
     *     b. Phase 2 (FUTURE): Add SubmitBlockHandler, GetBlockHandler, etc.
     *        with their own mutexes.  When ALL opcodes have handlers, THEN
     *        StatelessMiner::ProcessPacket() can be retired and both lanes
     *        dispatch exclusively via their lane handlers.
     *     c. Phase 3 (FUTURE): StatelessMiner class becomes a namespace of
     *        pure-functional helpers (BuildAuthMessage, etc.) with no routing.
     *
     *  4. RE-AUTH READINESS:
     *     The handler architecture already enables re-authentication:
     *     any handler can call SessionStartPacket::BuildPayload() to rebuild
     *     a SESSION_START without duplicating serialization logic.  This makes
     *     reconnect/re-auth possible from any handler context.
     *
     *  SUMMARY: Keep StatelessMiner::ProcessPacket() as the stateless lane's
     *  primary router for now.  m_laneHandler (StatelessLaneHandler) is wired
     *  and available but should not be aliased to ProcessPacket() until all
     *  opcodes have per-class handlers.
     *  ═══════════════════════════════════════════════════════════════════
     *
     **/
    class StatelessLaneHandler
    {
    public:

        StatelessLaneHandler()
        {
            /* Register per-opcode handlers with their own mutexes.
             * Each handler is a separate instance → separate mutex. */

            /* Authentication handlers */
            m_registry.Register(StatelessOpcodes::AUTH_INIT,
                std::make_shared<AuthInitHandler>());

            m_registry.Register(StatelessOpcodes::AUTH_RESPONSE,
                std::make_shared<AuthResponseHandler>());

            /* Session handlers */
            m_registry.Register(StatelessOpcodes::SESSION_START,
                std::make_shared<SessionStartHandler>());

            m_registry.Register(StatelessOpcodes::SESSION_KEEPALIVE,
                std::make_shared<SessionKeepaliveHandler>());

            /* Configuration handlers */
            m_registry.Register(StatelessOpcodes::SET_CHANNEL,
                std::make_shared<SetChannelHandler>());

            m_registry.Register(StatelessOpcodes::SET_REWARD,
                std::make_shared<SetRewardHandler>());
        }


        /** CanHandle
         *
         *  Check if this lane handler has a registered handler for the opcode.
         *
         **/
        bool CanHandle(uint16_t nOpcode) const
        {
            return m_registry.HasHandler(nOpcode);
        }


        /** Dispatch
         *
         *  Route a packet to the appropriate per-opcode handler.
         *
         *  @param[in] context  Current mining context.
         *  @param[in] packet   The incoming packet.
         *
         *  @return ProcessResult from the handler, or Error if no handler found.
         *
         **/
        ProcessResult Dispatch(
            const MiningContext& context,
            const StatelessPacket& packet)
        {
            PacketHandler* pHandler = m_registry.Lookup(packet.HEADER);
            if(pHandler == nullptr)
            {
                debug::log(1, "StatelessLaneHandler: No handler for opcode 0x",
                           std::hex, uint32_t(packet.HEADER), std::dec);
                return ProcessResult::Error(context, "Unknown packet type");
            }

            debug::log(3, "StatelessLaneHandler: Dispatching to ", pHandler->Name(),
                       " for opcode 0x", std::hex, uint32_t(packet.HEADER), std::dec);

            return pHandler->Handle(context, packet);
        }


        /** GetRegistry
         *
         *  Access the underlying registry (for diagnostics/introspection).
         *
         **/
        const PacketHandlerRegistry& GetRegistry() const
        {
            return m_registry;
        }

    private:
        PacketHandlerRegistry m_registry;
    };


}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_STATELESS_LANE_HANDLER_H */
