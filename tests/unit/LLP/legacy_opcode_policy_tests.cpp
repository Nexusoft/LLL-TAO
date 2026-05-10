/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/legacy_opcode_policy.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/stateless_miner.h>


/* These tests pin the legacy (port 8323) preflight policy and rejection
 * framing contract so they cannot silently drift.  They cover Options 4 + 5
 * of the Legacy Lane architectural fix:
 *
 *   - Option 5: a single LEGACY_OPCODE_POLICY table is the source of truth
 *     for "is this opcode protected" and "what minimum state does it need".
 *   - Option 4: only true block opcodes (GET_BLOCK / SUBMIT_BLOCK) may
 *     receive BLOCK_REJECTED responses; auth/session/config opcodes must
 *     not.
 */
TEST_CASE("Legacy opcode policy table — single source of truth", "[llp][legacy_opcode_policy]")
{
    namespace Opcodes = LLP::OpcodeUtility::Opcodes;

    SECTION("Channel-requiring opcodes need at least CHANNEL_SET")
    {
        const uint8_t channelOpcodes[] = {
            Opcodes::GET_BLOCK, Opcodes::SUBMIT_BLOCK, Opcodes::GET_ROUND,
            Opcodes::GET_REWARD, Opcodes::MINER_READY, Opcodes::CHECK_BLOCK,
        };

        for(const uint8_t nOpcode : channelOpcodes)
        {
            CAPTURE(static_cast<uint32_t>(nOpcode));
            REQUIRE(LLP::IsLegacyPreflightProtectedOpcode(nOpcode));
            REQUIRE(LLP::MinimumStateForLegacyOpcode(nOpcode) == LLP::MinerSessionState::CHANNEL_SET);
            REQUIRE_FALSE(LLP::IsLegacyPreflightBypassOpcode(nOpcode));
        }
    }

    SECTION("MINER_SET_REWARD requires ENCRYPTION_READY")
    {
        REQUIRE(LLP::IsLegacyPreflightProtectedOpcode(Opcodes::MINER_SET_REWARD));
        REQUIRE(LLP::MinimumStateForLegacyOpcode(Opcodes::MINER_SET_REWARD)
                == LLP::MinerSessionState::ENCRYPTION_READY);
    }

    SECTION("Auth/session-mgmt opcodes need only AUTHENTICATED")
    {
        const uint8_t authOpcodes[] = {
            Opcodes::SET_CHANNEL, Opcodes::SESSION_KEEPALIVE,
            Opcodes::GET_HEIGHT, Opcodes::CLEAR_MAP, Opcodes::SUBSCRIBE,
        };

        for(const uint8_t nOpcode : authOpcodes)
        {
            CAPTURE(static_cast<uint32_t>(nOpcode));
            REQUIRE(LLP::IsLegacyPreflightProtectedOpcode(nOpcode));
            REQUIRE(LLP::MinimumStateForLegacyOpcode(nOpcode) == LLP::MinerSessionState::AUTHENTICATED);
        }
    }

    SECTION("Bypass opcodes are not in the policy table")
    {
        const uint8_t bypassOpcodes[] = {
            Opcodes::MINER_AUTH_INIT, Opcodes::MINER_AUTH_RESPONSE,
            Opcodes::MINER_AUTH_RESULT, Opcodes::SESSION_START,
        };

        for(const uint8_t nOpcode : bypassOpcodes)
        {
            CAPTURE(static_cast<uint32_t>(nOpcode));
            REQUIRE(LLP::IsLegacyPreflightBypassOpcode(nOpcode));
            REQUIRE_FALSE(LLP::IsLegacyPreflightProtectedOpcode(nOpcode));
            REQUIRE(LLP::MinimumStateForLegacyOpcode(nOpcode) == LLP::MinerSessionState::CONNECTED);
        }
    }

    SECTION("Unknown opcodes are not protected and not bypass")
    {
        const uint8_t nUnknown = 0x42;  // not in either set
        REQUIRE_FALSE(LLP::IsLegacyPreflightBypassOpcode(nUnknown));
        REQUIRE_FALSE(LLP::IsLegacyPreflightProtectedOpcode(nUnknown));
        REQUIRE(LLP::MinimumStateForLegacyOpcode(nUnknown) == LLP::MinerSessionState::CONNECTED);
    }

    SECTION("Bypass and protected sets are disjoint")
    {
        for(const auto& entry : LLP::LEGACY_OPCODE_POLICY)
        {
            CAPTURE(static_cast<uint32_t>(entry.nOpcode));
            REQUIRE_FALSE(LLP::IsLegacyPreflightBypassOpcode(entry.nOpcode));
        }
    }
}


TEST_CASE("Legacy rejection framing contract — only block opcodes may receive BLOCK_REJECTED",
          "[llp][legacy_opcode_policy][rejection_framing]")
{
    namespace Opcodes = LLP::OpcodeUtility::Opcodes;

    SECTION("GET_BLOCK and SUBMIT_BLOCK are block opcodes")
    {
        REQUIRE(LLP::IsLegacyBlockOpcode(Opcodes::GET_BLOCK));
        REQUIRE(LLP::IsLegacyBlockOpcode(Opcodes::SUBMIT_BLOCK));
    }

    SECTION("Auth, session, channel, reward, ready, subscribe opcodes are NOT block opcodes")
    {
        /* Sending BLOCK_REJECTED in response to any of these is a protocol
         * violation that legacy miner clients (NexusMiner, cpuminer-multi)
         * treat as fatal POLL_ERROR.  This pins the contract that the legacy
         * lane's PreflightSessionGate must drop these silently rather than
         * reply with BLOCK_REJECTED. */
        const uint8_t nonBlockOpcodes[] = {
            Opcodes::MINER_READY,        // The opcode visible in the bug screenshot
            Opcodes::SET_CHANNEL,
            Opcodes::SESSION_KEEPALIVE,
            Opcodes::MINER_SET_REWARD,
            Opcodes::SUBSCRIBE,
            Opcodes::GET_HEIGHT,
            Opcodes::GET_ROUND,
            Opcodes::GET_REWARD,
            Opcodes::CLEAR_MAP,
            Opcodes::CHECK_BLOCK,
            Opcodes::MINER_AUTH_INIT,
            Opcodes::MINER_AUTH_RESPONSE,
            Opcodes::SESSION_START,
        };

        for(const uint8_t nOpcode : nonBlockOpcodes)
        {
            CAPTURE(static_cast<uint32_t>(nOpcode));
            REQUIRE_FALSE(LLP::IsLegacyBlockOpcode(nOpcode));
        }
    }

    SECTION("Block opcodes are also protected with CHANNEL_SET — they must clear gate before processing")
    {
        REQUIRE(LLP::IsLegacyPreflightProtectedOpcode(Opcodes::GET_BLOCK));
        REQUIRE(LLP::IsLegacyPreflightProtectedOpcode(Opcodes::SUBMIT_BLOCK));
        REQUIRE(LLP::MinimumStateForLegacyOpcode(Opcodes::GET_BLOCK)    == LLP::MinerSessionState::CHANNEL_SET);
        REQUIRE(LLP::MinimumStateForLegacyOpcode(Opcodes::SUBMIT_BLOCK) == LLP::MinerSessionState::CHANNEL_SET);
    }
}


TEST_CASE("MiningContext::ComputeSessionState advances to CHANNEL_SET when channel is set",
          "[llp][legacy_opcode_policy][session_state]")
{
    /* This test pins the architectural contract that motivates Option 1:
     * once a SET_CHANNEL handler returns a context with nChannel != 0, that
     * channel MUST flow into the manager so the cached session state advances
     * past ENCRYPTION_READY.  Option 1 fixes the legacy lane's TransformMiner
     * commit which previously discarded nChannel.
     *
     * Here we drive ComputeSessionState directly to lock in the state-machine
     * expectations that PreflightSessionGate relies on.
     */
    using MSS = LLP::MinerSessionState;

    SECTION("All-zero context is CONNECTED")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(false, false, 0, false) == MSS::CONNECTED);
    }

    SECTION("Authenticated only is AUTHENTICATED")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, false, 0, false) == MSS::AUTHENTICATED);
    }

    SECTION("Authenticated + encryption is ENCRYPTION_READY")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, true, 0, false) == MSS::ENCRYPTION_READY);
    }

    SECTION("Authenticated + encryption + channel is CHANNEL_SET (the bug-fix invariant)")
    {
        /* Before Option 1, the legacy lane's TransformMiner commit dropped
         * the nChannel field after a successful SET_CHANNEL — so even though
         * a handler returned (true, true, 1, false), the manager-cached
         * context kept seeing nChannel=0 and capped state at ENCRYPTION_READY.
         * MINER_READY then failed PreflightSessionGate with
         * "ENCRYPTION_READY < required CHANNEL_SET". */
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, true, 1, false) == MSS::CHANNEL_SET);
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, true, 2, false) == MSS::CHANNEL_SET);
    }

    SECTION("Channel set without encryption still caps at AUTHENTICATED (impossible state suppression)")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, false, 1, false) == MSS::AUTHENTICATED);
    }

    SECTION("Subscribed without channel caps at ENCRYPTION_READY")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, true, 0, true) == MSS::ENCRYPTION_READY);
    }

    SECTION("All flags set is MINING")
    {
        REQUIRE(LLP::MiningContext::ComputeSessionState(true, true, 1, true) == MSS::MINING);
    }
}
