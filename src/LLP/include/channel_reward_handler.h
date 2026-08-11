/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CHANNEL_REWARD_HANDLER_H
#define NEXUS_LLP_INCLUDE_CHANNEL_REWARD_HANDLER_H

#include <LLP/include/packet_handler.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** SetChannelHandler
     *
     *  Per-opcode handler for SET_CHANNEL.
     *
     *  Owns its own mutex so channel selection doesn't contend with
     *  auth, session, or block operations.
     *
     **/
    class SetChannelHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "SetChannelHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(2, "PacketHandler: ", Name(), " processing SET_CHANNEL");

            return StatelessMiner::ProcessSetChannel(context, packet);
        }
    };


    /** SetRewardHandler
     *
     *  Per-opcode handler for MINER_SET_REWARD.
     *
     *  Owns its own mutex so reward binding (which involves ChaCha20 decryption)
     *  doesn't contend with auth, session, or block operations.
     *
     **/
    class SetRewardHandler : public PacketHandler
    {
    public:

        std::string Name() const override
        {
            return "SetRewardHandler";
        }

        ProcessResult Handle(
            const MiningContext& context,
            const StatelessPacket& packet) override
        {
            std::lock_guard<std::mutex> lk(handler_mutex);

            debug::log(2, "PacketHandler: ", Name(), " processing SET_REWARD");

            return StatelessMiner::ProcessSetReward(context, packet);
        }
    };


}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_CHANNEL_REWARD_HANDLER_H */
