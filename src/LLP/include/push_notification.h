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
#include <LLC/types/uint1024.h>

/* Forward declarations */
namespace LLP { class Packet; class StatelessPacket; struct PingFrame; }
namespace TAO { namespace Ledger { class BlockState; } }

namespace LLP
{
    /** Protocol constants for mining **/
    static constexpr size_t TRITIUM_BLOCK_SIZE = 216;  // Serialized Tritium block template size

    /** Colin PING/PONG diagnostic opcode constants **/
    namespace ColinDiagOpcodes
    {
        /* Stateless lane (16-bit) */
        static constexpr uint16_t PING_DIAG_STATELESS = 0xD0E0;
        static constexpr uint16_t PONG_DIAG_STATELESS = 0xD0E1;

        /* Legacy lane (8-bit, stored as uint16_t for uniformity) */
        static constexpr uint16_t PING_DIAG_LEGACY    = 0x00E0;
        static constexpr uint16_t PONG_DIAG_LEGACY    = 0x00E1;
    }

    /** ProtocolLane
     *
     *  Identifies which mining protocol lane (8-bit legacy or 16-bit stateless)
     *  is being used for opcode selection.
     *
     **/
    enum class ProtocolLane : uint8_t
    {
        LEGACY    = 0,  // Legacy Tritium Protocol: 8-bit opcodes (port 8323)
        STATELESS = 1   // Stateless Tritium Protocol: 16-bit opcodes (port 9323)
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
     *        nChannel, ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty, hashBestChain);
     *
     *  Stateless lane:
     *    auto packet = PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
     *        nChannel, ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty, hashBestChain);
     *
     **/
    class PushNotificationBuilder
    {
    public:
        /** BestChainHashForNotification
         *
         *  Resolve the best-chain hash to embed in a push notification.
         *  Prefers the hash from the loaded best-state snapshot so the
         *  notification hash stays consistent with the state/height fields
         *  even while SetBest() is publishing tStateBest and hashBestChain.
         *
         *  @param[in] stateBest Current best block state snapshot
         *
         *  @return Hash to embed in the notification payload
         *
         **/
        static uint1024_t BestChainHashForNotification(
            const TAO::Ledger::BlockState& stateBest);

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
         *  @param[in] hashBestChain Current best-chain hash for hash-based staleness detection
         *
         *  @return Packet or StatelessPacket ready to send
         *
         *  PAYLOAD FORMAT (148 bytes, big-endian):
         *    [0-3]     uint32_t  nUnifiedHeight       - Current blockchain height
         *    [4-7]     uint32_t  nChannelHeight       - Channel-specific height (OWN channel)
         *    [8-11]    uint32_t  nDifficulty          - Current difficulty
         *    [12-15]   uint32_t  nOtherChannelHeight  - Other PoW channel height (NEW)
         *                                               (Prime push → Hash height; Hash push → Prime height)
         *    [16-19]   uint32_t  nStakeHeight         - Stake channel height (NEW)
         *    [20-147]  uint1024_t hashBestChain       - Best-chain hash (128 bytes, little-endian)
         *
         *  The hashBestChain field (bytes 20-147) allows the miner to compare its current
         *  template's hashPrevBlock against the node's current tip, enabling hash-based
         *  staleness detection before submitting work (NexusMiner#170 pattern).
         *
         **/
        template<typename PacketType>
        static PacketType BuildChannelNotification(
            uint32_t nChannel,
            ProtocolLane lane,
            const TAO::Ledger::BlockState& stateBest,
            const TAO::Ledger::BlockState& stateChannel,
            uint32_t nDifficulty,
            const uint1024_t& hashBestChain = uint1024_t(0));

        /** BuildPingDiagPacket
         *
         *  Serialize a PingFrame into a lane-appropriate packet for wire transmission.
         *
         *  @tparam PacketType  Packet (legacy) or StatelessPacket (stateless)
         *  @param[in] frame    Fully populated PingFrame
         *  @param[in] lane     Protocol lane
         *
         *  @return PacketType with opcode set and DATA = frame.Serialize()
         *
         **/
        template<typename PacketType>
        static PacketType BuildPingDiagPacket(const PingFrame& frame, ProtocolLane lane);

    private:
        /** BuildPayload
         *
         *  Build the 148-byte notification payload (big-endian).
         *
         *  @param[in] nUnifiedHeight      Current blockchain height
         *  @param[in] nChannelHeight      Channel-specific height (OWN channel)
         *  @param[in] nDifficulty         Current difficulty
         *  @param[in] nOtherChannelHeight Other PoW channel height (Prime push→Hash; Hash push→Prime)
         *  @param[in] nStakeHeight        Stake channel height
         *  @param[in] hashBestChain       Best-chain hash (128 bytes appended)
         *
         *  @return 148-byte vector containing the payload
         *
         **/
        static std::vector<uint8_t> BuildPayload(
            uint32_t nUnifiedHeight,
            uint32_t nChannelHeight,
            uint32_t nDifficulty,
            uint32_t nOtherChannelHeight,
            uint32_t nStakeHeight,
            const uint1024_t& hashBestChain = uint1024_t(0));

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
