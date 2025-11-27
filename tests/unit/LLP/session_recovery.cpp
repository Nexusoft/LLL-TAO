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
            .WithAuth(true);
        
        SessionRecoveryData data(ctx);
        
        REQUIRE(data.nSessionId == 12345);
        REQUIRE(data.hashKeyID == testKeyId);
        REQUIRE(data.hashGenesis == testGenesis);
        REQUIRE(data.nChannel == 1);
        REQUIRE(data.fAuthenticated == true);
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
        
        SessionRecoveryData data(original);
        MiningContext restored = data.ToContext();
        
        REQUIRE(restored.nChannel == 2);
        REQUIRE(restored.nSessionId == 99999);
        REQUIRE(restored.hashKeyID == testKeyId);
        REQUIRE(restored.fAuthenticated == true);
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
