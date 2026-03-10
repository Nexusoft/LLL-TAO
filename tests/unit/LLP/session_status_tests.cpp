/* Tests for SESSION_STATUS (219) and SESSION_STATUS_ACK (220) wire format */
#include <unit/catch2/catch.hpp>
#include <LLP/include/session_status.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/opcode_utility.h>

using namespace LLP;

TEST_CASE("SESSION_STATUS opcode constants are mirror-mapped", "[llp][session_status]")
{
    REQUIRE(OpcodeUtility::Opcodes::SESSION_STATUS     == 219);
    REQUIRE(OpcodeUtility::Opcodes::SESSION_STATUS_ACK == 220);
    REQUIRE(OpcodeUtility::Stateless::SESSION_STATUS     == 0xD0DB);
    REQUIRE(OpcodeUtility::Stateless::SESSION_STATUS_ACK == 0xD0DC);
    REQUIRE(OpcodeUtility::Stateless::Mirror(219) == 0xD0DB);
    REQUIRE(OpcodeUtility::Stateless::Mirror(220) == 0xD0DC);
}

TEST_CASE("SessionStatusRequest serializes and parses correctly", "[llp][session_status]")
{
    SessionStatus::SessionStatusRequest req;
    req.session_id   = 0xDEADBEEF;
    req.status_flags = SessionStatus::MINER_HAS_TEMPLATE | SessionStatus::MINER_WORKERS_ACTIVE;

    auto v = req.Serialize();
    REQUIRE(v.size() == 8);

    /* session_id little-endian at [0-3] */
    REQUIRE(v[0] == 0xEF);
    REQUIRE(v[1] == 0xBE);
    REQUIRE(v[2] == 0xAD);
    REQUIRE(v[3] == 0xDE);

    /* status_flags big-endian at [4-7] */
    REQUIRE(v[4] == static_cast<uint8_t>((req.status_flags >> 24) & 0xFF));
    REQUIRE(v[5] == static_cast<uint8_t>((req.status_flags >> 16) & 0xFF));
    REQUIRE(v[6] == static_cast<uint8_t>((req.status_flags >>  8) & 0xFF));
    REQUIRE(v[7] == static_cast<uint8_t>( req.status_flags        & 0xFF));

    /* Round-trip parse */
    SessionStatus::SessionStatusRequest req2;
    REQUIRE(req2.Parse(v));
    REQUIRE(req2.session_id   == req.session_id);
    REQUIRE(req2.status_flags == req.status_flags);
}

TEST_CASE("SessionStatusAck serializes and parses correctly", "[llp][session_status]")
{
    uint32_t lane_health = SessionStatus::LANE_PRIMARY_ALIVE | SessionStatus::LANE_AUTHENTICATED;
    uint32_t uptime = 12345u;
    uint32_t echo   = SessionStatus::MINER_HAS_TEMPLATE;

    auto v = SessionStatus::BuildAckPayload(0xCAFEBABE, lane_health, uptime, echo);
    REQUIRE(v.size() == 16);

    /* session_id little-endian [0-3] */
    REQUIRE(v[0] == 0xBE);
    REQUIRE(v[1] == 0xBA);
    REQUIRE(v[2] == 0xFE);
    REQUIRE(v[3] == 0xCA);

    /* lane_health big-endian [4-7] */
    REQUIRE(v[4]  == static_cast<uint8_t>((lane_health >> 24) & 0xFF));

    /* uptime big-endian [8-11] */
    REQUIRE(v[8]  == static_cast<uint8_t>((uptime >> 24) & 0xFF));
    REQUIRE(v[9]  == static_cast<uint8_t>((uptime >> 16) & 0xFF));
    REQUIRE(v[10] == static_cast<uint8_t>((uptime >>  8) & 0xFF));
    REQUIRE(v[11] == static_cast<uint8_t>( uptime        & 0xFF));

    /* Round-trip parse */
    SessionStatus::SessionStatusAck ack;
    REQUIRE(ack.Parse(v));
    REQUIRE(ack.session_id        == 0xCAFEBABE);
    REQUIRE(ack.lane_health_flags == lane_health);
    REQUIRE(ack.uptime_seconds    == uptime);
    REQUIRE(ack.status_echo_flags == echo);
}

TEST_CASE("SessionStatusAck carries authoritative node session_id separately from echoed miner flags", "[llp][session_status]")
{
    constexpr uint32_t nNodeSessionId = 0x12345678u;
    constexpr uint32_t nMinerFlags = SessionStatus::MINER_DEGRADED | SessionStatus::MINER_HAS_TEMPLATE;

    auto v = SessionStatus::BuildAckPayload(nNodeSessionId, 0u, 0u, nMinerFlags);
    REQUIRE(v.size() == SessionStatus::ACK_PAYLOAD_SIZE);

    SessionStatus::SessionStatusAck ack;
    REQUIRE(ack.Parse(v));
    REQUIRE(ack.session_id == nNodeSessionId);
    REQUIRE(ack.status_echo_flags == nMinerFlags);
}

TEST_CASE("SessionStatusAck can carry uptime derived from MiningContext", "[llp][session_status]")
{
    constexpr uint64_t nSessionStart = 1'000u;
    constexpr uint64_t nNow = 1'123u;

    const MiningContext context = MiningContext()
        .WithSession(0x10203040u)
        .WithSessionStart(nSessionStart);

    const uint32_t nUptime = static_cast<uint32_t>(context.GetSessionDuration(nNow));
    auto v = SessionStatus::BuildAckPayload(context.nSessionId,
                                            SessionStatus::LANE_PRIMARY_ALIVE | SessionStatus::LANE_AUTHENTICATED,
                                            nUptime,
                                            SessionStatus::MINER_WORKERS_ACTIVE);

    SessionStatus::SessionStatusAck ack;
    REQUIRE(ack.Parse(v));
    REQUIRE(ack.uptime_seconds == 123u);
}

TEST_CASE("SessionStatusRequest parse rejects short buffers", "[llp][session_status]")
{
    SessionStatus::SessionStatusRequest req;
    std::vector<uint8_t> short_buf(7, 0x00);
    REQUIRE_FALSE(req.Parse(short_buf));
}

TEST_CASE("SessionStatusAck parse rejects short buffers", "[llp][session_status]")
{
    SessionStatus::SessionStatusAck ack;
    std::vector<uint8_t> short_buf(15, 0x00);
    REQUIRE_FALSE(ack.Parse(short_buf));
}

TEST_CASE("Lane health flags combine correctly", "[llp][session_status]")
{
    uint32_t flags = SessionStatus::LANE_PRIMARY_ALIVE
                   | SessionStatus::LANE_SECONDARY_ALIVE
                   | SessionStatus::LANE_SIM_LINK_ACTIVE
                   | SessionStatus::LANE_AUTHENTICATED;
    REQUIRE(flags == 0x0F);
}

TEST_CASE("Miner status flags combine correctly", "[llp][session_status]")
{
    uint32_t flags = SessionStatus::MINER_DEGRADED
                   | SessionStatus::MINER_HAS_TEMPLATE
                   | SessionStatus::MINER_WORKERS_ACTIVE
                   | SessionStatus::MINER_SECONDARY_UP;
    REQUIRE(flags == 0x0F);
}
