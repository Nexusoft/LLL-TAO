/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_constants.h>

#include <cstdint>
#include <set>
#include <string>

using namespace LLP;

/**
 * GET_BLOCK Response Contract Tests
 *
 * These tests verify the deterministic response contract for GET_BLOCK:
 * - Every reason code has a human-readable name (no silent/unknown codes)
 * - Retryable reasons produce retry_after_ms in control payloads
 * - Non-retryable reasons force retry_after_ms to zero
 * - Enum values are unique and non-overlapping
 * - New reason codes for template state are properly defined
 */

TEST_CASE("GET_BLOCK reason codes cover all template states", "[get_block][response_contract]")
{
    SECTION("All reason codes have explicit string names")
    {
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::NONE)) == "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED)) == "RATE_LIMIT_EXCEEDED");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::SESSION_INVALID)) == "SESSION_INVALID");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::UNAUTHENTICATED)) == "UNAUTHENTICATED");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_NOT_READY)) == "TEMPLATE_NOT_READY");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::INTERNAL_RETRY)) == "INTERNAL_RETRY");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_STALE)) == "TEMPLATE_STALE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS)) == "TEMPLATE_REBUILD_IN_PROGRESS");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE)) == "TEMPLATE_SOURCE_UNAVAILABLE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::CHANNEL_NOT_SET)) == "CHANNEL_NOT_SET");
    }

    SECTION("Backward-compatible alias NO_TEMPLATE_READY resolves to TEMPLATE_NOT_READY")
    {
        REQUIRE(static_cast<uint8_t>(GetBlockPolicyReason::NO_TEMPLATE_READY) ==
                static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_NOT_READY));
    }

    SECTION("Enum values are unique (no collisions except deprecated alias)")
    {
        std::set<uint8_t> seen;
        /* NO_TEMPLATE_READY intentionally aliases TEMPLATE_NOT_READY — skip it */
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::NONE));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::SESSION_INVALID));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::UNAUTHENTICATED));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_NOT_READY));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::INTERNAL_RETRY));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_STALE));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE));
        seen.insert(static_cast<uint8_t>(GetBlockPolicyReason::CHANNEL_NOT_SET));
        /* 10 distinct values (NONE through CHANNEL_NOT_SET) */
        REQUIRE(seen.size() == 10);
    }
}

TEST_CASE("GET_BLOCK retryable classification", "[get_block][response_contract][retry]")
{
    SECTION("Template-state reasons are retryable")
    {
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::TEMPLATE_NOT_READY));
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::TEMPLATE_STALE));
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS));
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE));
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::INTERNAL_RETRY));
        REQUIRE(IsGetBlockRetryable(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED));
    }

    SECTION("Terminal reasons are not retryable")
    {
        REQUIRE_FALSE(IsGetBlockRetryable(GetBlockPolicyReason::NONE));
        REQUIRE_FALSE(IsGetBlockRetryable(GetBlockPolicyReason::SESSION_INVALID));
        REQUIRE_FALSE(IsGetBlockRetryable(GetBlockPolicyReason::UNAUTHENTICATED));
        REQUIRE_FALSE(IsGetBlockRetryable(GetBlockPolicyReason::CHANNEL_NOT_SET));
    }
}

TEST_CASE("GET_BLOCK control payload for new reason codes", "[get_block][response_contract][payload]")
{
    SECTION("TEMPLATE_STALE includes retry_after_ms")
    {
        const auto payload = BuildGetBlockControlPayload(GetBlockPolicyReason::TEMPLATE_STALE, 500u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[0] == GET_BLOCK_CONTROL_PAYLOAD_VERSION);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_STALE));
        /* 500 = 0x000001F4 big-endian */
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x01);
        REQUIRE(payload[7] == 0xF4);
    }

    SECTION("TEMPLATE_REBUILD_IN_PROGRESS includes retry_after_ms")
    {
        const auto payload = BuildGetBlockControlPayload(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS, 250u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS));
        /* 250 = 0x000000FA big-endian */
        REQUIRE(payload[7] == 0xFA);
    }

    SECTION("TEMPLATE_SOURCE_UNAVAILABLE includes retry_after_ms")
    {
        const auto payload = BuildGetBlockControlPayload(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE, 2000u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE));
        /* 2000 = 0x000007D0 big-endian */
        REQUIRE(payload[6] == 0x07);
        REQUIRE(payload[7] == 0xD0);
    }

    SECTION("CHANNEL_NOT_SET is non-retryable so retry_after_ms is forced to 0")
    {
        const auto payload = BuildGetBlockControlPayload(GetBlockPolicyReason::CHANNEL_NOT_SET, 9999u);
        REQUIRE(payload.size() == 8u);
        REQUIRE(payload[1] == static_cast<uint8_t>(GetBlockPolicyReason::CHANNEL_NOT_SET));
        REQUIRE(payload[4] == 0x00);
        REQUIRE(payload[5] == 0x00);
        REQUIRE(payload[6] == 0x00);
        REQUIRE(payload[7] == 0x00);
    }
}

TEST_CASE("GET_BLOCK response contract: no silent drops possible", "[get_block][response_contract][liveness]")
{
    SECTION("Every defined reason code produces a non-empty payload")
    {
        /* Iterate all known reason codes and verify BuildGetBlockControlPayload
         * always returns exactly 8 bytes (never empty / never silent). */
        const GetBlockPolicyReason allReasons[] = {
            GetBlockPolicyReason::NONE,
            GetBlockPolicyReason::RATE_LIMIT_EXCEEDED,
            GetBlockPolicyReason::SESSION_INVALID,
            GetBlockPolicyReason::UNAUTHENTICATED,
            GetBlockPolicyReason::TEMPLATE_NOT_READY,
            GetBlockPolicyReason::INTERNAL_RETRY,
            GetBlockPolicyReason::TEMPLATE_STALE,
            GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS,
            GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE,
            GetBlockPolicyReason::CHANNEL_NOT_SET,
        };

        for(auto eReason : allReasons)
        {
            const auto payload = BuildGetBlockControlPayload(eReason, 1000u);
            REQUIRE(payload.size() == 8u);
            REQUIRE(payload[0] == GET_BLOCK_CONTROL_PAYLOAD_VERSION);
            REQUIRE(payload[1] == static_cast<uint8_t>(eReason));
        }
    }

    SECTION("Every reason code has a non-NONE string representation")
    {
        /* Except NONE itself, every reason must map to a descriptive string. */
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::RATE_LIMIT_EXCEEDED)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::SESSION_INVALID)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::UNAUTHENTICATED)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_NOT_READY)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::INTERNAL_RETRY)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_STALE)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE)) != "NONE");
        REQUIRE(std::string(GetBlockPolicyReasonCode(GetBlockPolicyReason::CHANNEL_NOT_SET)) != "NONE");
    }
}

TEST_CASE("GET_BLOCK throttle constants are bounded", "[get_block][response_contract][timeout]")
{
    SECTION("GET_BLOCK cooldown is non-zero and reasonable")
    {
        REQUIRE(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS > 0);
        REQUIRE(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS <= 10);
    }

    SECTION("GET_BLOCK throttle interval is non-zero")
    {
        REQUIRE(MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS > 0);
        REQUIRE(MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS <= 10000);
    }

    SECTION("Rolling limiter defaults are sane")
    {
        REQUIRE(GET_BLOCK_ROLLING_LIMIT_PER_MINUTE >= 5);
        REQUIRE(GET_BLOCK_ROLLING_LIMIT_PER_MINUTE <= 100);
        REQUIRE(GET_BLOCK_ROLLING_WINDOW.count() == 60);
    }
}
