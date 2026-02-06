/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_PUSH_NOTIFICATION_H
#define NEXUS_LLP_INCLUDE_PUSH_NOTIFICATION_H

#include <cstdint>
#include <vector>

/* Forward declarations */
namespace LLP { class Packet; class StatelessPacket; }
namespace TAO { namespace Ledger { class BlockState; } }

namespace LLP
{
    /** Protocol constants for mining **/
    static constexpr size_t TRITIUM_BLOCK_SIZE = 216;  // Serialized Tritium block template size

    /** ProtocolLane
     *
     *  Identifies which mining protocol lane (8-bit legacy or 16-bit stateless)
     *  is being used for opcode selection.
     *
     **/
    enum class ProtocolLane : uint8_t
    {
        LEGACY    = 0,  // 8-bit opcodes (port 8323)
        STATELESS = 1   // 16-bit opcodes (port 9323)
    };

    /** GetChannelName
     *
     *  Get the human-readable name for a mining channel.
     *
     *  @param[in] nChannel Mining channel (1=Prime, 2=Hash)
     *
     *  @return "Prime" for channel 1, "Hash" for channel 2, "Unknown" otherwise
     *
     **/
    inline const char* GetChannelName(uint32_t nChannel)
    {
        return (nChannel == 1) ? "Prime" : (nChannel == 2) ? "Hash" : "Unknown";
    }

    /** PushNotificationBuilder
     *
     *  Unified builder for push notification packets used across both
     *  legacy (8-bit) and stateless (16-bit) mining protocol lanes.
     *
     *  ARCHITECTURE:
     *  - Template-based design for type safety (Packet vs StatelessPacket)
     *  - Lane-aware opcode selection (8-bit vs 16-bit)
     *  - Eliminates 254 lines of duplicated packet building code
     *  - Enables push notifications on BOTH lanes
     *
     *  USAGE:
     *  Legacy lane:
     *    auto packet = PushNotificationBuilder::BuildChannelNotification<Packet>(
     *        nChannel, ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
     *
     *  Stateless lane:
     *    auto packet = PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
     *        nChannel, ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
     *
     **/
    class PushNotificationBuilder
    {
    public:
        /** BuildChannelNotification
         *
         *  Build a channel-specific push notification packet.
         *
         *  Template Parameters:
         *  @tparam PacketType Either Packet (8-bit) or StatelessPacket (16-bit)
         *
         *  @param[in] nChannel Mining channel (1=Prime, 2=Hash)
         *  @param[in] lane Protocol lane (LEGACY or STATELESS)
         *  @param[in] stateBest Current best block state
         *  @param[in] stateChannel Last block state for this channel
         *  @param[in] nDifficulty Current difficulty for this channel
         *
         *  @return Packet or StatelessPacket ready to send
         *
         *  PAYLOAD FORMAT (12 bytes, big-endian):
         *    [0-3]   uint32_t nUnifiedHeight   - Current blockchain height
         *    [4-7]   uint32_t nChannelHeight   - Channel-specific height
         *    [8-11]  uint32_t nDifficulty      - Current difficulty
         *
         **/
        template<typename PacketType>
        static PacketType BuildChannelNotification(
            uint32_t nChannel,
            ProtocolLane lane,
            const TAO::Ledger::BlockState& stateBest,
            const TAO::Ledger::BlockState& stateChannel,
            uint32_t nDifficulty);

    private:
        /** BuildPayload
         *
         *  Build the 12-byte notification payload (big-endian).
         *
         *  @param[in] nUnifiedHeight Current blockchain height
         *  @param[in] nChannelHeight Channel-specific height
         *  @param[in] nDifficulty Current difficulty
         *
         *  @return 12-byte vector containing the payload
         *
         **/
        static std::vector<uint8_t> BuildPayload(
            uint32_t nUnifiedHeight,
            uint32_t nChannelHeight,
            uint32_t nDifficulty);

        /** GetNotificationOpcode
         *
         *  Get the correct opcode for a channel notification.
         *
         *  @param[in] nChannel Mining channel (1=Prime, 2=Hash)
         *  @param[in] lane Protocol lane (LEGACY or STATELESS)
         *
         *  @return 8-bit opcode for LEGACY, 16-bit opcode for STATELESS
         *
         *  OPCODE MAPPING:
         *  - LEGACY lane:
         *    - Channel 1 (Prime): 0xD9 (217)
         *    - Channel 2 (Hash):  0xDA (218)
         *  - STATELESS lane:
         *    - Channel 1 (Prime): 0xD0D9 (53465)
         *    - Channel 2 (Hash):  0xD0DA (53466)
         *
         **/
        static uint16_t GetNotificationOpcode(uint32_t nChannel, ProtocolLane lane);
    };

} // namespace LLP

#endif
