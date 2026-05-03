#include <unit/catch2/catch.hpp>

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>

#include <algorithm>

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
}
