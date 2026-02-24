/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/push_notification.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/opcode_utility.h>
#include <TAO/Ledger/types/state.h>

namespace LLP
{
    /* BuildPayload - Build 140-byte notification payload (big-endian) */
    std::vector<uint8_t> PushNotificationBuilder::BuildPayload(
        uint32_t nUnifiedHeight,
        uint32_t nChannelHeight,
        uint32_t nDifficulty,
        const uint1024_t& hashBestChain)
    {
        std::vector<uint8_t> vPayload;
        vPayload.reserve(140);  // 12 bytes header + 128 bytes hash

        // Unified height [0-3] (big-endian)
        vPayload.push_back((nUnifiedHeight >> 24) & 0xFF);
        vPayload.push_back((nUnifiedHeight >> 16) & 0xFF);
        vPayload.push_back((nUnifiedHeight >> 8) & 0xFF);
        vPayload.push_back((nUnifiedHeight >> 0) & 0xFF);
        
        // Channel height [4-7] (big-endian)
        vPayload.push_back((nChannelHeight >> 24) & 0xFF);
        vPayload.push_back((nChannelHeight >> 16) & 0xFF);
        vPayload.push_back((nChannelHeight >> 8) & 0xFF);
        vPayload.push_back((nChannelHeight >> 0) & 0xFF);
        
        // Difficulty [8-11] (big-endian)
        vPayload.push_back((nDifficulty >> 24) & 0xFF);
        vPayload.push_back((nDifficulty >> 16) & 0xFF);
        vPayload.push_back((nDifficulty >> 8) & 0xFF);
        vPayload.push_back((nDifficulty >> 0) & 0xFF);

        // hashBestChain [12-139] (128 bytes, little-endian word order via GetBytes())
        // Allows miner to compare its template's hashPrevBlock against this value
        // for hash-based staleness detection (NexusMiner#170 pattern).
        std::vector<uint8_t> vHashBytes = hashBestChain.GetBytes();
        vPayload.insert(vPayload.end(), vHashBytes.begin(), vHashBytes.end());
        
        return vPayload;
    }

    /* GetNotificationOpcode - Lane-aware opcode selection */
    uint16_t PushNotificationBuilder::GetNotificationOpcode(uint32_t nChannel, ProtocolLane lane)
    {
        if (lane == ProtocolLane::LEGACY)
        {
            // 8-bit opcodes for legacy lane (port 8323)
            return (nChannel == 1) 
                ? OpcodeUtility::Opcodes::PRIME_BLOCK_AVAILABLE  // 0xD9
                : OpcodeUtility::Opcodes::HASH_BLOCK_AVAILABLE;  // 0xDA
        }
        else
        {
            // 16-bit opcodes for stateless lane (port 9323)
            return (nChannel == 1)
                ? OpcodeUtility::Stateless::PRIME_BLOCK_AVAILABLE  // 0xD0D9
                : OpcodeUtility::Stateless::HASH_BLOCK_AVAILABLE;  // 0xD0DA
        }
    }

    /* Template specialization for 8-bit Packet (legacy lane) */
    template<>
    Packet PushNotificationBuilder::BuildChannelNotification<Packet>(
        uint32_t nChannel,
        ProtocolLane lane,
        const TAO::Ledger::BlockState& stateBest,
        const TAO::Ledger::BlockState& stateChannel,
        uint32_t nDifficulty,
        const uint1024_t& hashBestChain)
    {
        // Get 8-bit opcode (lane should be LEGACY)
        uint16_t nOpcode = GetNotificationOpcode(nChannel, lane);
        
        // Create packet with 8-bit header
        Packet notification;
        notification.HEADER = static_cast<uint8_t>(nOpcode);
        
        // Build 140-byte payload (12 header + 128 hash)
        notification.DATA = BuildPayload(
            stateBest.nHeight,
            stateChannel.nChannelHeight,
            nDifficulty,
            hashBestChain
        );
        
        notification.LENGTH = static_cast<uint32_t>(notification.DATA.size());
        
        return notification;
    }

    /* Template specialization for 16-bit StatelessPacket (stateless lane) */
    template<>
    StatelessPacket PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
        uint32_t nChannel,
        ProtocolLane lane,
        const TAO::Ledger::BlockState& stateBest,
        const TAO::Ledger::BlockState& stateChannel,
        uint32_t nDifficulty,
        const uint1024_t& hashBestChain)
    {
        // Get 16-bit opcode (lane should be STATELESS)
        uint16_t nOpcode = GetNotificationOpcode(nChannel, lane);
        
        // Create packet with 16-bit header
        StatelessPacket notification(nOpcode);
        
        // Build 140-byte payload (12 header + 128 hash)
        notification.DATA = BuildPayload(
            stateBest.nHeight,
            stateChannel.nChannelHeight,
            nDifficulty,
            hashBestChain
        );
        
        notification.LENGTH = static_cast<uint32_t>(notification.DATA.size());
        
        return notification;
    }

} // namespace LLP
