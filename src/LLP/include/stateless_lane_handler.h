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
