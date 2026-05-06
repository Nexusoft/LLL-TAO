#include <unit/catch2/catch.hpp>

#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_transport.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/packet_framing.h>
#include <LLP/include/session_start_packet.h>
#include <LLP/include/session_status.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace
{
    void RequireLanePairMatches(const uint8_t nLegacyOpcode, const std::vector<uint8_t>& vPayload)
    {
        const auto vLegacyBytes = LLP::MiningTransport::BuildResponseBytes(
            LLP::MiningTransportLane::LEGACY_8323, nLegacyOpcode, vPayload);
        const auto vStatelessBytes = LLP::MiningTransport::BuildResponseBytes(
            LLP::MiningTransportLane::STATELESS_9323, nLegacyOpcode, vPayload);

        REQUIRE(vLegacyBytes.size() == vPayload.size() + 5u);
        REQUIRE(vStatelessBytes.size() == vPayload.size() + 6u);

        REQUIRE(vLegacyBytes[0] == nLegacyOpcode);
        REQUIRE(vStatelessBytes[0] == 0xD0);
        REQUIRE(vStatelessBytes[1] == nLegacyOpcode);

        REQUIRE(std::equal(vLegacyBytes.begin() + 1, vLegacyBytes.begin() + 5,
                           vStatelessBytes.begin() + 2));
        REQUIRE(LLP::PacketFraming::DecodeLengthComponents(
            vLegacyBytes[1], vLegacyBytes[2], vLegacyBytes[3], vLegacyBytes[4]) == vPayload.size());
        REQUIRE(LLP::PacketFraming::DecodeLengthComponents(
            vStatelessBytes[2], vStatelessBytes[3], vStatelessBytes[4], vStatelessBytes[5]) == vPayload.size());

        REQUIRE(std::equal(vPayload.begin(), vPayload.end(), vLegacyBytes.begin() + 5));
        REQUIRE(std::equal(vPayload.begin(), vPayload.end(), vStatelessBytes.begin() + 6));
    }
}

TEST_CASE("LLP strict lane opcode mapping", "[llp][lane_enforcement]")
{
    namespace Opcodes = LLP::OpcodeUtility::Opcodes;
    namespace Stateless = LLP::OpcodeUtility::Stateless;

    SECTION("Auth opcodes are mirror-mapped on stateless lane")
    {
        REQUIRE(Stateless::CHANNEL_ACK == 0xD0CE);
        REQUIRE(Stateless::AUTH_INIT == 0xD0CF);
        REQUIRE(Stateless::AUTH_CHALLENGE == 0xD0D0);
        REQUIRE(Stateless::AUTH_RESPONSE == 0xD0D1);
        REQUIRE(Stateless::AUTH_RESULT == 0xD0D2);
    }

    SECTION("Legacy lane auth opcodes remain 8-bit")
    {
        REQUIRE(Opcodes::CHANNEL_ACK == 206);
        REQUIRE(Opcodes::MINER_AUTH_INIT == 207);
        REQUIRE(Opcodes::MINER_AUTH_CHALLENGE == 208);
        REQUIRE(Opcodes::MINER_AUTH_RESPONSE == 209);
        REQUIRE(Opcodes::MINER_AUTH_RESULT == 210);
    }

    SECTION("Template delivery opcodes remain lane-specific")
    {
        REQUIRE(Opcodes::BLOCK_DATA == 0x00);
        REQUIRE(Stateless::GET_BLOCK == 0xD081);
    }

    SECTION("Legacy auth opcodes are invalid on stateless 16-bit lane")
    {
        REQUIRE(Stateless::IsStateless(Opcodes::CHANNEL_ACK) == false);
        REQUIRE(Stateless::IsStateless(Opcodes::MINER_AUTH_CHALLENGE) == false);
        REQUIRE(Stateless::IsStateless(Stateless::AUTH_CHALLENGE) == true);
    }

    SECTION("SESSION_STATUS opcodes are mirror-mapped on stateless lane")
    {
        REQUIRE(Stateless::SESSION_STATUS     == 0xD0DB);
        REQUIRE(Stateless::SESSION_STATUS_ACK == 0xD0DC);
        REQUIRE(Stateless::STATELESS_SESSION_STATUS     == 0xD0DB);
        REQUIRE(Stateless::STATELESS_SESSION_STATUS_ACK == 0xD0DC);
    }

    SECTION("Legacy zero-length packets still carry the 4-byte length field")
    {
        LLP::Packet ready(Opcodes::MINER_READY);
        ready.LENGTH = 0;

        const auto bytes = ready.GetBytes();
        REQUIRE(bytes.size() == 5u);
        REQUIRE(bytes[0] == Opcodes::MINER_READY);
        REQUIRE(bytes[1] == 0x00);
        REQUIRE(bytes[2] == 0x00);
        REQUIRE(bytes[3] == 0x00);
        REQUIRE(bytes[4] == 0x00);

        LLP::Packet parsed(Opcodes::MINER_READY);
        parsed.SetLength({0x00, 0x00, 0x00, 0x00});
        REQUIRE(parsed.Header());
        REQUIRE(parsed.Complete());
    }

    SECTION("Legacy and stateless transports differ only by framing width for mirrored semantics")
    {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};

        LLP::Packet legacy(Opcodes::SESSION_START);
        legacy.DATA = payload;
        legacy.LENGTH = static_cast<uint32_t>(payload.size());

        LLP::StatelessPacket stateless(Stateless::SESSION_START);
        stateless.DATA = payload;
        stateless.LENGTH = static_cast<uint32_t>(payload.size());

        const auto legacyBytes = legacy.GetBytes();
        const auto statelessBytes = stateless.GetBytes();

        REQUIRE(legacyBytes.size() == payload.size() + 5u);
        REQUIRE(statelessBytes.size() == payload.size() + 6u);
        REQUIRE(legacyBytes[0] == Opcodes::SESSION_START);
        REQUIRE(statelessBytes[0] == 0xD0);
        REQUIRE(statelessBytes[1] == Opcodes::SESSION_START);
        REQUIRE(std::equal(payload.begin(), payload.end(), legacyBytes.begin() + 5));
        REQUIRE(std::equal(payload.begin(), payload.end(), statelessBytes.begin() + 6));
    }

    SECTION("Shared framing helpers preserve one wire-format difference")
    {
        const std::vector<uint8_t> payload = {0xAA, 0xBB};
        const auto legacyBytes =
            LLP::PacketFraming::BuildLegacyBytes(Opcodes::BLOCK_REJECTED, payload.size(), payload);
        const auto statelessBytes =
            LLP::PacketFraming::BuildStatelessBytes(Stateless::BLOCK_REJECTED, payload.size(), payload);

        REQUIRE(LLP::PacketFraming::LEGACY_HEADER_BYTES == 1u);
        REQUIRE(LLP::PacketFraming::STATELESS_HEADER_BYTES == 2u);
        REQUIRE(LLP::PacketFraming::LENGTH_BYTES == 4u);
        REQUIRE(legacyBytes.size() == statelessBytes.size() - 1u);
        REQUIRE(legacyBytes[0] == Opcodes::BLOCK_REJECTED);
        REQUIRE(statelessBytes[0] == 0xD0);
        REQUIRE(statelessBytes[1] == Opcodes::BLOCK_REJECTED);
        REQUIRE(std::equal(legacyBytes.begin() + 1, legacyBytes.end(), statelessBytes.begin() + 2));

        const std::array<uint8_t, LLP::PacketFraming::LENGTH_BYTES> lengthBytes =
            {legacyBytes[1], legacyBytes[2], legacyBytes[3], legacyBytes[4]};
        REQUIRE(LLP::PacketFraming::DecodeLength(lengthBytes) == payload.size());
    }

    SECTION("Legacy transport encoder owns 8-bit framing for shared semantic opcodes")
    {
        const std::vector<uint8_t> payload = {0x11};
        /* These opcodes are shared business semantics returned by auth/session,
         * template, and rejection handlers; the legacy transport must still
         * encode each one as an 8-bit Packet frame. */
        const std::array<uint8_t, 5> opcodes = {
            Opcodes::SESSION_START,
            Opcodes::MINER_AUTH_RESULT,
            Opcodes::CHANNEL_ACK,
            Opcodes::BLOCK_DATA,
            Opcodes::BLOCK_REJECTED
        };

        for(const uint8_t opcode : opcodes)
        {
            const auto bytes = LLP::MiningTransport::BuildResponseBytes(
                LLP::MiningTransportLane::LEGACY_8323, opcode, payload);

            REQUIRE(bytes.size() == payload.size() + 5u);
            REQUIRE(bytes[0] == opcode);
            REQUIRE(bytes[1] == 0x00);
            REQUIRE(bytes[2] == 0x00);
            REQUIRE(bytes[3] == 0x00);
            REQUIRE(bytes[4] == static_cast<uint8_t>(payload.size()));
            REQUIRE(std::equal(payload.begin(), payload.end(), bytes.begin() + 5));
        }
    }

    SECTION("Stateless transport encoder mirrors the same semantics into 16-bit framing")
    {
        const std::vector<uint8_t> payload = {0x22};
        const auto bytes = LLP::MiningTransport::BuildResponseBytes(
            LLP::MiningTransportLane::STATELESS_9323,
            Opcodes::BLOCK_REJECTED,
            payload);

        REQUIRE(bytes.size() == payload.size() + 6u);
        REQUIRE(bytes[0] == 0xD0);
        REQUIRE(bytes[1] == Opcodes::BLOCK_REJECTED);
        REQUIRE(bytes[2] == 0x00);
        REQUIRE(bytes[3] == 0x00);
        REQUIRE(bytes[4] == 0x00);
        REQUIRE(bytes[5] == static_cast<uint8_t>(payload.size()));
        REQUIRE(bytes[6] == payload[0]);
    }

    SECTION("Stateless transport encoder covers auth/session semantic opcodes")
    {
        const std::array<uint8_t, 3> opcodes = {
            Opcodes::MINER_READY,
            Opcodes::SESSION_KEEPALIVE,
            Opcodes::MINER_AUTH_RESULT
        };

        for(const uint8_t opcode : opcodes)
        {
            const auto bytes = LLP::MiningTransport::BuildResponseBytes(
                LLP::MiningTransportLane::STATELESS_9323, opcode);

            REQUIRE(bytes.size() == 6u);
            REQUIRE(bytes[0] == 0xD0);
            REQUIRE(bytes[1] == opcode);
            REQUIRE(bytes[2] == 0x00);
            REQUIRE(bytes[3] == 0x00);
            REQUIRE(bytes[4] == 0x00);
            REQUIRE(bytes[5] == 0x00);
        }
    }

    SECTION("Stateless transport encoder preserves payloads for auth/session responses")
    {
        const std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
        const std::array<uint8_t, 2> opcodes = {
            Opcodes::SESSION_KEEPALIVE,
            Opcodes::MINER_AUTH_RESULT
        };

        for(const uint8_t opcode : opcodes)
        {
            const auto bytes = LLP::MiningTransport::BuildResponseBytes(
                LLP::MiningTransportLane::STATELESS_9323, opcode, payload);

            REQUIRE(bytes.size() == payload.size() + 6u);
            REQUIRE(bytes[0] == 0xD0);
            REQUIRE(bytes[1] == opcode);
            REQUIRE(bytes[2] == 0x00);
            REQUIRE(bytes[3] == 0x00);
            REQUIRE(bytes[4] == 0x00);
            REQUIRE(bytes[5] == static_cast<uint8_t>(payload.size()));
            REQUIRE(std::equal(payload.begin(), payload.end(), bytes.begin() + 6));
        }
    }
}


TEST_CASE("Payload parity across legacy 8323 and stateless 9323 lanes", "[llp][lane_enforcement][legacy]")
{
    namespace Opcodes = LLP::OpcodeUtility::Opcodes;

    SECTION("Mirrored response opcodes preserve the same payload on both lanes")
    {
        const std::vector<uint8_t> payload = {0x00, 0x01, 0x7F, 0x80, 0xFF};
        const std::array opcodes = {
            Opcodes::BLOCK_DATA,
            Opcodes::BLOCK_ACCEPTED,
            Opcodes::BLOCK_REJECTED,
            Opcodes::ORPHAN_BLOCK,
            Opcodes::NEW_ROUND,
            Opcodes::OLD_ROUND,
            Opcodes::CHANNEL_ACK,
            Opcodes::MINER_AUTH_CHALLENGE,
            Opcodes::MINER_AUTH_RESULT,
            Opcodes::SESSION_START,
            Opcodes::SESSION_KEEPALIVE,
            Opcodes::MINER_REWARD_RESULT,
            Opcodes::MINER_READY,
            Opcodes::PRIME_BLOCK_AVAILABLE,
            Opcodes::HASH_BLOCK_AVAILABLE,
            Opcodes::SESSION_STATUS_ACK
        };

        for(const uint8_t opcode : opcodes)
            RequireLanePairMatches(opcode, payload);
    }

    SECTION("Header-only control responses differ only by opcode width")
    {
        /* These opcodes are included in the payload-preservation table above
         * and repeated here with the zero-length packets they use on the wire. */
        const std::vector<uint8_t> empty;
        const std::array opcodes = {
            Opcodes::CHANNEL_ACK,
            Opcodes::MINER_READY,
            Opcodes::PRIME_BLOCK_AVAILABLE,
            Opcodes::HASH_BLOCK_AVAILABLE,
            Opcodes::SESSION_KEEPALIVE,
            Opcodes::NEW_ROUND,
            Opcodes::OLD_ROUND
        };

        for(const uint8_t opcode : opcodes)
            RequireLanePairMatches(opcode, empty);
    }

    SECTION("SESSION_START payload builder is lane-neutral")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        const auto payload = LLP::SessionStartPacket::BuildPayload(
            0x12345678u,
            900u,
            hashGenesis);

        REQUIRE(payload.size() == 41u);
        REQUIRE(payload[0] == 0x01);
        RequireLanePairMatches(Opcodes::SESSION_START, payload);
    }

    SECTION("SESSION_STATUS_ACK payload builder is lane-neutral")
    {
        const auto payload = LLP::SessionStatus::BuildAckPayload(
            0xAABBCCDDu,
            LLP::SessionStatus::LANE_PRIMARY_ALIVE | LLP::SessionStatus::LANE_AUTHENTICATED,
            42u,
            LLP::SessionStatus::MINER_HAS_TEMPLATE | LLP::SessionStatus::MINER_WORKERS_ACTIVE);

        REQUIRE(payload.size() == LLP::SessionStatus::ACK_PAYLOAD_SIZE);
        RequireLanePairMatches(Opcodes::SESSION_STATUS_ACK, payload);
    }

    SECTION("GET_BLOCK control rejection payload is lane-neutral")
    {
        const auto payload = LLP::BuildGetBlockControlPayload(
            LLP::GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS,
            250u);

        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[1] == static_cast<uint8_t>(LLP::GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS));
        RequireLanePairMatches(Opcodes::BLOCK_REJECTED, payload);
    }
}
