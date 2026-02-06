/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/push_notification.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/opcode_utility.h>
#include <TAO/Ledger/types/state.h>

#include <vector>
#include <cstdint>

/* Test suite for unified PushNotificationBuilder
 * 
 * Tests the builder's ability to construct channel notification packets
 * for both legacy (8-bit) and stateless (16-bit) mining protocol lanes.
 * 
 * Key test areas:
 * - Opcode selection (lane-aware)
 * - Payload construction (12-byte big-endian)
 * - Template specialization (Packet vs StatelessPacket)
 * - Channel-specific opcodes (Prime vs Hash)
 */

TEST_CASE("PushNotificationBuilder - Opcode Selection", "[push_notification][llp]")
{
    SECTION("Legacy lane - Prime channel (1)")
    {
        /* Create mock block states */
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 1000;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 333;
        
        uint32_t nDifficulty = 0x12345678;
        
        /* Build notification using legacy lane */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify 8-bit opcode for Prime */
        REQUIRE(notification.HEADER == LLP::OpcodeUtility::Opcodes::PRIME_BLOCK_AVAILABLE);
        REQUIRE(notification.HEADER == 0xD9);
        REQUIRE(notification.HEADER == 217);
    }
    
    SECTION("Legacy lane - Hash channel (2)")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 1000;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 667;
        
        uint32_t nDifficulty = 0x87654321;
        
        /* Build notification using legacy lane */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            2, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify 8-bit opcode for Hash */
        REQUIRE(notification.HEADER == LLP::OpcodeUtility::Opcodes::HASH_BLOCK_AVAILABLE);
        REQUIRE(notification.HEADER == 0xDA);
        REQUIRE(notification.HEADER == 218);
    }
    
    SECTION("Stateless lane - Prime channel (1)")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 2000;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 666;
        
        uint32_t nDifficulty = 0xABCDEF01;
        
        /* Build notification using stateless lane */
        LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            1, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        /* Verify 16-bit opcode for Prime */
        REQUIRE(notification.HEADER == LLP::OpcodeUtility::Stateless::PRIME_BLOCK_AVAILABLE);
        REQUIRE(notification.HEADER == 0xD0D9);
        REQUIRE(notification.HEADER == 53465);
    }
    
    SECTION("Stateless lane - Hash channel (2)")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 2000;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 1334;
        
        uint32_t nDifficulty = 0x01EFCDAB;
        
        /* Build notification using stateless lane */
        LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            2, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        /* Verify 16-bit opcode for Hash */
        REQUIRE(notification.HEADER == LLP::OpcodeUtility::Stateless::HASH_BLOCK_AVAILABLE);
        REQUIRE(notification.HEADER == 0xD0DA);
        REQUIRE(notification.HEADER == 53466);
    }
}

TEST_CASE("PushNotificationBuilder - Payload Construction", "[push_notification][llp]")
{
    SECTION("12-byte payload format - big-endian encoding")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 0x12345678;  // Unified height
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 0xABCDEF01;  // Channel height
        
        uint32_t nDifficulty = 0xDEADBEEF;  // Difficulty
        
        /* Build notification */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify payload size */
        REQUIRE(notification.LENGTH == 12);
        REQUIRE(notification.DATA.size() == 12);
        
        /* Verify unified height (bytes 0-3, big-endian) */
        REQUIRE(notification.DATA[0] == 0x12);
        REQUIRE(notification.DATA[1] == 0x34);
        REQUIRE(notification.DATA[2] == 0x56);
        REQUIRE(notification.DATA[3] == 0x78);
        
        /* Verify channel height (bytes 4-7, big-endian) */
        REQUIRE(notification.DATA[4] == 0xAB);
        REQUIRE(notification.DATA[5] == 0xCD);
        REQUIRE(notification.DATA[6] == 0xEF);
        REQUIRE(notification.DATA[7] == 0x01);
        
        /* Verify difficulty (bytes 8-11, big-endian) */
        REQUIRE(notification.DATA[8] == 0xDE);
        REQUIRE(notification.DATA[9] == 0xAD);
        REQUIRE(notification.DATA[10] == 0xBE);
        REQUIRE(notification.DATA[11] == 0xEF);
    }
    
    SECTION("Payload consistency across lanes")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 0x11223344;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 0x55667788;
        
        uint32_t nDifficulty = 0x99AABBCC;
        
        /* Build notifications on both lanes */
        LLP::Packet legacyNotification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        LLP::StatelessPacket statelessNotification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            1, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        /* Verify both have same payload size */
        REQUIRE(legacyNotification.LENGTH == 12);
        REQUIRE(statelessNotification.LENGTH == 12);
        REQUIRE(legacyNotification.DATA.size() == statelessNotification.DATA.size());
        
        /* Verify payload bytes are identical */
        for (size_t i = 0; i < 12; ++i)
        {
            REQUIRE(legacyNotification.DATA[i] == statelessNotification.DATA[i]);
        }
    }
    
    SECTION("Edge cases - zero values")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 0;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 0;
        
        uint32_t nDifficulty = 0;
        
        /* Build notification */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify payload is all zeros */
        REQUIRE(notification.LENGTH == 12);
        for (size_t i = 0; i < 12; ++i)
        {
            REQUIRE(notification.DATA[i] == 0x00);
        }
    }
    
    SECTION("Edge cases - maximum values")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 0xFFFFFFFF;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 0xFFFFFFFF;
        
        uint32_t nDifficulty = 0xFFFFFFFF;
        
        /* Build notification */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            2, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify payload is all 0xFF */
        REQUIRE(notification.LENGTH == 12);
        for (size_t i = 0; i < 12; ++i)
        {
            REQUIRE(notification.DATA[i] == 0xFF);
        }
    }
}

TEST_CASE("PushNotificationBuilder - Template Specialization", "[push_notification][llp]")
{
    SECTION("Packet type (legacy 8-bit)")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 100;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 33;
        
        uint32_t nDifficulty = 0x1000;
        
        /* Build using Packet template */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify it's a valid Packet type */
        REQUIRE(notification.HEADER < 256);  // 8-bit range
        REQUIRE(notification.LENGTH == 12);
        REQUIRE(notification.DATA.size() == 12);
    }
    
    SECTION("StatelessPacket type (16-bit)")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 200;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 66;
        
        uint32_t nDifficulty = 0x2000;
        
        /* Build using StatelessPacket template */
        LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            2, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        /* Verify it's a valid StatelessPacket type */
        REQUIRE(notification.HEADER >= 0xD000);  // 16-bit stateless range
        REQUIRE(notification.HEADER <= 0xD0FF);
        REQUIRE(notification.LENGTH == 12);
        REQUIRE(notification.DATA.size() == 12);
    }
}

TEST_CASE("PushNotificationBuilder - Real-World Scenarios", "[push_notification][llp]")
{
    SECTION("Typical mainnet heights")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 6535193;  // Realistic mainnet height
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 2165442;  // Realistic Prime channel height
        
        uint32_t nDifficulty = 0x03C00000;  // Typical Prime difficulty
        
        /* Build notification */
        LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify basic structure */
        REQUIRE(notification.HEADER == 0xD9);  // Prime
        REQUIRE(notification.LENGTH == 12);
        
        /* Verify height encoding */
        uint32_t decodedHeight = 
            (static_cast<uint32_t>(notification.DATA[0]) << 24) |
            (static_cast<uint32_t>(notification.DATA[1]) << 16) |
            (static_cast<uint32_t>(notification.DATA[2]) << 8) |
            static_cast<uint32_t>(notification.DATA[3]);
        
        REQUIRE(decodedHeight == 6535193);
    }
    
    SECTION("Channel switch scenario - Prime to Hash")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 1000;
        
        TAO::Ledger::BlockState statePrime;
        statePrime.nChannelHeight = 333;
        
        TAO::Ledger::BlockState stateHash;
        stateHash.nChannelHeight = 667;
        
        uint32_t nDifficulty = 0x1000;
        
        /* Build Prime notification */
        LLP::Packet primeNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, statePrime, nDifficulty);
        
        /* Build Hash notification */
        LLP::Packet hashNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            2, LLP::ProtocolLane::LEGACY, stateBest, stateHash, nDifficulty);
        
        /* Verify different opcodes */
        REQUIRE(primeNotif.HEADER == 0xD9);
        REQUIRE(hashNotif.HEADER == 0xDA);
        
        /* Verify same unified height */
        REQUIRE(primeNotif.DATA[0] == hashNotif.DATA[0]);
        REQUIRE(primeNotif.DATA[1] == hashNotif.DATA[1]);
        REQUIRE(primeNotif.DATA[2] == hashNotif.DATA[2]);
        REQUIRE(primeNotif.DATA[3] == hashNotif.DATA[3]);
        
        /* Verify different channel heights */
        REQUIRE(primeNotif.DATA[4] != hashNotif.DATA[4]);
    }
}

TEST_CASE("PushNotificationBuilder - Protocol Compliance", "[push_notification][llp]")
{
    SECTION("8-bit opcodes stay within legacy range")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 100;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 50;
        
        uint32_t nDifficulty = 0x1000;
        
        /* Test both channels */
        LLP::Packet primeNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        LLP::Packet hashNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            2, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Verify opcodes are in valid 8-bit range */
        REQUIRE(primeNotif.HEADER >= 0x00);
        REQUIRE(primeNotif.HEADER <= 0xFF);
        REQUIRE(hashNotif.HEADER >= 0x00);
        REQUIRE(hashNotif.HEADER <= 0xFF);
    }
    
    SECTION("16-bit opcodes follow mirror-map pattern")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 200;
        
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 100;
        
        uint32_t nDifficulty = 0x2000;
        
        /* Test both channels */
        LLP::StatelessPacket primeNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            1, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        LLP::StatelessPacket hashNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            2, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
        /* Verify opcodes follow 0xD000 | legacyOpcode pattern */
        REQUIRE((primeNotif.HEADER & 0xFF00) == 0xD000);
        REQUIRE((hashNotif.HEADER & 0xFF00) == 0xD000);
        
        /* Verify low byte matches legacy opcodes */
        REQUIRE((primeNotif.HEADER & 0xFF) == 0xD9);
        REQUIRE((hashNotif.HEADER & 0xFF) == 0xDA);
    }
}
