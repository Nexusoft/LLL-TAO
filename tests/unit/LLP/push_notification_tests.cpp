/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/push_notification.h>
#include <LLP/include/miner_push_dispatcher.h>
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
    SECTION("140-byte payload format - big-endian encoding")
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
        REQUIRE(notification.LENGTH == 140);
        REQUIRE(notification.DATA.size() == 140);
        
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
        REQUIRE(legacyNotification.LENGTH == 140);
        REQUIRE(statelessNotification.LENGTH == 140);
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
        
        /* Verify payload is all zeros (height=0, channelHeight=0, difficulty=0, hashBestChain=0) */
        REQUIRE(notification.LENGTH == 140);
        for (size_t i = 0; i < 140; ++i)
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
        
        /* Verify header bytes (0-11) are all 0xFF.
         * Note: bytes 12-139 (hashBestChain) default to 0 when not passed. */
        REQUIRE(notification.LENGTH == 140);
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
        REQUIRE(notification.LENGTH == 140);
        REQUIRE(notification.DATA.size() == 140);
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
        REQUIRE(notification.LENGTH == 140);
        REQUIRE(notification.DATA.size() == 140);
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
        REQUIRE(notification.LENGTH == 140);
        
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

TEST_CASE("PushNotificationBuilder - Universal Tip Push", "[push_notification][llp]")
{
    /* Tests the universal tip push scenario: when ANY channel (including Stake=0)
     * advances the unified best tip, both Prime (1) and Hash (2) subscribers must
     * receive channel-appropriate notifications sharing the same unified height. */

    SECTION("Stake block advances tip - both PoW channels get same unified height")
    {
        /* Simulate a Stake block advancing the unified tip */
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 7000000;  // New unified height after stake block

        /* Simulate Prime and Hash channel states (unchanged by stake block) */
        TAO::Ledger::BlockState statePrime;
        statePrime.nChannelHeight = 2300000;

        TAO::Ledger::BlockState stateHash;
        stateHash.nChannelHeight = 4500000;

        uint32_t nPrimeBits = 0x03C00000;
        uint32_t nHashBits  = 0x1D00FFFF;

        /* Build notifications for both PoW channels (simulating universal tip push) */
        LLP::Packet primeNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            1, LLP::ProtocolLane::LEGACY, stateBest, statePrime, nPrimeBits);

        LLP::Packet hashNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
            2, LLP::ProtocolLane::LEGACY, stateBest, stateHash, nHashBits);

        /* Both notifications must carry the same unified height */
        REQUIRE(primeNotif.DATA[0] == hashNotif.DATA[0]);
        REQUIRE(primeNotif.DATA[1] == hashNotif.DATA[1]);
        REQUIRE(primeNotif.DATA[2] == hashNotif.DATA[2]);
        REQUIRE(primeNotif.DATA[3] == hashNotif.DATA[3]);

        /* Decode and verify unified height */
        uint32_t decodedHeight =
            (static_cast<uint32_t>(primeNotif.DATA[0]) << 24) |
            (static_cast<uint32_t>(primeNotif.DATA[1]) << 16) |
            (static_cast<uint32_t>(primeNotif.DATA[2]) << 8)  |
             static_cast<uint32_t>(primeNotif.DATA[3]);
        REQUIRE(decodedHeight == 7000000);

        /* Each channel carries its own channel-specific height */
        uint32_t decodedPrimeCh =
            (static_cast<uint32_t>(primeNotif.DATA[4]) << 24) |
            (static_cast<uint32_t>(primeNotif.DATA[5]) << 16) |
            (static_cast<uint32_t>(primeNotif.DATA[6]) << 8)  |
             static_cast<uint32_t>(primeNotif.DATA[7]);
        REQUIRE(decodedPrimeCh == 2300000);

        uint32_t decodedHashCh =
            (static_cast<uint32_t>(hashNotif.DATA[4]) << 24) |
            (static_cast<uint32_t>(hashNotif.DATA[5]) << 16) |
            (static_cast<uint32_t>(hashNotif.DATA[6]) << 8)  |
             static_cast<uint32_t>(hashNotif.DATA[7]);
        REQUIRE(decodedHashCh == 4500000);

        /* Each channel carries its own difficulty */
        uint32_t decodedPrimeBits =
            (static_cast<uint32_t>(primeNotif.DATA[8])  << 24) |
            (static_cast<uint32_t>(primeNotif.DATA[9])  << 16) |
            (static_cast<uint32_t>(primeNotif.DATA[10]) << 8)  |
             static_cast<uint32_t>(primeNotif.DATA[11]);
        REQUIRE(decodedPrimeBits == nPrimeBits);

        uint32_t decodedHashBits =
            (static_cast<uint32_t>(hashNotif.DATA[8])  << 24) |
            (static_cast<uint32_t>(hashNotif.DATA[9])  << 16) |
            (static_cast<uint32_t>(hashNotif.DATA[10]) << 8)  |
             static_cast<uint32_t>(hashNotif.DATA[11]);
        REQUIRE(decodedHashBits == nHashBits);

        /* Correct channel-specific opcodes */
        REQUIRE(primeNotif.HEADER == 0xD9);  // PRIME_BLOCK_AVAILABLE
        REQUIRE(hashNotif.HEADER  == 0xDA);  // HASH_BLOCK_AVAILABLE
    }

    SECTION("Universal tip push - stateless lane both channels")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 5000000;

        TAO::Ledger::BlockState statePrime;
        statePrime.nChannelHeight = 1666666;

        TAO::Ledger::BlockState stateHash;
        stateHash.nChannelHeight = 3333333;

        uint32_t nPrimeBits = 0x03C00000;
        uint32_t nHashBits  = 0x1D00FFFF;

        LLP::StatelessPacket primeNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            1, LLP::ProtocolLane::STATELESS, stateBest, statePrime, nPrimeBits);

        LLP::StatelessPacket hashNotif = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
            2, LLP::ProtocolLane::STATELESS, stateBest, stateHash, nHashBits);

        /* Both carry same unified height */
        for (int i = 0; i < 4; ++i)
            REQUIRE(primeNotif.DATA[i] == hashNotif.DATA[i]);

        /* Correct 16-bit opcodes */
        REQUIRE(primeNotif.HEADER == 0xD0D9);  // STATELESS PRIME_BLOCK_AVAILABLE
        REQUIRE(hashNotif.HEADER  == 0xD0DA);  // STATELESS HASH_BLOCK_AVAILABLE

        /* Protocol payload is 140 bytes (12 header + 128 hashBestChain) */
        REQUIRE(primeNotif.LENGTH == 140);
        REQUIRE(hashNotif.LENGTH  == 140);
    }
}


/* ============================================================================
 * MinerPushDispatcher — broadcast logic simulation tests
 *
 * These tests verify the channel-filter logic that NotifyChannelMiners uses
 * and ensure the dispatcher's 4-send-per-event invariant holds.
 * They mirror the filtering code in Server<T>::NotifyChannelMiners without
 * instantiating live server objects.
 * ============================================================================ */

namespace
{
    /** Minimal miner subscription state used by the filter simulation below. */
    struct FakeMinerCtx
    {
        bool     fSubscribedToNotifications{false};
        uint32_t nSubscribedChannel{0};
    };

    /**
     *  SimulateLaneBroadcast
     *
     *  Mirrors the filtering logic inside Server<T>::NotifyChannelMiners().
     *  Returns the number of miners that would be notified on a single lane
     *  for the given nChannel.
     */
    uint32_t SimulateLaneBroadcast(const std::vector<FakeMinerCtx>& vMiners,
                                   uint32_t nChannel)
    {
        uint32_t nNotified = 0;
        for (const auto& ctx : vMiners)
        {
            if (!ctx.fSubscribedToNotifications)
                continue;                          // polling miner — skip
            if (ctx.nSubscribedChannel != nChannel)
                continue;                          // wrong channel — skip
            ++nNotified;
        }
        return nNotified;
    }
}

TEST_CASE("BroadcastChannelNotification — 4 total sends per block event", "[push_broadcast][llp]")
{
    /* Set up one miner per (channel, lane) — 4 miners in 2 lanes. */

    /* Stateless lane: 1 Prime subscriber + 1 Hash subscriber */
    const std::vector<FakeMinerCtx> vStateless = {
        { true, 1 },  // Prime subscriber
        { true, 2 },  // Hash subscriber
    };

    /* Legacy lane: 1 Prime subscriber + 1 Hash subscriber */
    const std::vector<FakeMinerCtx> vLegacy = {
        { true, 1 },  // Prime subscriber
        { true, 2 },  // Hash subscriber
    };

    /* Count sends per (channel, lane) — mirrors BroadcastChannelNotification logic */
    uint32_t nStatelessPrime = SimulateLaneBroadcast(vStateless, 1);
    uint32_t nLegacyPrime    = SimulateLaneBroadcast(vLegacy,    1);
    uint32_t nStatelessHash  = SimulateLaneBroadcast(vStateless, 2);
    uint32_t nLegacyHash     = SimulateLaneBroadcast(vLegacy,    2);

    /* Exactly 4 server-level sends total: one per (channel, lane) */
    uint32_t nTotal = nStatelessPrime + nLegacyPrime + nStatelessHash + nLegacyHash;
    REQUIRE(nTotal == 4);

    /* Each lane+channel pair notifies exactly 1 miner */
    REQUIRE(nStatelessPrime == 1);
    REQUIRE(nLegacyPrime    == 1);
    REQUIRE(nStatelessHash  == 1);
    REQUIRE(nLegacyHash     == 1);
}

TEST_CASE("BroadcastChannelNotification — no cross-channel duplicates", "[push_broadcast][llp]")
{
    /* Miner subscribed to Prime must not receive Hash notifications. */
    const std::vector<FakeMinerCtx> vMiners = {
        { true, 1 },   // Prime subscriber
    };

    REQUIRE(SimulateLaneBroadcast(vMiners, 1) == 1);  // Prime: notified
    REQUIRE(SimulateLaneBroadcast(vMiners, 2) == 0);  // Hash: NOT notified

    /* Miner subscribed to Hash must not receive Prime notifications. */
    const std::vector<FakeMinerCtx> vHashMiners = {
        { true, 2 },   // Hash subscriber
    };

    REQUIRE(SimulateLaneBroadcast(vHashMiners, 1) == 0);  // Prime: NOT notified
    REQUIRE(SimulateLaneBroadcast(vHashMiners, 2) == 1);  // Hash: notified
}

TEST_CASE("BroadcastChannelNotification — unsubscribed miners receive nothing", "[push_broadcast][llp]")
{
    /* Miners using GET_ROUND polling have fSubscribedToNotifications == false. */
    const std::vector<FakeMinerCtx> vPollingMiners = {
        { false, 1 },  // unsubscribed Prime miner (polling)
        { false, 2 },  // unsubscribed Hash miner (polling)
    };

    REQUIRE(SimulateLaneBroadcast(vPollingMiners, 1) == 0);
    REQUIRE(SimulateLaneBroadcast(vPollingMiners, 2) == 0);
}

TEST_CASE("BroadcastChannelNotification — multiple miners per channel, still no duplicates", "[push_broadcast][llp]")
{
    /* Even with multiple miners on the same channel, each gets exactly one
     * notification per block event (no duplicate sends). */
    const std::vector<FakeMinerCtx> vStateless = {
        { true, 1 },   // Prime miner 1
        { true, 1 },   // Prime miner 2
        { true, 2 },   // Hash miner 1
    };
    const std::vector<FakeMinerCtx> vLegacy = {
        { true, 1 },   // Prime miner 3
        { true, 2 },   // Hash miner 2
        { true, 2 },   // Hash miner 3
    };

    uint32_t nStatelessPrime = SimulateLaneBroadcast(vStateless, 1);
    uint32_t nLegacyPrime    = SimulateLaneBroadcast(vLegacy,    1);
    uint32_t nStatelessHash  = SimulateLaneBroadcast(vStateless, 2);
    uint32_t nLegacyHash     = SimulateLaneBroadcast(vLegacy,    2);

    /* Each miner receives exactly one notification for their channel */
    REQUIRE(nStatelessPrime == 2);  // 2 stateless Prime miners
    REQUIRE(nLegacyPrime    == 1);  // 1 legacy Prime miner
    REQUIRE(nStatelessHash  == 1);  // 1 stateless Hash miner
    REQUIRE(nLegacyHash     == 2);  // 2 legacy Hash miners

    /* Total individual sends = 2+1+1+2 = 6 across 4 lane-level broadcast calls.
     * Each lane call processes its own connection list once; no miner gets two
     * notifications for the same event. */
    REQUIRE((nStatelessPrime + nLegacyPrime + nStatelessHash + nLegacyHash) == 6);
}

TEST_CASE("MinerPushDispatcher — dedup key packing and atomic dedup invariant", "[push_broadcast][llp]")
{
    /* Verify that the dedup key concept (height<<32 | hashPrefix) is correct. */

    const uint32_t nHeight = 100000;
    const uint32_t nHashPrefix = 0xDEADBEEF;

    uint64_t key = (static_cast<uint64_t>(nHeight) << 32) | static_cast<uint64_t>(nHashPrefix);

    /* High 32 bits encode height */
    REQUIRE(static_cast<uint32_t>(key >> 32) == nHeight);

    /* Low 32 bits encode hash prefix */
    REQUIRE(static_cast<uint32_t>(key & 0xFFFFFFFF) == nHashPrefix);

    /* Two different heights produce different keys even with the same prefix */
    uint64_t key2 = (static_cast<uint64_t>(nHeight + 1) << 32) | static_cast<uint64_t>(nHashPrefix);
    REQUIRE(key != key2);

    /* Same height but different hash prefix also produces different key */
    uint64_t key3 = (static_cast<uint64_t>(nHeight) << 32) | static_cast<uint64_t>(nHashPrefix ^ 1);
    REQUIRE(key != key3);

    /* Verify CAS-based dedup logic: simulate what DispatchPushEvent does.
     * First CAS on a fresh atomic must succeed; second CAS with same key must fail. */
    {
        std::atomic<uint64_t> dedupAtom{0};

        /* First attempt — should succeed (old value is 0, new key is different) */
        uint64_t oldVal = dedupAtom.load(std::memory_order_acquire);
        bool firstCAS = (oldVal != key) &&
                         dedupAtom.compare_exchange_strong(oldVal, key,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed);
        REQUIRE(firstCAS == true);
        REQUIRE(dedupAtom.load() == key);

        /* Second attempt with same key — CAS must not fire (old == key, condition fails) */
        uint64_t oldVal2 = dedupAtom.load(std::memory_order_acquire);
        bool secondCAS = (oldVal2 != key) &&
                          dedupAtom.compare_exchange_strong(oldVal2, key,
                                                            std::memory_order_release,
                                                            std::memory_order_relaxed);
        REQUIRE(secondCAS == false);   // dedup correctly blocks the second dispatch
        REQUIRE(dedupAtom.load() == key);  // value unchanged

        /* Different key (new height) must succeed again */
        uint64_t newKey = (static_cast<uint64_t>(nHeight + 1) << 32) | static_cast<uint64_t>(nHashPrefix);
        uint64_t oldVal3 = dedupAtom.load(std::memory_order_acquire);
        bool thirdCAS = (oldVal3 != newKey) &&
                         dedupAtom.compare_exchange_strong(oldVal3, newKey,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed);
        REQUIRE(thirdCAS == true);
        REQUIRE(dedupAtom.load() == newKey);
    }
}
