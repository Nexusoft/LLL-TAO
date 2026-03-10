/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/session_recovery.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/runtime.h>

using namespace LLP;


TEST_CASE("SessionRecoveryData Basic Tests", "[session_recovery]")
{
    SECTION("Default constructor creates empty data")
    {
        SessionRecoveryData data;
        
        REQUIRE(data.nSessionId == 0);
        REQUIRE(data.hashKeyID == uint256_t(0));
        REQUIRE(data.hashGenesis == uint256_t(0));
        REQUIRE(data.nChannel == 0);
        REQUIRE(data.fAuthenticated == false);
        REQUIRE(data.nLastLane == 0);
        REQUIRE(data.hashChaCha20Key == uint256_t(0));
        REQUIRE(data.nChaCha20Nonce == 0);
        REQUIRE(data.vDisposablePubKey.empty());
        REQUIRE(data.hashDisposableKeyID == uint256_t(0));
    }
    
    SECTION("Constructor from MiningContext preserves data")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(12345)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithDisposableKey(std::vector<uint8_t>{0xaa, 0xbb, 0xcc})
            .WithReconnectCount(3)
            .WithRewardAddress(testGenesis)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x5a))
            .WithProtocolLane(ProtocolLane::STATELESS);
        
        SessionRecoveryData data(ctx);
        
        REQUIRE(data.nSessionId == 12345);
        REQUIRE(data.hashKeyID == testKeyId);
        REQUIRE(data.hashGenesis == testGenesis);
        REQUIRE(data.nChannel == 1);
        REQUIRE(data.fAuthenticated == true);
        REQUIRE(data.vDisposablePubKey == std::vector<uint8_t>({0xaa, 0xbb, 0xcc}));
        REQUIRE(data.hashDisposableKeyID != uint256_t(0));
        REQUIRE(data.nReconnectCount == 3);
        REQUIRE(data.hashRewardAddress == testGenesis);
        REQUIRE(data.fRewardBound == true);
        REQUIRE(data.vChaCha20Key == std::vector<uint8_t>(32, 0x5a));
        REQUIRE(data.fEncryptionReady == true);
        REQUIRE(data.nProtocolLane == ProtocolLane::STATELESS);
    }

    SECTION("Shared session binding and crypto context expose canonical recovery state")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("111122223333444455556666777788889999aaaabbbbccccddddeeeeffff0000");

        uint256_t testGenesis;
        testGenesis.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");

        uint256_t reward;
        reward.SetHex("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");

        MiningContext ctx = MiningContext()
            .WithSession(424242)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithPubKey(std::vector<uint8_t>{0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04})
            .WithRewardAddress(reward)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x9a))
            .WithProtocolLane(ProtocolLane::STATELESS);

        SessionRecoveryData data(ctx);
        const SessionBinding binding = data.GetSessionBinding();
        const CryptoContext crypto = data.GetCryptoContext();

        REQUIRE(binding.nSessionId == 424242);
        REQUIRE(binding.hashKeyID == testKeyId);
        REQUIRE(binding.hashGenesis == testGenesis);
        REQUIRE(binding.hashRewardAddress == reward);
        REQUIRE(binding.fRewardBound == true);
        REQUIRE(binding.HasRewardBinding() == true);
        REQUIRE(binding.nProtocolLane == ProtocolLane::STATELESS);
        REQUIRE(binding.strKeyFingerprint == "deadbeef01020304");

        REQUIRE(crypto.nSessionId == 424242);
        REQUIRE(crypto.hashKeyID == testKeyId);
        REQUIRE(crypto.hashGenesis == testGenesis);
        REQUIRE(crypto.fEncryptionReady == true);
        REQUIRE(crypto.HasUsableKey() == true);
        REQUIRE(crypto.vChaCha20Key == std::vector<uint8_t>(32, 0x9a));
        REQUIRE(crypto.strKeyFingerprint == "9a9a9a9a9a9a9a9a");

        REQUIRE(data.ValidateConsistency() == SessionConsistencyResult::Ok);
    }

    SECTION("ValidateConsistency reports missing reward hash and crypto key invariants")
    {
        SessionRecoveryData rewardBroken;
        rewardBroken.fRewardBound = true;
        REQUIRE(rewardBroken.ValidateConsistency() == SessionConsistencyResult::RewardBoundMissingHash);

        SessionRecoveryData cryptoBroken;
        cryptoBroken.fEncryptionReady = true;
        REQUIRE(cryptoBroken.ValidateConsistency() == SessionConsistencyResult::EncryptionReadyMissingKey);
    }

    SECTION("Diagnostics key fingerprint returns stable hex prefixes")
    {
        REQUIRE(Diagnostics::KeyFingerprint(std::vector<uint8_t>()) == "NOT AVAILABLE");
        REQUIRE(Diagnostics::KeyFingerprint(std::vector<uint8_t>{0x01, 0x23, 0x45}) == "012345");
        REQUIRE(Diagnostics::KeyFingerprint(
            std::vector<uint8_t>{0xde, 0xad, 0xbe, 0xef, 0x00, 0x11, 0x22, 0x33, 0x44}) ==
            "deadbeef00112233");
    }
    
    SECTION("ToContext restores MiningContext")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        MiningContext original = MiningContext()
            .WithChannel(2)
            .WithSession(99999)
            .WithKeyId(testKeyId)
            .WithAuth(true);

        uint256_t reward;
        reward.SetHex("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
        original = original
            .WithDisposableKey(std::vector<uint8_t>{0x10, 0x20, 0x30})
            .WithReconnectCount(4)
            .WithRewardAddress(reward)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x3c))
            .WithProtocolLane(ProtocolLane::STATELESS);
        
        SessionRecoveryData data(original);
        MiningContext restored = data.ToContext();
        
        REQUIRE(restored.nChannel == 2);
        REQUIRE(restored.nSessionId == 99999);
        REQUIRE(restored.hashKeyID == testKeyId);
        REQUIRE(restored.fAuthenticated == true);
        REQUIRE(restored.vDisposablePubKey == std::vector<uint8_t>({0x10, 0x20, 0x30}));
        REQUIRE(restored.hashDisposableKeyID != uint256_t(0));
        REQUIRE(restored.nReconnectCount == 4);
        REQUIRE(restored.hashRewardAddress == reward);
        REQUIRE(restored.fRewardBound == true);
        REQUIRE(restored.vChaChaKey == std::vector<uint8_t>(32, 0x3c));
        REQUIRE(restored.fEncryptionReady == true);
        REQUIRE(restored.nProtocolLane == ProtocolLane::STATELESS);
    }
    
    SECTION("IsExpired returns true for old sessions")
    {
        SessionRecoveryData data;
        data.nLastActivity = runtime::unifiedtimestamp() - 7200;  // 2 hours ago
        
        REQUIRE(data.IsExpired(3600) == true);  // 1 hour timeout
    }
    
    SECTION("IsExpired returns false for recent sessions")
    {
        SessionRecoveryData data;
        data.nLastActivity = runtime::unifiedtimestamp() - 100;  // 100 seconds ago
        
        REQUIRE(data.IsExpired(3600) == false);  // 1 hour timeout
    }
}


TEST_CASE("SessionRecoveryManager Basic Tests", "[session_recovery]")
{
    /* Get the singleton manager */
    SessionRecoveryManager& manager = SessionRecoveryManager::Get();
    
    /* Clear any existing sessions for clean test state */
    manager.CleanupExpired(0);
    
    SECTION("SaveSession stores authenticated session")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1111111111111111111111111111111111111111111111111111111111111111");
        
        MiningContext ctx = MiningContext()
            .WithSession(11111)
            .WithKeyId(testKeyId)
            .WithAuth(true);
        
        bool result = manager.SaveSession(ctx);
        
        REQUIRE(result == true);
        REQUIRE(manager.HasSession(testKeyId) == true);
        
        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }

    SECTION("ChaCha20 and disposable key state persists")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("4444444444444444444444444444444444444444444444444444444444444444");

        uint256_t reward;
        reward.SetHex("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

        MiningContext ctx = MiningContext()
            .WithSession(55555)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithRewardAddress(reward)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x7f))
            .WithProtocolLane(ProtocolLane::STATELESS);

        manager.SaveSession(ctx);

        uint256_t chachaKey;
        chachaKey.SetHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        manager.SaveChaCha20State(testKeyId, chachaKey, 42);

        std::vector<uint8_t> disposablePubKey = {0x01, 0x02, 0x03};
        uint256_t disposableKeyId;
        disposableKeyId.SetHex("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
        manager.SaveDisposableKey(testKeyId, disposablePubKey, disposableKeyId);

        uint256_t restoredKey(0);
        uint64_t restoredNonce = 0;
        REQUIRE(manager.RestoreChaCha20State(testKeyId, restoredKey, restoredNonce) == true);
        REQUIRE(restoredKey == chachaKey);
        REQUIRE(restoredNonce == 42);

        std::vector<uint8_t> restoredPubKey;
        uint256_t restoredDisposableId(0);
        REQUIRE(manager.RestoreDisposableKey(testKeyId, restoredPubKey, restoredDisposableId) == true);
        REQUIRE(restoredPubKey == disposablePubKey);
        REQUIRE(restoredDisposableId == disposableKeyId);

        MiningContext recovered;
        REQUIRE(manager.RecoverSession(testKeyId, recovered) == true);
        REQUIRE(recovered.hashRewardAddress == reward);
        REQUIRE(recovered.fRewardBound == true);
        REQUIRE(recovered.vDisposablePubKey == disposablePubKey);
        REQUIRE(recovered.hashDisposableKeyID == disposableKeyId);
        REQUIRE(recovered.vChaChaKey == chachaKey.GetBytes());
        REQUIRE(recovered.fEncryptionReady == true);
        REQUIRE(recovered.nProtocolLane == ProtocolLane::STATELESS);

        manager.RemoveSession(testKeyId);
    }

    SECTION("SaveSession preserves authoritative reward and crypto state across partial refreshes")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("5555555555555555555555555555555555555555555555555555555555555555");

        uint256_t reward;
        reward.SetHex("dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");

        MiningContext fullContext = MiningContext()
            .WithSession(66666)
            .WithKeyId(testKeyId)
            .WithGenesis(reward)
            .WithAuth(true)
            .WithRewardAddress(reward)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x44))
            .WithProtocolLane(ProtocolLane::STATELESS);

        REQUIRE(manager.SaveSession(fullContext) == true);

        MiningContext partialRefresh = MiningContext()
            .WithSession(77777)
            .WithKeyId(testKeyId)
            .WithGenesis(reward)
            .WithAuth(true)
            .WithChannel(2);

        REQUIRE(manager.SaveSession(partialRefresh) == true);

        MiningContext recovered;
        REQUIRE(manager.RecoverSession(testKeyId, recovered) == true);
        REQUIRE(recovered.nSessionId == 77777);
        REQUIRE(recovered.nChannel == 2);
        REQUIRE(recovered.hashRewardAddress == reward);
        REQUIRE(recovered.fRewardBound == true);
        REQUIRE(recovered.vChaChaKey == std::vector<uint8_t>(32, 0x44));
        REQUIRE(recovered.fEncryptionReady == true);

        manager.RemoveSession(testKeyId);
    }

    SECTION("MergeContext keeps reconnect metadata unless explicitly refreshed")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("6666666666666666666666666666666666666666666666666666666666666666");

        SessionRecoveryData data = SessionRecoveryData(
            MiningContext()
                .WithSession(88888)
                .WithKeyId(testKeyId)
                .WithAuth(true)
                .WithReconnectCount(5)
                .WithDisposableKey(std::vector<uint8_t>{0x0a, 0x0b, 0x0c})
                .WithProtocolLane(ProtocolLane::STATELESS)
        );

        data.MergeContext(
            MiningContext()
                .WithSession(99999)
                .WithKeyId(testKeyId)
                .WithAuth(true)
                .WithChannel(2)
                .WithProtocolLane(ProtocolLane::STATELESS)
        );

        REQUIRE(data.nSessionId == 99999);
        REQUIRE(data.nReconnectCount == 5);
        REQUIRE(data.vDisposablePubKey == std::vector<uint8_t>({0x0a, 0x0b, 0x0c}));
        REQUIRE(data.nProtocolLane == ProtocolLane::STATELESS);
    }

    SECTION("MergeContext preserves authenticated recovery state across partial refreshes")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("6767676767676767676767676767676767676767676767676767676767676767");

        SessionRecoveryData data = SessionRecoveryData(
            MiningContext()
                .WithSession(12121)
                .WithKeyId(testKeyId)
                .WithAuth(true)
        );

        data.MergeContext(
            MiningContext()
                .WithSession(23232)
                .WithKeyId(testKeyId)
                .WithAuth(false)
                .WithChannel(2)
        );

        REQUIRE(data.nSessionId == 23232);
        REQUIRE(data.nChannel == 2);
        REQUIRE(data.fAuthenticated == true);
    }

    SECTION("SaveDisposableKey and UpdateLane preserve merge-managed recovery state")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("7777777777777777777777777777777777777777777777777777777777777777");

        MiningContext ctx = MiningContext()
            .WithSession(123123)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithReconnectCount(2)
            .WithProtocolLane(ProtocolLane::STATELESS);
        ctx.strAddress = "127.0.0.1";

        REQUIRE(manager.SaveSession(ctx) == true);

        std::vector<uint8_t> disposablePubKey = {0x21, 0x22, 0x23};
        uint256_t disposableKeyId;
        disposableKeyId.SetHex("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");

        REQUIRE(manager.SaveDisposableKey(testKeyId, disposablePubKey, disposableKeyId) == true);
        REQUIRE(manager.UpdateLane(testKeyId, 0) == true);

        auto optRecovered = manager.RecoverSessionByAddress("127.0.0.1");
        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->vDisposablePubKey == disposablePubKey);
        REQUIRE(optRecovered->hashDisposableKeyID == disposableKeyId);
        REQUIRE(optRecovered->nReconnectCount == 2);
        REQUIRE(optRecovered->nProtocolLane == ProtocolLane::LEGACY);
        REQUIRE(optRecovered->nLastLane == 0);

        manager.RemoveSession(testKeyId);
    }
    
    SECTION("SaveSession rejects unauthenticated session")
    {
        MiningContext ctx = MiningContext()
            .WithSession(22222)
            .WithAuth(false);  // Not authenticated
        
        bool result = manager.SaveSession(ctx);
        
        REQUIRE(result == false);
    }
    
    SECTION("RecoverSession restores session")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("2222222222222222222222222222222222222222222222222222222222222222");
        
        MiningContext original = MiningContext()
            .WithChannel(1)
            .WithSession(33333)
            .WithKeyId(testKeyId)
            .WithAuth(true);
        
        manager.SaveSession(original);
        
        MiningContext recovered;
        bool result = manager.RecoverSession(testKeyId, recovered);
        
        REQUIRE(result == true);
        REQUIRE(recovered.nSessionId == 33333);
        REQUIRE(recovered.nChannel == 1);
        REQUIRE(recovered.fAuthenticated == true);
        
        /* Cleanup */
        manager.RemoveSession(testKeyId);
    }
    
    SECTION("RecoverSession returns false for unknown key")
    {
        uint256_t unknownKey;
        unknownKey.SetHex("9999999999999999999999999999999999999999999999999999999999999999");
        
        MiningContext ctx;
        bool result = manager.RecoverSession(unknownKey, ctx);
        
        REQUIRE(result == false);
    }
    
    SECTION("RemoveSession removes existing session")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("3333333333333333333333333333333333333333333333333333333333333333");
        
        MiningContext ctx = MiningContext()
            .WithSession(44444)
            .WithKeyId(testKeyId)
            .WithAuth(true);
        
        manager.SaveSession(ctx);
        REQUIRE(manager.HasSession(testKeyId) == true);
        
        bool result = manager.RemoveSession(testKeyId);
        
        REQUIRE(result == true);
        REQUIRE(manager.HasSession(testKeyId) == false);
    }
    
    SECTION("GetSessionTimeout and SetSessionTimeout work correctly")
    {
        uint64_t originalTimeout = manager.GetSessionTimeout();
        
        manager.SetSessionTimeout(7200);
        REQUIRE(manager.GetSessionTimeout() == 7200);
        
        /* Restore original */
        manager.SetSessionTimeout(originalTimeout);
    }
    
    SECTION("GetMaxReconnects and SetMaxReconnects work correctly")
    {
        uint32_t originalMax = manager.GetMaxReconnects();
        
        manager.SetMaxReconnects(5);
        REQUIRE(manager.GetMaxReconnects() == 5);
        
        /* Restore original */
        manager.SetMaxReconnects(originalMax);
    }

    SECTION("MarkFreshAuth returns false for unknown session")
    {
        uint256_t missingKey;
        missingKey.SetHex("f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1f1");

        REQUIRE(manager.MarkFreshAuth(missingKey) == false);
    }

    SECTION("RecoverSessionByIdentity preserves fresh auth on key recovery")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("abababababababababababababababababababababababababababababababab");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(606060)
            .WithKeyId(testKeyId)
            .WithGenesis(uint256_t(606))
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctx.strAddress = "172.16.12.12";

        REQUIRE(manager.SaveSession(ctx) == true);
        REQUIRE(manager.MarkFreshAuth(testKeyId) == true);

        const auto optRecovered = manager.RecoverSessionByIdentity(testKeyId, "172.16.12.12");

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->nSessionId == 606060);
        REQUIRE(optRecovered->fFreshAuth == true);
        REQUIRE(optRecovered->nReconnectCount == 1);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity preserves fresh auth on address fallback")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("acacacacacacacacacacacacacacacacacacacacacacacacacacacacacacacac");

        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithSession(707070)
            .WithKeyId(testKeyId)
            .WithGenesis(uint256_t(707))
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctx.strAddress = "172.16.13.13";

        REQUIRE(manager.SaveSession(ctx) == true);
        REQUIRE(manager.MarkFreshAuth(testKeyId) == true);

        const auto optRecovered = manager.RecoverSessionByIdentity(uint256_t(0), "172.16.13.13");

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->nSessionId == 707070);
        REQUIRE(optRecovered->fFreshAuth == true);
        REQUIRE(optRecovered->nReconnectCount == 1);

        manager.RemoveSession(testKeyId);
    }

    SECTION("SaveSession updates address mapping when the miner address changes")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("adadadadadadadadadadadadadadadadadadadadadadadadadadadadadadadad");

        MiningContext original = MiningContext()
            .WithSession(808080)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        original.strAddress = "10.20.30.40";

        MiningContext moved = original.WithTimestamp(runtime::unifiedtimestamp());
        moved.strAddress = "10.20.30.41";

        REQUIRE(manager.SaveSession(original) == true);
        REQUIRE(manager.SaveSession(moved) == true);

        REQUIRE_FALSE(manager.RecoverSessionByAddress("10.20.30.40").has_value());

        const auto optRecovered = manager.RecoverSessionByAddress("10.20.30.41");
        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->strAddress == "10.20.30.41");

        manager.RemoveSession(testKeyId);
    }

    SECTION("UpdateSession updates address mapping when the miner address changes")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("aeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeaeae");

        MiningContext original = MiningContext()
            .WithSession(909090)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        original.strAddress = "10.20.31.40";

        MiningContext moved = original.WithChannel(2).WithTimestamp(runtime::unifiedtimestamp());
        moved.strAddress = "10.20.31.41";

        REQUIRE(manager.SaveSession(original) == true);
        REQUIRE(manager.UpdateSession(moved) == true);

        REQUIRE_FALSE(manager.RecoverSessionByAddress("10.20.31.40").has_value());

        const auto optRecovered = manager.RecoverSessionByAddress("10.20.31.41");
        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->strAddress == "10.20.31.41");
        REQUIRE(optRecovered->nChannel == 2);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity prefers falcon key over reused address mapping")
    {
        uint256_t keyA;
        keyA.SetHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        uint256_t keyB;
        keyB.SetHex("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

        MiningContext ctxA = MiningContext()
            .WithChannel(1)
            .WithSession(101010)
            .WithKeyId(keyA)
            .WithGenesis(uint256_t(111))
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctxA.strAddress = "172.16.1.25";

        MiningContext ctxB = MiningContext()
            .WithChannel(2)
            .WithSession(202020)
            .WithKeyId(keyB)
            .WithGenesis(uint256_t(222))
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctxB.strAddress = "172.16.1.25";

        REQUIRE(manager.SaveSession(ctxA) == true);
        REQUIRE(manager.SaveSession(ctxB) == true);

        const auto optRecovered = manager.RecoverSessionByIdentity(keyA, "172.16.1.25");

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == keyA);
        REQUIRE(optRecovered->nSessionId == 101010);
        REQUIRE(optRecovered->hashGenesis == uint256_t(111));

        manager.RemoveSession(keyA);
        manager.RemoveSession(keyB);
    }

    SECTION("RecoverSessionByIdentity falls back to address when key is unavailable")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(303030)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctx.strAddress = "172.16.9.9";

        REQUIRE(manager.SaveSession(ctx) == true);

        const auto optRecovered = manager.RecoverSessionByIdentity(uint256_t(0), "172.16.9.9");

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->nSessionId == 303030);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity increments reconnect count on key-based recovery")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(404040)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctx.strAddress = "172.16.10.10";

        REQUIRE(manager.SaveSession(ctx) == true);

        const auto firstRecovery = manager.RecoverSessionByIdentity(testKeyId, "172.16.10.10");
        REQUIRE(firstRecovery.has_value());
        REQUIRE(firstRecovery->nReconnectCount == 1);

        const auto secondRecovery = manager.RecoverSessionByIdentity(testKeyId, "172.16.10.10");
        REQUIRE(secondRecovery.has_value());
        REQUIRE(secondRecovery->nReconnectCount == 2);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity enforces reconnect limit")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");

        const uint32_t originalMaxReconnects = manager.GetMaxReconnects();
        manager.SetMaxReconnects(2);

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(505050)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        ctx.strAddress = "172.16.11.11";

        REQUIRE(manager.SaveSession(ctx) == true);

        REQUIRE(manager.RecoverSessionByIdentity(testKeyId, "172.16.11.11").has_value());
        REQUIRE(manager.RecoverSessionByIdentity(testKeyId, "172.16.11.11").has_value());
        REQUIRE_FALSE(manager.RecoverSessionByIdentity(testKeyId, "172.16.11.11").has_value());

        manager.SetMaxReconnects(originalMaxReconnects);
        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity resolves same Falcon identity with recovered canonical session id")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1212121212121212121212121212121212121212121212121212121212121212");

        uint256_t testGenesis;
        testGenesis.SetHex("3434343434343434343434343434343434343434343434343434343434343434");

        MiningContext recovered = MiningContext()
            .WithChannel(1)
            .WithSession(606060)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        recovered.strAddress = "172.16.12.12";

        REQUIRE(manager.SaveSession(recovered) == true);

        MiningContext live = MiningContext()
            .WithChannel(1)
            .WithSession(909090)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true);
        live.strAddress = "172.16.12.12";

        const auto optRecovered = manager.RecoverSessionByIdentity(live);

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->hashKeyID == testKeyId);
        REQUIRE(optRecovered->hashGenesis == testGenesis);
        REQUIRE(optRecovered->nSessionId == 606060);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity rejects address fallback that resolves to different Falcon identity")
    {
        uint256_t liveKeyId;
        liveKeyId.SetHex("1313131313131313131313131313131313131313131313131313131313131313");

        uint256_t recoveredKeyId;
        recoveredKeyId.SetHex("1414141414141414141414141414141414141414141414141414141414141414");

        MiningContext recovered = MiningContext()
            .WithChannel(2)
            .WithSession(707070)
            .WithKeyId(recoveredKeyId)
            .WithGenesis(uint256_t(414141))
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        recovered.strAddress = "172.16.13.13";

        REQUIRE(manager.SaveSession(recovered) == true);

        MiningContext live = MiningContext()
            .WithChannel(2)
            .WithKeyId(liveKeyId)
            .WithGenesis(uint256_t(515151))
            .WithAuth(true);
        live.strAddress = "172.16.13.13";

        REQUIRE_FALSE(manager.RecoverSessionByIdentity(live).has_value());

        manager.RemoveSession(recoveredKeyId);
    }

    SECTION("RecoverSessionByIdentity preserves live reward binding when recovered reward disagrees")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1515151515151515151515151515151515151515151515151515151515151515");

        uint256_t testGenesis;
        testGenesis.SetHex("1616161616161616161616161616161616161616161616161616161616161616");

        uint256_t recoveredReward;
        recoveredReward.SetHex("1717171717171717171717171717171717171717171717171717171717171717");

        uint256_t liveReward;
        liveReward.SetHex("1818181818181818181818181818181818181818181818181818181818181818");

        MiningContext recovered = MiningContext()
            .WithChannel(1)
            .WithSession(808080)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithRewardAddress(recoveredReward)
            .WithTimestamp(runtime::unifiedtimestamp());
        recovered.strAddress = "172.16.14.14";

        REQUIRE(manager.SaveSession(recovered) == true);

        MiningContext live = MiningContext()
            .WithChannel(1)
            .WithSession(818181)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithRewardAddress(liveReward);
        live.strAddress = "172.16.14.14";

        const auto optRecovered = manager.RecoverSessionByIdentity(live);

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->nSessionId == 808080);
        REQUIRE(optRecovered->fRewardBound == true);
        REQUIRE(optRecovered->hashRewardAddress == liveReward);

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity preserves live crypto snapshot when recovered snapshot disagrees")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1919191919191919191919191919191919191919191919191919191919191919");

        uint256_t testGenesis;
        testGenesis.SetHex("1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a");

        MiningContext recovered = MiningContext()
            .WithChannel(2)
            .WithSession(828282)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x41))
            .WithDisposableKey(std::vector<uint8_t>{0x01, 0x02, 0x03})
            .WithTimestamp(runtime::unifiedtimestamp());
        recovered.strAddress = "172.16.15.15";

        REQUIRE(manager.SaveSession(recovered) == true);

        MiningContext live = MiningContext()
            .WithChannel(2)
            .WithSession(838383)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithChaChaKey(std::vector<uint8_t>(32, 0x52))
            .WithDisposableKey(std::vector<uint8_t>{0x0a, 0x0b, 0x0c});
        live.strAddress = "172.16.15.15";

        const auto optRecovered = manager.RecoverSessionByIdentity(live);

        REQUIRE(optRecovered.has_value());
        REQUIRE(optRecovered->nSessionId == 828282);
        REQUIRE(optRecovered->fEncryptionReady == true);
        REQUIRE(optRecovered->vChaCha20Key == std::vector<uint8_t>(32, 0x52));
        REQUIRE(optRecovered->hashChaCha20Key == uint256_t(std::vector<uint8_t>(32, 0x52)));
        REQUIRE(optRecovered->vDisposablePubKey == std::vector<uint8_t>({0x0a, 0x0b, 0x0c}));

        manager.RemoveSession(testKeyId);
    }

    SECTION("RecoverSessionByIdentity rejects recovered snapshot when live genesis disagrees")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b");

        uint256_t recoveredGenesis;
        recoveredGenesis.SetHex("1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c1c");

        uint256_t liveGenesis;
        liveGenesis.SetHex("1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d");

        MiningContext recovered = MiningContext()
            .WithChannel(1)
            .WithSession(848484)
            .WithKeyId(testKeyId)
            .WithGenesis(recoveredGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());
        recovered.strAddress = "172.16.16.16";

        REQUIRE(manager.SaveSession(recovered) == true);

        MiningContext live = MiningContext()
            .WithChannel(1)
            .WithSession(858585)
            .WithKeyId(testKeyId)
            .WithGenesis(liveGenesis)
            .WithAuth(true);
        live.strAddress = "172.16.16.16";

        REQUIRE_FALSE(manager.RecoverSessionByIdentity(live).has_value());

        manager.RemoveSession(testKeyId);
    }
}


TEST_CASE("Uint256Hash Tests", "[session_recovery]")
{
    SECTION("Same values produce same hash")
    {
        uint256_t val1;
        val1.SetHex("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
        
        uint256_t val2;
        val2.SetHex("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
        
        Uint256Hash hasher;
        
        REQUIRE(hasher(val1) == hasher(val2));
    }
    
    SECTION("Different values produce different hashes")
    {
        uint256_t val1;
        val1.SetHex("1111111111111111111111111111111111111111111111111111111111111111");
        
        uint256_t val2;
        val2.SetHex("2222222222222222222222222222222222222222222222222222222222222222");
        
        Uint256Hash hasher;
        
        REQUIRE(hasher(val1) != hasher(val2));
    }
}
