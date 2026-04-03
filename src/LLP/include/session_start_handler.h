/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_START_HANDLER_H
#define NEXUS_LLP_INCLUDE_SESSION_START_HANDLER_H

#include <LLP/include/packet_handler.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** SessionStartHandler
     *
     *  Per-opcode handler for SESSION_START (miner-initiated session request).
     *
     *  Owns its own mutex so session negotiation doesn't contend with
     *  auth, keepalive, or block operations.
     *
     *  This handler wraps StatelessMiner::ProcessSessionStart() which now
     *  validates whether a session was already auto-started at auth time and
     *  handles re-negotiation gracefully (preserving keepalive counters).
     *
     **/
    class SessionStartHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "SessionStartHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(2, "PacketHandler: ", Name(), " processing SESSION_START");

            return StatelessMiner::ProcessSessionStart(context, packet);
        }
    };


    /** SessionKeepaliveHandler
     *
     *  Per-opcode handler for SESSION_KEEPALIVE.
     *
     *  Owns its own mutex so keepalive processing (which happens frequently)
     *  doesn't contend with auth or block operations.
     *
     **/
    class SessionKeepaliveHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "SessionKeepaliveHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            /* Keepalive is high-frequency and lock-free in StatelessMiner.
             * The handler mutex is still acquired for context consistency. */
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(3, "PacketHandler: ", Name(), " processing SESSION_KEEPALIVE");

            return StatelessMiner::ProcessSessionKeepalive(context, packet);
        }
    };


}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_SESSION_START_HANDLER_H */
