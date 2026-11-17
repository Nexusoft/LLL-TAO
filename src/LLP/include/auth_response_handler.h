/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_AUTH_RESPONSE_HANDLER_H
#define NEXUS_LLP_INCLUDE_AUTH_RESPONSE_HANDLER_H

#include <LLP/include/packet_handler.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** AuthResponseHandler
     *
     *  Per-opcode handler for MINER_AUTH_RESPONSE (Falcon signature verification).
     *
     *  Owns its own mutex so authentication processing doesn't contend with
     *  keepalive, get_block, or submit_block handlers.
     *
     *  This handler wraps StatelessMiner::ProcessFalconResponse() which is the
     *  pure-functional auth processor.  The handler adds mutex isolation.
     *
     **/
    class AuthResponseHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "AuthResponseHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(2, "PacketHandler: ", Name(), " processing AUTH_RESPONSE");

            return StatelessMiner::ProcessFalconResponse(context, packet);
        }
    };


    /** AuthInitHandler
     *
     *  Per-opcode handler for MINER_AUTH_INIT (start Falcon handshake).
     *
     *  Shares the same mutex domain as AuthResponseHandler would be ideal,
     *  but since PacketHandler base provides per-instance mutex, each auth
     *  phase gets its own lock.  Auth init and auth response are sequential
     *  (never concurrent for the same connection), so separate mutexes are safe.
     *
     **/
    class AuthInitHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "AuthInitHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(2, "PacketHandler: ", Name(), " processing AUTH_INIT");

            return StatelessMiner::ProcessMinerAuthInit(context, packet);
        }
    };


}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_AUTH_RESPONSE_HANDLER_H */
