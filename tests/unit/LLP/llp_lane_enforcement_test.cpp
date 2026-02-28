#include <unit/catch2/catch.hpp>

#include <LLP/include/opcode_utility.h>

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
}
