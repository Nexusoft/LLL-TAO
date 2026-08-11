/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/get_block_policy.h>

#include <chrono>
#include <cstdint>
#include <string>

using namespace LLP;

TEST_CASE("GET_BLOCK rolling limiter enforces per-session budget", "[simlink][rate_limit]")
{
    using clock = GetBlockRollingLimiter::clock;

    SECTION("Same session key shares one rolling window")
    {
        GetBlockRollingLimiter limiter(20, std::chrono::seconds(60));
        const std::string strKey = "session=123|combined";

        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;
        const clock::time_point tStart{};

        for(int i = 0; i < 20; ++i)
        {
            REQUIRE(limiter.Allow(strKey, tStart + std::chrono::seconds(i), nRetryAfterMs, nCurrentInWindow));
            REQUIRE(nRetryAfterMs == 0);
        }

        REQUIRE_FALSE(limiter.Allow(strKey, tStart + std::chrono::seconds(20), nRetryAfterMs, nCurrentInWindow));
        REQUIRE(nRetryAfterMs > 0);
    }

    SECTION("Different session keys get independent budgets")
    {
        GetBlockRollingLimiter limiter(1, std::chrono::seconds(60));

        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;
        const clock::time_point tNow{};

        REQUIRE(limiter.Allow("session=111|combined", tNow, nRetryAfterMs, nCurrentInWindow));
        REQUIRE(limiter.Allow("session=222|combined", tNow, nRetryAfterMs, nCurrentInWindow));
    }

    SECTION("Requests age out after the rolling window")
    {
        GetBlockRollingLimiter limiter(1, std::chrono::seconds(60));
        const std::string strKey = "session=456|combined";

        uint32_t nRetryAfterMs = 0;
        std::size_t nCurrentInWindow = 0;
        const clock::time_point tStart{};

        REQUIRE(limiter.Allow(strKey, tStart, nRetryAfterMs, nCurrentInWindow));
        REQUIRE_FALSE(limiter.Allow(strKey, tStart + std::chrono::seconds(1), nRetryAfterMs, nCurrentInWindow));
        REQUIRE(limiter.Allow(strKey, tStart + std::chrono::seconds(60), nRetryAfterMs, nCurrentInWindow));
    }
}

TEST_CASE("GET_BLOCK policy helpers build canonical control payloads", "[simlink][policy]")
{
    SECTION("Retryable reason preserves retry_after_ms")
    {
        std::vector<uint8_t> payload = BuildGetBlockControlPayload(
            GetBlockPolicyReason::RATE_LIMIT_EXCEEDED, 1500);

        REQUIRE(payload.size() == 8);
        REQUIRE(payload[0] == GET_BLOCK_CONTROL_PAYLOAD_VERSION);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED));
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x05);
        REQUIRE(payload[7] == 0xDC);
    }

    SECTION("Non-retryable reason zeroes retry_after_ms")
    {
        std::vector<uint8_t> payload = BuildGetBlockControlPayload(
            GetBlockPolicyReason::SESSION_INVALID, 1500);

        REQUIRE(payload.size() == 8);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::SESSION_INVALID));
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x00);
        REQUIRE(payload[7] == 0x00);
    }

    SECTION("Reason-code helpers match current semantics")
    {
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED))
                == "RATE_LIMIT_EXCEEDED");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::CHANNEL_NOT_SET))
                == "CHANNEL_NOT_SET");
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE));
        REQUIRE_FALSE(IsGetBlockRetryable(GetBlockPolicyReason::SESSION_INVALID));
    }
}
