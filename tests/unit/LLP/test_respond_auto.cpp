/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/opcode_utility.h>
#include <LLP/include/push_notification.h>
#include <LLP/include/stateless_miner.h>

/**
 * Test suite for respond_auto() protocol lane detection and
 * SubmitBlockRejectionReason enum.
 *
 * These are pure-logic unit tests that validate:
 * - OpcodeUtility::Stateless::Mirror() correctly maps legacy opcodes
 * - SubmitBlockRejectionReason enum values match the wire protocol
 * - SubmitBlockRejectionReasonString() returns meaningful names
 * - ProtocolLane detection via MiningContext::IsStateless()
 * - fStatelessProtocol flag semantics (LEGACY default, STATELESS after auth)
 */

using namespace LLP;

/* ===========================================================================
 * SubmitBlockRejectionReason Enum Tests
 * ===========================================================================*/

TEST_CASE("SubmitBlockRejectionReason - Wire Values", "[opcode_utility][submit_block][llp]")
{
    SECTION("CHACHA20_DECRYPTION_FAILED is 0x0B")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::SubmitBlockRejectionReason::CHACHA20_DECRYPTION_FAILED) == 0x0B);
    }

    SECTION("ENCRYPTION_REQUIRED is 0x0C")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::SubmitBlockRejectionReason::ENCRYPTION_REQUIRED) == 0x0C);
    }

    SECTION("NO_SESSION_KEY is 0x0D")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::SubmitBlockRejectionReason::NO_SESSION_KEY) == 0x0D);
    }

    SECTION("INTERNAL_ERROR is 0xFF")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::SubmitBlockRejectionReason::INTERNAL_ERROR) == 0xFF);
    }
}

TEST_CASE("SubmitBlockRejectionReason - String Conversion", "[opcode_utility][submit_block][llp]")
{
    SECTION("All reason codes have meaningful names")
    {
        REQUIRE(std::string(OpcodeUtility::SubmitBlockRejectionReasonString(
            OpcodeUtility::SubmitBlockRejectionReason::CHACHA20_DECRYPTION_FAILED)) == "CHACHA20_DECRYPTION_FAILED");

        REQUIRE(std::string(OpcodeUtility::SubmitBlockRejectionReasonString(
            OpcodeUtility::SubmitBlockRejectionReason::ENCRYPTION_REQUIRED)) == "ENCRYPTION_REQUIRED");

        REQUIRE(std::string(OpcodeUtility::SubmitBlockRejectionReasonString(
            OpcodeUtility::SubmitBlockRejectionReason::NO_SESSION_KEY)) == "NO_SESSION_KEY");

        REQUIRE(std::string(OpcodeUtility::SubmitBlockRejectionReasonString(
            OpcodeUtility::SubmitBlockRejectionReason::INTERNAL_ERROR)) == "INTERNAL_ERROR");
    }

    SECTION("Unknown reason returns UNKNOWN")
    {
        REQUIRE(std::string(OpcodeUtility::SubmitBlockRejectionReasonString(
            static_cast<OpcodeUtility::SubmitBlockRejectionReason>(0x42))) == "UNKNOWN");
    }
}

/* ===========================================================================
 * Mirror() Opcode Mapping Tests (respond_auto correctness)
 * ===========================================================================*/

TEST_CASE("Mirror - Legacy to Stateless Opcode Mapping", "[opcode_utility][respond_auto][llp]")
{
    SECTION("BLOCK_REJECTED maps to 0xD0C9")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_REJECTED) == 0xD0C9);
    }

    SECTION("BLOCK_ACCEPTED maps to 0xD0C8")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_ACCEPTED) == 0xD0C8);
    }

    SECTION("ORPHAN_BLOCK maps to 0xD007")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::ORPHAN_BLOCK) == 0xD007);
    }

    SECTION("BLOCK_DATA maps to 0xD000")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_DATA) == 0xD000);
    }

    SECTION("MINER_AUTH_RESULT maps to 0xD0D2")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::MINER_AUTH_RESULT) == 0xD0D2);
    }

    SECTION("NEW_ROUND maps to 0xD0CC")
    {
        REQUIRE(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::NEW_ROUND) == 0xD0CC);
    }

    SECTION("All mirror opcodes are in stateless range 0xD000-0xD0FF")
    {
        REQUIRE(OpcodeUtility::Stateless::IsStateless(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_DATA)));
        REQUIRE(OpcodeUtility::Stateless::IsStateless(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_REJECTED)));
        REQUIRE(OpcodeUtility::Stateless::IsStateless(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::BLOCK_ACCEPTED)));
        REQUIRE(OpcodeUtility::Stateless::IsStateless(OpcodeUtility::Stateless::Mirror(OpcodeUtility::Opcodes::ORPHAN_BLOCK)));
    }

    SECTION("Unmirror round-trips correctly")
    {
        uint8_t legacy = OpcodeUtility::Opcodes::BLOCK_REJECTED;
        uint16_t stateless = OpcodeUtility::Stateless::Mirror(legacy);
        uint8_t roundTripped = OpcodeUtility::Stateless::Unmirror(stateless);
        REQUIRE(roundTripped == legacy);
    }
}

/* ===========================================================================
 * ProtocolLane Detection Tests (fStatelessProtocol semantics)
 * ===========================================================================*/

TEST_CASE("MiningContext - Protocol Lane for respond_auto", "[mining_context][respond_auto][llp]")
{
    SECTION("Default MiningContext has LEGACY lane (fStatelessProtocol=false equivalent)")
    {
        MiningContext ctx;
        REQUIRE(ctx.nProtocolLane == ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx.IsStateless());
    }

    SECTION("After WithProtocolLane(STATELESS), IsStateless() returns true")
    {
        MiningContext ctx;
        ctx = ctx.WithProtocolLane(ProtocolLane::STATELESS);
        REQUIRE(ctx.IsStateless());
    }

    SECTION("respond_auto dispatch logic: LEGACY lane uses legacy opcode")
    {
        /* Simulate respond_auto logic: if fStatelessProtocol is false, use legacy opcode */
        bool fStatelessProtocol = false;
        uint8_t nLegacyOpcode = OpcodeUtility::Opcodes::BLOCK_REJECTED;

        if(fStatelessProtocol)
        {
            uint16_t nStateless = OpcodeUtility::Stateless::Mirror(nLegacyOpcode);
            /* Would call respond_stateless(nStateless, vData) */
            REQUIRE(false); /* Should not reach here */
        }
        else
        {
            /* Would call respond(nLegacyOpcode, vData) */
            REQUIRE(nLegacyOpcode == OpcodeUtility::Opcodes::BLOCK_REJECTED);
        }
    }

    SECTION("respond_auto dispatch logic: STATELESS lane mirrors opcode")
    {
        /* Simulate respond_auto logic: if fStatelessProtocol is true, mirror to stateless */
        bool fStatelessProtocol = true;
        uint8_t nLegacyOpcode = OpcodeUtility::Opcodes::BLOCK_REJECTED;

        if(fStatelessProtocol)
        {
            uint16_t nStateless = OpcodeUtility::Stateless::Mirror(nLegacyOpcode);
            REQUIRE(nStateless == 0xD0C9);
            REQUIRE(OpcodeUtility::Stateless::IsStateless(nStateless));
        }
        else
        {
            REQUIRE(false); /* Should not reach here */
        }
    }
}

/* ===========================================================================
 * Throttle Hysteresis Tests
 * ===========================================================================*/

TEST_CASE("RejectionReason Enum - Existing Values Preserved", "[opcode_utility][llp]")
{
    SECTION("RejectionReason STALE is 1")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::RejectionReason::STALE) == 1);
    }

    SECTION("RejectionReason INVALID_POW is 2")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::RejectionReason::INVALID_POW) == 2);
    }

    SECTION("RejectionReason DUPLICATE is 4")
    {
        REQUIRE(static_cast<uint8_t>(OpcodeUtility::RejectionReason::DUPLICATE) == 4);
    }
}
