/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_LEGACY_LANE_HANDLER_H
#define NEXUS_LLP_INCLUDE_LEGACY_LANE_HANDLER_H

#include <LLP/include/packet_handler.h>
#include <LLP/include/auth_response_handler.h>
#include <LLP/include/session_start_handler.h>
#include <LLP/include/channel_reward_handler.h>
#include <LLP/include/opcode_utility.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** LegacyLaneHandler
     *
     *  Packet dispatch layer for the Legacy Mining lane (port 8323).
     *
     *  The legacy lane receives 8-bit opcodes.  Before routing to handlers,
     *  opcodes are converted to their 16-bit mirrored equivalents so handlers
     *  are shared between both lanes.
     *
     *  Owns a PacketHandlerRegistry with per-opcode handlers, each with
     *  independent mutexes.  This provides the same MUTEX isolation benefits
     *  as the StatelessLaneHandler.
     *
     *  Usage:
     *    LegacyLaneHandler handler;
     *    uint16_t nStatelessOpcode = OpcodeUtility::Stateless::Mirror(PACKET.HEADER);
     *    if(handler.CanHandle(nStatelessOpcode))
     *    {
     *        // Convert legacy Packet to StatelessPacket for unified handler interface
     *        StatelessPacket sp(nStatelessOpcode);
     *        sp.DATA = PACKET.DATA;
     *        sp.LENGTH = PACKET.LENGTH;
     *        ProcessResult result = handler.Dispatch(context, sp);
     *        // handle result...
     *    }
     *
     *  Opcodes NOT registered here (GET_BLOCK, SUBMIT_BLOCK, etc.) are
     *  handled by the Miner connection layer directly.
     *
     **/
    class LegacyLaneHandler
    {
    public:

        LegacyLaneHandler()
        {
            /* Register per-opcode handlers using 16-bit mirrored opcodes.
             * The caller mirrors 8-bit → 16-bit before lookup. */

            using StatelessOpcodes = OpcodeUtility::Stateless;

            /* Authentication handlers */
            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::MINER_AUTH_INIT),
                std::make_shared<AuthInitHandler>());

            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::MINER_AUTH_RESPONSE),
                std::make_shared<AuthResponseHandler>());

            /* Session handlers */
            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::SESSION_START),
                std::make_shared<SessionStartHandler>());

            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::SESSION_KEEPALIVE),
                std::make_shared<SessionKeepaliveHandler>());

            /* Configuration handlers */
            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::SET_CHANNEL),
                std::make_shared<SetChannelHandler>());

            m_registry.Register(StatelessOpcodes::Mirror(OpcodeUtility::Opcodes::MINER_SET_REWARD),
                std::make_shared<SetRewardHandler>());
        }


        /** CanHandle
         *
         *  Check if this lane handler has a registered handler for the 16-bit opcode.
         *  Caller must mirror 8-bit → 16-bit before calling.
         *
         **/
        bool CanHandle(uint16_t nMirroredOpcode) const
        {
            return m_registry.HasHandler(nMirroredOpcode);
        }


        /** Dispatch
         *
         *  Route a packet to the appropriate per-opcode handler.
         *  Packet must already have the 16-bit mirrored opcode in HEADER.
         *
         *  @param[in] context  Current mining context.
         *  @param[in] packet   The incoming packet (with mirrored opcode).
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
                debug::log(1, "LegacyLaneHandler: No handler for opcode 0x",
                           std::hex, uint32_t(packet.HEADER), std::dec);
                return ProcessResult::Error(context, "Unknown packet type");
            }

            debug::log(3, "LegacyLaneHandler: Dispatching to ", pHandler->Name(),
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

#endif /* NEXUS_LLP_INCLUDE_LEGACY_LANE_HANDLER_H */
