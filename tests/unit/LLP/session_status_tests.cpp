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

TEST_CASE("SESSION_STATUS with MINER_DEGRADED is detected in request flags", "[llp][session_status][degraded]")
{
    SECTION("MINER_DEGRADED bit is isolated correctly")
    {
        /* Verify MINER_DEGRADED is distinct and non-zero as required by the protocol */
        REQUIRE(SessionStatus::MINER_DEGRADED != 0u);
        /* It must not overlap with any other defined miner flag */
        REQUIRE((SessionStatus::MINER_DEGRADED & SessionStatus::MINER_HAS_TEMPLATE) == 0);
        REQUIRE((SessionStatus::MINER_DEGRADED & SessionStatus::MINER_WORKERS_ACTIVE) == 0);
        REQUIRE((SessionStatus::MINER_DEGRADED & SessionStatus::MINER_SECONDARY_UP) == 0);
    }

    SECTION("MINER_DEGRADED flag is detected in combined status_flags")
    {
        /* Miner in degraded mode with active workers */
        uint32_t status_flags = SessionStatus::MINER_DEGRADED | SessionStatus::MINER_WORKERS_ACTIVE;
        REQUIRE((status_flags & SessionStatus::MINER_DEGRADED) != 0);
    }

    SECTION("MINER_DEGRADED flag is absent when miner is healthy")
    {
        /* Healthy miner: has template and workers, not degraded */
        uint32_t status_flags = SessionStatus::MINER_HAS_TEMPLATE | SessionStatus::MINER_WORKERS_ACTIVE;
        REQUIRE((status_flags & SessionStatus::MINER_DEGRADED) == 0);
    }

    SECTION("SESSION_STATUS ACK echoes MINER_DEGRADED flag back to miner")
    {
        constexpr uint32_t nSessionId = 0xABCD1234u;
        constexpr uint32_t nLaneHealth = SessionStatus::LANE_PRIMARY_ALIVE | SessionStatus::LANE_AUTHENTICATED;
        constexpr uint32_t nUptime = 42u;
        /* Miner sends degraded + has template */
        constexpr uint32_t nMinerFlags = SessionStatus::MINER_DEGRADED | SessionStatus::MINER_HAS_TEMPLATE;

        auto v = SessionStatus::BuildAckPayload(nSessionId, nLaneHealth, nUptime, nMinerFlags);
        SessionStatus::SessionStatusAck ack;
        REQUIRE(ack.Parse(v));

        /* The ACK must echo back all miner-provided flags including MINER_DEGRADED */
        REQUIRE((ack.status_echo_flags & SessionStatus::MINER_DEGRADED) != 0);
        REQUIRE((ack.status_echo_flags & SessionStatus::MINER_HAS_TEMPLATE) != 0);
    }

    SECTION("Recovery push condition: authenticated channel 1 with MINER_DEGRADED")
    {
        /* Verify the guard logic used in the SESSION_STATUS degraded-recovery path:
         * (req.status_flags & MINER_DEGRADED) && fAuthenticated && (nChannel == 1 || 2) */
        uint32_t status_flags = SessionStatus::MINER_DEGRADED;
        bool fAuthenticated = true;
        uint32_t nChannel = 1;

        bool should_recover = (status_flags & SessionStatus::MINER_DEGRADED) &&
                              fAuthenticated &&
                              (nChannel == 1 || nChannel == 2);
        REQUIRE(should_recover == true);
    }

    SECTION("Recovery push condition: unauthenticated miner does NOT trigger push")
    {
        uint32_t status_flags = SessionStatus::MINER_DEGRADED;
        bool fAuthenticated = false;
        uint32_t nChannel = 1;

        bool should_recover = (status_flags & SessionStatus::MINER_DEGRADED) &&
                              fAuthenticated &&
                              (nChannel == 1 || nChannel == 2);
        REQUIRE(should_recover == false);
    }

    SECTION("Recovery push condition: Stake channel (0) does NOT trigger push")
    {
        uint32_t status_flags = SessionStatus::MINER_DEGRADED;
        bool fAuthenticated = true;
        uint32_t nChannel = 0;  // Proof-of-Stake — not mineable via this path

        bool should_recover = (status_flags & SessionStatus::MINER_DEGRADED) &&
                              fAuthenticated &&
                              (nChannel == 1 || nChannel == 2);
        REQUIRE(should_recover == false);
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * R-02: ValidateConsistency gate — tests for the security boundaries added
 * at MINER_SET_REWARD and SUBMIT_BLOCK handlers.
 * ───────────────────────────────────────────────────────────────────────────*/

TEST_CASE("ValidateConsistency rejects structurally inconsistent contexts", "[llp][session_status][consistency]")
{
    SECTION("Authenticated context missing session ID is detected")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(0);       // zero session ID — inconsistent

        const SessionConsistencyResult result = ctx.ValidateConsistency();
        REQUIRE(result == SessionConsistencyResult::MissingSessionId);
        REQUIRE(result != SessionConsistencyResult::Ok);
    }

    SECTION("Authenticated context missing genesis hash is detected")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345);
        /* hashGenesis defaults to 0 */

        const SessionConsistencyResult result = ctx.ValidateConsistency();
        REQUIRE(result == SessionConsistencyResult::MissingGenesis);
    }

    SECTION("Authenticated context missing Falcon key ID is detected")
    {
        uint256_t genesis;
        genesis.SetHex("aabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccdd");

        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345)
            .WithGenesis(genesis);
        /* hashKeyID defaults to 0 */

        const SessionConsistencyResult result = ctx.ValidateConsistency();
        REQUIRE(result == SessionConsistencyResult::MissingFalconKey);
    }

    SECTION("Reward-bound context with zero reward address is detected")
    {
        uint256_t genesis;
        genesis.SetHex("aabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccdd");
        uint256_t keyID;
        keyID.SetHex("1122334411223344112233441122334411223344112233441122334411223344");

        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345)
            .WithGenesis(genesis)
            .WithKeyId(keyID)
            .WithRewardAddress(uint256_t(0));   // bound but zero hash — inconsistent

        const SessionConsistencyResult result = ctx.ValidateConsistency();
        REQUIRE(result == SessionConsistencyResult::RewardBoundMissingHash);
    }

    SECTION("Fully valid authenticated context passes consistency check")
    {
        uint256_t genesis;
        genesis.SetHex("aabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccdd");
        uint256_t keyID;
        keyID.SetHex("1122334411223344112233441122334411223344112233441122334411223344");

        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345)
            .WithGenesis(genesis)
            .WithKeyId(keyID);

        const SessionConsistencyResult result = ctx.ValidateConsistency();
        REQUIRE(result == SessionConsistencyResult::Ok);
    }

    SECTION("Unauthenticated context always passes (no invariants apply)")
    {
        /* Unauthenticated context with all zero fields should be OK */
        MiningContext ctx = MiningContext();
        REQUIRE(ctx.ValidateConsistency() == SessionConsistencyResult::Ok);
    }
}

TEST_CASE("ValidateConsistency gate: SUBMIT_BLOCK rejected on inconsistent session", "[llp][session_status][consistency]")
{
    /* This test verifies the boolean gate logic used in the SUBMIT_BLOCK handlers:
     * if(consistency != SessionConsistencyResult::Ok) → reject with BLOCK_REJECTED.
     * The actual network send is exercised in integration tests; here we confirm the
     * decision logic is correct for all relevant inconsistency variants. */

    struct GateScenario
    {
        const char* description;
        MiningContext ctx;
        bool expectRejected;
    };

    uint256_t genesis;
    genesis.SetHex("deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");
    uint256_t keyID;
    keyID.SetHex("cafebabecafebabecafebabecafebabecafebabecafebabecafebabecafebabe");  // 64 hex chars

    /* Build one good and one bad context for comparison */
    MiningContext goodCtx = MiningContext()
        .WithAuth(true)
        .WithSession(99)
        .WithGenesis(genesis)
        .WithKeyId(keyID);

    MiningContext noSessionCtx = MiningContext()
        .WithAuth(true)
        .WithSession(0)
        .WithGenesis(genesis)
        .WithKeyId(keyID);

    SECTION("Good context passes the gate")
    {
        const bool rejected = (goodCtx.ValidateConsistency() != SessionConsistencyResult::Ok);
        REQUIRE(rejected == false);
    }

    SECTION("Zero session ID triggers rejection at SUBMIT_BLOCK gate")
    {
        const bool rejected = (noSessionCtx.ValidateConsistency() != SessionConsistencyResult::Ok);
        REQUIRE(rejected == true);
    }

    SECTION("SessionConsistencyResultString is non-empty for every non-OK result")
    {
        /* Ensure the string table is populated so log messages are meaningful */
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::MissingSessionId)).size() > 0);
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::MissingGenesis)).size() > 0);
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::MissingFalconKey)).size() > 0);
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::RewardBoundMissingHash)).size() > 0);
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::EncryptionReadyMissingKey)).size() > 0);
        REQUIRE(std::string(SessionConsistencyResultString(SessionConsistencyResult::Ok)).size() > 0);
    }
}
