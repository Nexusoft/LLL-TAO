/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_auth.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/session_recovery.h>
#include <LLP/packets/packet.h>

#include <LLC/include/random.h>
#include <Util/include/runtime.h>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace LLP;

/* Packet type definitions for testing - Phase 2 Unified Hybrid Protocol */
const Packet::message_t MINER_AUTH_INIT_TEST = 207;
const Packet::message_t MINER_AUTH_CHALLENGE_TEST = 208;
const Packet::message_t MINER_AUTH_RESPONSE_TEST = 209;
const Packet::message_t MINER_AUTH_RESULT_TEST = 210;
const Packet::message_t SESSION_START_TEST = 211;
const Packet::message_t SESSION_KEEPALIVE_TEST = 212;


/** Test Suite: Falcon Authentication Stress Testing
 *
 *  These tests simulate edge cases such as:
 *  - Authentication failures and mismatches in Falcon sessions
 *  - Session keep-alive mechanisms under network stress
 *  - Encrypted challenge-response flows verification
 *
 **/

TEST_CASE("Falcon Authentication Edge Cases", "[falcon_auth][stress]")
{
    /* Initialize Falcon auth for testing */
    FalconAuth::Initialize();

    SECTION("Authentication with invalid public key bytes")
    {
        MiningContext ctx;

        Packet packet(MINER_AUTH_INIT_TEST);

        /* Create intentionally malformed pubkey data */
        std::vector<uint8_t> badPubKey(100, 0xFF);  // Invalid pattern
        uint16_t nPubKeyLen = static_cast<uint16_t>(badPubKey.size());

        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), badPubKey.begin(), badPubKey.end());

        std::string testMinerId = "malicious_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), testMinerId.begin(), testMinerId.end());

        /* Process should succeed at INIT stage (pubkey stored for challenge) */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Init succeeds - challenge is generated */
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.vMinerPubKey == badPubKey);
    }

    SECTION("Authentication with empty signature")
    {
        MiningContext ctx = MiningContext()
            .WithNonce(std::vector<uint8_t>(32, 0x42))  // Valid nonce
            .WithPubKey(std::vector<uint8_t>(897, 0x42));  // Valid-length pubkey

        Packet packet(MINER_AUTH_RESPONSE_TEST);

        /* Empty signature length */
        packet.DATA.push_back(0x00);
        packet.DATA.push_back(0x00);

        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - sig_len is 0 */
        REQUIRE(result.fSuccess == true);  // Returns success but with failure code
        REQUIRE(result.response.DATA.size() >= 1);
        REQUIRE(result.response.DATA[0] == 0x00);  // Failure code
    }

    SECTION("Authentication with truncated signature data")
    {
        MiningContext ctx = MiningContext()
            .WithNonce(std::vector<uint8_t>(32, 0x42))
            .WithPubKey(std::vector<uint8_t>(897, 0x42));

        Packet packet(MINER_AUTH_RESPONSE_TEST);

        /* Claim 100 bytes but provide fewer */
        packet.DATA.push_back(0x00);
        packet.DATA.push_back(0x64);  // 100 bytes claimed
        packet.DATA.push_back(0x01);  // Only 1 byte provided

        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - packet too small for signature */
        REQUIRE(result.fSuccess == true);  // Returns success but with failure code
        REQUIRE(result.response.DATA.size() >= 1);
        REQUIRE(result.response.DATA[0] == 0x00);  // Failure code
    }

    SECTION("Authentication response without prior INIT")
    {
        MiningContext ctx;  // Fresh context without nonce/pubkey

        Packet packet(MINER_AUTH_RESPONSE_TEST);
        packet.DATA.push_back(0x00);
        packet.DATA.push_back(0x10);  // 16 bytes
        for(int i = 0; i < 16; ++i)
            packet.DATA.push_back(0xAB);

        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - no nonce stored from INIT */
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.response.DATA.size() >= 1);
        REQUIRE(result.response.DATA[0] == 0x00);  // Failure code
    }

    SECTION("Rapid repeated authentication attempts (stress)")
    {
        /* Simulate rapid repeated auth attempts */
        const int nAttempts = 50;
        int nSuccess = 0;
        int nFail = 0;

        for(int i = 0; i < nAttempts; ++i)
        {
            MiningContext ctx;
            ctx = ctx.WithTimestamp(runtime::unifiedtimestamp());

            Packet initPacket(MINER_AUTH_INIT_TEST);
            std::vector<uint8_t> testPubKey(897, static_cast<uint8_t>(i & 0xFF));
            uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());

            initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
            initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
            initPacket.DATA.insert(initPacket.DATA.end(), testPubKey.begin(), testPubKey.end());

            std::string minerId = "stress_test_" + std::to_string(i);
            uint16_t nMinerIdLen = static_cast<uint16_t>(minerId.size());
            initPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
            initPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
            initPacket.DATA.insert(initPacket.DATA.end(), minerId.begin(), minerId.end());

            ProcessResult result = StatelessMiner::ProcessPacket(ctx, initPacket);

            if(result.fSuccess)
                ++nSuccess;
            else
                ++nFail;
        }

        /* All INIT requests should succeed (challenge generation) */
        REQUIRE(nSuccess == nAttempts);
        REQUIRE(nFail == 0);
    }

    FalconAuth::Shutdown();
}


TEST_CASE("Session Keep-Alive Stress Testing", "[falcon_auth][keepalive][stress]")
{
    SECTION("Rapid keepalive flood (network stress simulation)")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345)
            .WithSessionStart(runtime::unifiedtimestamp())
            .WithSessionTimeout(300)
            .WithTimestamp(runtime::unifiedtimestamp());

        const int nKeepalives = 100;
        uint32_t nKeepaliveCount = 0;

        for(int i = 0; i < nKeepalives; ++i)
        {
            Packet packet(SESSION_KEEPALIVE_TEST);

            ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

            REQUIRE(result.fSuccess == true);
            REQUIRE(result.context.nKeepaliveCount == ctx.nKeepaliveCount + 1);

            /* Update context for next iteration */
            ctx = result.context;
            ++nKeepaliveCount;
        }

        /* Final keepalive count should match */
        REQUIRE(ctx.nKeepaliveCount == nKeepalives);
    }

    SECTION("Keepalive with expired session")
    {
        /* Create context with old timestamp (expired) */
        uint64_t nOldTime = runtime::unifiedtimestamp() - 3600;  // 1 hour ago
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(99999)
            .WithSessionStart(nOldTime)
            .WithSessionTimeout(300)  // 5 minute timeout
            .WithTimestamp(nOldTime);

        Packet packet(SESSION_KEEPALIVE_TEST);
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - session expired */
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("expired") != std::string::npos);
    }

    SECTION("Keepalive before session start")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(11111);
        /* No session start timestamp set (nSessionStart == 0) */

        Packet packet(SESSION_KEEPALIVE_TEST);
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - session not started */
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("not started") != std::string::npos);
    }

    SECTION("Keepalive without authentication")
    {
        MiningContext ctx;  // Not authenticated

        Packet packet(SESSION_KEEPALIVE_TEST);
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);

        /* Should fail - not authenticated */
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("authenticated") != std::string::npos);
    }
}


TEST_CASE("Challenge-Response Flow Verification", "[falcon_auth][challenge]")
{
    /* Initialize Falcon auth for testing */
    FalconAuth::Initialize();

    SECTION("Full challenge-response flow with valid key")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a real Falcon key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "test_challenge_key"
        );

        /* Step 1: MINER_AUTH_INIT */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT_TEST);

        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        std::string minerId = "challenge_test_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(minerId.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        initPacket.DATA.insert(initPacket.DATA.end(), minerId.begin(), minerId.end());

        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE_TEST);
        REQUIRE(!initResult.context.vAuthNonce.empty());

        /* Step 2: Sign the challenge with real Falcon key */
        std::vector<uint8_t> vSignature = pAuth->Sign(meta.keyId, initResult.context.vAuthNonce);
        REQUIRE(!vSignature.empty());

        /* Step 3: MINER_AUTH_RESPONSE */
        Packet responsePacket(MINER_AUTH_RESPONSE_TEST);
        uint16_t nSigLen = static_cast<uint16_t>(vSignature.size());
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen >> 8));
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen & 0xFF));
        responsePacket.DATA.insert(responsePacket.DATA.end(), vSignature.begin(), vSignature.end());

        ProcessResult authResult = StatelessMiner::ProcessPacket(initResult.context, responsePacket);

        /* Should succeed with valid signature */
        REQUIRE(authResult.fSuccess == true);
        REQUIRE(authResult.response.HEADER == MINER_AUTH_RESULT_TEST);
        REQUIRE(authResult.response.DATA.size() >= 1);
        REQUIRE(authResult.response.DATA[0] == 0x01);  // Success code
        REQUIRE(authResult.context.fAuthenticated == true);
    }

    SECTION("Challenge-response with wrong key")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate two different keys */
        FalconAuth::KeyMetadata keyA = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "key_A"
        );

        FalconAuth::KeyMetadata keyB = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "key_B"
        );

        /* INIT with key A */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT_TEST);

        uint16_t nPubKeyLen = static_cast<uint16_t>(keyA.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        initPacket.DATA.insert(initPacket.DATA.end(), keyA.pubkey.begin(), keyA.pubkey.end());

        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);
        REQUIRE(initResult.fSuccess == true);

        /* Sign with key B (wrong key!) */
        std::vector<uint8_t> wrongSig = pAuth->Sign(keyB.keyId, initResult.context.vAuthNonce);
        REQUIRE(!wrongSig.empty());

        /* RESPONSE with wrong signature */
        Packet responsePacket(MINER_AUTH_RESPONSE_TEST);
        uint16_t nSigLen = static_cast<uint16_t>(wrongSig.size());
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen >> 8));
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen & 0xFF));
        responsePacket.DATA.insert(responsePacket.DATA.end(), wrongSig.begin(), wrongSig.end());

        ProcessResult authResult = StatelessMiner::ProcessPacket(initResult.context, responsePacket);

        /* Should fail - signature doesn't match pubkey */
        REQUIRE(authResult.fSuccess == true);  // Returns response but with failure
        REQUIRE(authResult.response.DATA.size() >= 1);
        REQUIRE(authResult.response.DATA[0] == 0x00);  // Failure code
        REQUIRE(authResult.context.fAuthenticated == false);
    }

    SECTION("Challenge-response with tampered nonce")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        FalconAuth::KeyMetadata meta = pAuth->GenerateKey();

        /* INIT */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT_TEST);

        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);
        REQUIRE(initResult.fSuccess == true);

        /* Sign a DIFFERENT nonce (tampered) */
        std::vector<uint8_t> tamperedNonce = initResult.context.vAuthNonce;
        if(!tamperedNonce.empty())
            tamperedNonce[0] ^= 0xFF;  // Flip bits in first byte

        std::vector<uint8_t> badSig = pAuth->Sign(meta.keyId, tamperedNonce);
        REQUIRE(!badSig.empty());

        /* RESPONSE with signature over wrong nonce */
        Packet responsePacket(MINER_AUTH_RESPONSE_TEST);
        uint16_t nSigLen = static_cast<uint16_t>(badSig.size());
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen >> 8));
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen & 0xFF));
        responsePacket.DATA.insert(responsePacket.DATA.end(), badSig.begin(), badSig.end());

        ProcessResult authResult = StatelessMiner::ProcessPacket(initResult.context, responsePacket);

        /* Should fail - signature is over wrong message */
        REQUIRE(authResult.fSuccess == true);
        REQUIRE(authResult.response.DATA.size() >= 1);
        REQUIRE(authResult.response.DATA[0] == 0x00);  // Failure code
    }

    FalconAuth::Shutdown();
}


TEST_CASE("Falcon Session Challenge Scaling", "[falcon_auth][scaling]")
{
    /* Initialize Falcon auth with custom config */
    FalconAuth::ChallengeConfig config;
    config.nMinChallengeSize = 32;
    config.nMaxChallengeSize = 64;
    config.nScaleThreshold = 100;
    config.nChallengeTimeout = 60;

    FalconAuth::InitializeWithConfig(config);

    SECTION("Challenge size scales with active sessions")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Low load - minimum challenge size */
        size_t lowLoadSize = pAuth->GetChallengeSize(10);
        REQUIRE(lowLoadSize == 32);

        /* At threshold - still minimum */
        size_t thresholdSize = pAuth->GetChallengeSize(100);
        REQUIRE(thresholdSize == 32);

        /* Above threshold - starts scaling */
        size_t scaledSize = pAuth->GetChallengeSize(200);
        REQUIRE(scaledSize > 32);
        REQUIRE(scaledSize <= 64);

        /* High load - approaches maximum */
        size_t highLoadSize = pAuth->GetChallengeSize(500);
        REQUIRE(highLoadSize > scaledSize);
        REQUIRE(highLoadSize <= 64);
    }

    SECTION("Challenge generation produces expected sizes")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate challenges at different load levels */
        std::vector<uint8_t> lowChallenge = pAuth->GenerateChallenge(50);
        REQUIRE(lowChallenge.size() == 32);

        std::vector<uint8_t> highChallenge = pAuth->GenerateChallenge(400);
        REQUIRE(highChallenge.size() >= 32);
        REQUIRE(highChallenge.size() <= 64);
    }

    SECTION("Session stats are tracked")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate some challenges to increment stats */
        for(int i = 0; i < 5; ++i)
        {
            pAuth->GenerateChallenge(i * 50);
        }

        std::string stats = pAuth->GetSessionStats();

        /* Stats should be valid JSON with expected fields */
        REQUIRE(!stats.empty());
        REQUIRE(stats.find("challenges_generated") != std::string::npos);
        REQUIRE(stats.find("min_challenge_size") != std::string::npos);
    }

    FalconAuth::Shutdown();
}
