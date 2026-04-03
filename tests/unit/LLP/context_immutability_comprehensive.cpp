/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <helpers/test_fixtures.h>

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: MiningContext Immutability Architecture
 *
 *  COMPREHENSIVE VALIDATION of the immutable context pattern that forms
 *  the foundation of stateless mining architecture.
 *
 *  WHY THIS MATTERS:
 *  - Immutability prevents accidental state corruption in concurrent scenarios
 *  - Each context represents a snapshot of miner state at a specific moment
 *  - Context chaining enables clean state transitions without side effects
 *  - Thread-safety through immutability (no locks needed on context itself)
 *
 *  WHAT WE TEST:
 *  1. Default construction creates zero-initialized contexts
 *  2. All With* methods return NEW contexts, leaving originals unchanged
 *  3. Method chaining works correctly
 *  4. Field preservation during updates
 *  5. Copy semantics and memory safety
 *  6. Complex state transitions
 *  7. Edge cases and boundary conditions
 *
 **/


TEST_CASE("MiningContext Default Construction", "[context][immutability][architecture]")
{
    SECTION("Default constructor initializes all fields to zero/empty/false")
    {
        MiningContext ctx;
        
        /* Numeric fields are zero */
        REQUIRE(ctx.nChannel == 0);
        REQUIRE(ctx.nHeight == 0);
        REQUIRE(ctx.nTimestamp == 0);
        REQUIRE(ctx.nProtocolVersion == 0);
        REQUIRE(ctx.nSessionId == 0);
        REQUIRE(ctx.nSessionStart == 0);
        REQUIRE(ctx.nKeepaliveCount == 0);
        
        /* String fields are empty */
        REQUIRE(ctx.strAddress.empty());
        REQUIRE(ctx.strUserName.empty());
        
        /* Boolean flags are false */
        REQUIRE(ctx.fAuthenticated == false);
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.fEncryptionReady == false);
        
        /* Hash fields are zero */
        REQUIRE(ctx.hashKeyID == uint256_t(0));
        REQUIRE(ctx.hashGenesis == uint256_t(0));
        REQUIRE(ctx.hashRewardAddress == uint256_t(0));
        
        /* Vector fields are empty */
        REQUIRE(ctx.vAuthNonce.empty());
        REQUIRE(ctx.vMinerPubKey.empty());
        REQUIRE(ctx.vChaChaKey.empty());
    }
    
    SECTION("Multiple default constructions produce identical contexts")
    {
        MiningContext ctx1;
        MiningContext ctx2;
        MiningContext ctx3;
        
        /* All fields should be identical */
        REQUIRE(ctx1.nChannel == ctx2.nChannel);
        REQUIRE(ctx2.nChannel == ctx3.nChannel);
        REQUIRE(ctx1.hashGenesis == ctx2.hashGenesis);
        REQUIRE(ctx2.hashGenesis == ctx3.hashGenesis);
        REQUIRE(ctx1.fAuthenticated == ctx2.fAuthenticated);
        REQUIRE(ctx2.fAuthenticated == ctx3.fAuthenticated);
    }
}


TEST_CASE("MiningContext Immutability - Basic Fields", "[context][immutability]")
{
    SECTION("WithChannel creates new context, original unchanged")
    {
        MiningContext original;
        MiningContext updated = original.WithChannel(2);
        
        /* Original remains at default */
        REQUIRE(original.nChannel == 0);
        
        /* Updated has new value */
        REQUIRE(updated.nChannel == 2);
        
        /* All other fields preserved */
        REQUIRE(updated.nHeight == original.nHeight);
        REQUIRE(updated.fAuthenticated == original.fAuthenticated);
    }
    
    SECTION("WithHeight creates new context, original unchanged")
    {
        MiningContext original = MiningContext().WithChannel(1);
        MiningContext updated = original.WithHeight(100000);
        
        REQUIRE(original.nHeight == 0);
        REQUIRE(updated.nHeight == 100000);
        REQUIRE(updated.nChannel == 1);  // Previous value preserved
    }
    
    SECTION("WithTimestamp creates new context, original unchanged")
    {
        uint64_t testTimestamp = runtime::unifiedtimestamp();
        
        MiningContext original;
        MiningContext updated = original.WithTimestamp(testTimestamp);
        
        REQUIRE(original.nTimestamp == 0);
        REQUIRE(updated.nTimestamp == testTimestamp);
    }
    
    SECTION("WithAuth creates new context, original unchanged")
    {
        MiningContext original;
        MiningContext updated = original.WithAuth(true);
        
        REQUIRE(original.fAuthenticated == false);
        REQUIRE(updated.fAuthenticated == true);
    }
    
    SECTION("WithSession creates new context, original unchanged")
    {
        MiningContext original;
        MiningContext updated = original.WithSession(42);
        
        REQUIRE(original.nSessionId == 0);
        REQUIRE(updated.nSessionId == 42);
    }
}


TEST_CASE("MiningContext Immutability - Identity Fields", "[context][immutability][dual-identity]")
{
    SECTION("WithKeyId creates new context, original unchanged")
    {
        uint256_t testKeyId = CreateRandomHash();
        
        MiningContext original;
        MiningContext updated = original.WithKeyId(testKeyId);
        
        REQUIRE(original.hashKeyID == uint256_t(0));
        REQUIRE(updated.hashKeyID == testKeyId);
    }
    
    SECTION("WithGenesis creates new context, original unchanged")
    {
        uint256_t testGenesis = CreateTestGenesis();
        
        MiningContext original;
        MiningContext updated = original.WithGenesis(testGenesis);
        
        REQUIRE(original.hashGenesis == uint256_t(0));
        REQUIRE(updated.hashGenesis == testGenesis);
    }
    
    SECTION("WithRewardAddress creates new context, original unchanged")
    {
        uint256_t testReward = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext original;
        MiningContext updated = original.WithRewardAddress(testReward);
        
        REQUIRE(original.hashRewardAddress == uint256_t(0));
        REQUIRE(original.fRewardBound == false);
        REQUIRE(updated.hashRewardAddress == testReward);
        REQUIRE(updated.fRewardBound == true);
    }
    
    SECTION("WithUserName creates new context, original unchanged")
    {
        MiningContext original;
        MiningContext updated = original.WithUserName("test_miner");
        
        REQUIRE(original.strUserName.empty());
        REQUIRE(updated.strUserName == "test_miner");
    }
}


TEST_CASE("MiningContext Immutability - Cryptographic Fields", "[context][immutability][crypto]")
{
    SECTION("WithNonce creates new context, original unchanged")
    {
        std::vector<uint8_t> testNonce = CreateTestNonce();
        
        MiningContext original;
        MiningContext updated = original.WithNonce(testNonce);
        
        REQUIRE(original.vAuthNonce.empty());
        REQUIRE(updated.vAuthNonce == testNonce);
    }
    
    SECTION("WithPubKey creates new context, original unchanged")
    {
        std::vector<uint8_t> testPubKey = CreateTestFalconPubKey();
        
        MiningContext original;
        MiningContext updated = original.WithPubKey(testPubKey);
        
        REQUIRE(original.vMinerPubKey.empty());
        REQUIRE(updated.vMinerPubKey == testPubKey);
    }
    
    SECTION("WithChaChaKey creates new context, original unchanged")
    {
        std::vector<uint8_t> testKey = CreateTestChaChaKey();
        
        MiningContext original;
        MiningContext updated = original.WithChaChaKey(testKey);
        
        REQUIRE(original.vChaChaKey.empty());
        REQUIRE(updated.vChaChaKey == testKey);
    }
    
    SECTION("WithEncryptionReady creates new context, original unchanged")
    {
        MiningContext original;
        MiningContext updated = original.WithEncryptionReady(true);
        
        REQUIRE(original.fEncryptionReady == false);
        REQUIRE(updated.fEncryptionReady == true);
    }
}


TEST_CASE("MiningContext Method Chaining", "[context][immutability][chaining]")
{
    SECTION("Chain two updates")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithHeight(12345);
        
        REQUIRE(ctx.nChannel == 1);
        REQUIRE(ctx.nHeight == 12345);
    }
    
    SECTION("Chain five updates")
    {
        uint256_t testGenesis = CreateTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithHeight(100000)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithSession(42);
        
        REQUIRE(ctx.nChannel == 2);
        REQUIRE(ctx.nHeight == 100000);
        REQUIRE(ctx.hashGenesis == testGenesis);
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nSessionId == 42);
    }
    
    SECTION("Chain ten updates - complete initialization")
    {
        uint256_t testGenesis = CreateTestGenesis();
        uint256_t testReward = CreateTestGenesis(Constants::GENESIS_2);
        uint256_t testKeyId = CreateRandomHash();
        std::vector<uint8_t> testNonce = CreateTestNonce();
        uint64_t testTimestamp = runtime::unifiedtimestamp();
        
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithHeight(100000)
            .WithTimestamp(testTimestamp)
            .WithGenesis(testGenesis)
            .WithRewardAddress(testReward)
            .WithKeyId(testKeyId)
            .WithAuth(true)
            .WithSession(12345)
            .WithNonce(testNonce)
            .WithProtocolVersion(1);
        
        /* Verify all fields */
        REQUIRE(ctx.nChannel == 2);
        REQUIRE(ctx.nHeight == 100000);
        REQUIRE(ctx.nTimestamp == testTimestamp);
        REQUIRE(ctx.hashGenesis == testGenesis);
        REQUIRE(ctx.hashRewardAddress == testReward);
        REQUIRE(ctx.hashKeyID == testKeyId);
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nSessionId == 12345);
        REQUIRE(ctx.vAuthNonce == testNonce);
        REQUIRE(ctx.nProtocolVersion == 1);
        REQUIRE(ctx.fRewardBound == true);
    }
}


TEST_CASE("MiningContext State Transitions", "[context][immutability][transitions]")
{
    SECTION("Transition: Disconnected -> Connected")
    {
        MiningContext disconnected;
        
        MiningContext connected = disconnected
            .WithTimestamp(runtime::unifiedtimestamp());
        
        connected.strAddress = "192.168.1.100:9325";
        
        REQUIRE(connected.nTimestamp > 0);
        REQUIRE(!connected.strAddress.empty());
        REQUIRE(connected.fAuthenticated == false);
    }
    
    SECTION("Transition: Connected -> Authenticated")
    {
        uint256_t testGenesis = CreateTestGenesis();
        uint256_t testKeyId = CreateRandomHash();
        
        MiningContext connected = MiningContext()
            .WithTimestamp(runtime::unifiedtimestamp());
        connected.strAddress = "192.168.1.100:9325";
        
        MiningContext authenticated = connected
            .WithGenesis(testGenesis)
            .WithKeyId(testKeyId)
            .WithAuth(true);
        
        /* Verify transition */
        REQUIRE(authenticated.fAuthenticated == true);
        REQUIRE(authenticated.hashGenesis == testGenesis);
        REQUIRE(authenticated.hashKeyID == testKeyId);
        
        /* Previous state preserved */
        REQUIRE(authenticated.nTimestamp == connected.nTimestamp);
        REQUIRE(authenticated.strAddress == connected.strAddress);
    }
    
    SECTION("Transition: Authenticated -> Reward Bound")
    {
        MiningContext authenticated = CreateAuthenticatedContext();
        uint256_t testReward = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext rewardBound = authenticated
            .WithRewardAddress(testReward);
        
        /* Verify transition */
        REQUIRE(rewardBound.fRewardBound == true);
        REQUIRE(rewardBound.hashRewardAddress == testReward);
        
        /* Previous state preserved */
        REQUIRE(rewardBound.fAuthenticated == authenticated.fAuthenticated);
        REQUIRE(rewardBound.hashGenesis == authenticated.hashGenesis);
    }
    
    SECTION("Transition: Reward Bound -> Channel Selected")
    {
        MiningContext rewardBound = CreateRewardBoundContext();
        
        MiningContext channelSelected = rewardBound
            .WithChannel(2);
        
        /* Verify transition */
        REQUIRE(channelSelected.nChannel == 2);
        
        /* Previous state preserved */
        REQUIRE(channelSelected.fRewardBound == rewardBound.fRewardBound);
        REQUIRE(channelSelected.hashRewardAddress == rewardBound.hashRewardAddress);
    }
    
    SECTION("Complete lifecycle: Disconnected -> Mining Ready")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        uint256_t keyId = CreateRandomHash();
        std::vector<uint8_t> nonce = CreateTestNonce();
        
        MiningContext miningReady = MiningContext()
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithGenesis(authGenesis)
            .WithKeyId(keyId)
            .WithAuth(true)
            .WithNonce(nonce)
            .WithRewardAddress(rewardAddr)
            .WithChannel(2)
            .WithHeight(100000)
            .WithSession(12345)
            .WithProtocolVersion(1);
        
        /* Verify complete state */
        REQUIRE(miningReady.fAuthenticated == true);
        REQUIRE(miningReady.fRewardBound == true);
        REQUIRE(miningReady.nChannel > 0);
        REQUIRE(miningReady.nHeight > 0);
        REQUIRE(miningReady.hashGenesis != uint256_t(0));
        REQUIRE(miningReady.hashRewardAddress != uint256_t(0));
    }
}


TEST_CASE("MiningContext Copy Semantics", "[context][immutability][copy]")
{
    SECTION("Copy construction creates independent copy")
    {
        MiningContext original = CreateFullMiningContext();
        MiningContext copy(original);
        
        /* Values are equal */
        REQUIRE(copy.nChannel == original.nChannel);
        REQUIRE(copy.hashGenesis == original.hashGenesis);
        REQUIRE(copy.fAuthenticated == original.fAuthenticated);
        
        /* Modifying copy doesn't affect original */
        MiningContext modifiedCopy = copy.WithChannel(99);
        REQUIRE(original.nChannel != 99);
        REQUIRE(modifiedCopy.nChannel == 99);
    }
    
    SECTION("Copy assignment creates independent copy")
    {
        MiningContext original = CreateFullMiningContext();
        MiningContext copy;
        
        copy = original;
        
        /* Values are equal */
        REQUIRE(copy.nHeight == original.nHeight);
        REQUIRE(copy.hashRewardAddress == original.hashRewardAddress);
        
        /* Independent modification */
        MiningContext modifiedCopy = copy.WithHeight(999999);
        REQUIRE(original.nHeight != 999999);
    }
}


TEST_CASE("MiningContext Edge Cases", "[context][immutability][edge-cases]")
{
    SECTION("Setting same value multiple times")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithChannel(2)
            .WithChannel(3);
        
        /* Latest value wins */
        REQUIRE(ctx.nChannel == 3);
    }
    
    SECTION("Overwriting genesis hash")
    {
        uint256_t genesis1 = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t genesis2 = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(genesis1)
            .WithGenesis(genesis2);
        
        /* Latest value wins */
        REQUIRE(ctx.hashGenesis == genesis2);
    }
    
    SECTION("Max values for numeric fields")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(UINT32_MAX)
            .WithHeight(UINT32_MAX)
            .WithTimestamp(UINT64_MAX)
            .WithSession(UINT32_MAX);
        
        REQUIRE(ctx.nChannel == UINT32_MAX);
        REQUIRE(ctx.nHeight == UINT32_MAX);
        REQUIRE(ctx.nTimestamp == UINT64_MAX);
        REQUIRE(ctx.nSessionId == UINT32_MAX);
    }
    
    SECTION("Empty vectors after initialization")
    {
        std::vector<uint8_t> emptyVec;
        
        MiningContext ctx = MiningContext()
            .WithNonce(emptyVec)
            .WithPubKey(emptyVec)
            .WithChaChaKey(emptyVec);
        
        REQUIRE(ctx.vAuthNonce.empty());
        REQUIRE(ctx.vMinerPubKey.empty());
        REQUIRE(ctx.vChaChaKey.empty());
    }
    
    SECTION("Large vectors")
    {
        std::vector<uint8_t> largeVec(10000, 0xFF);
        
        MiningContext ctx = MiningContext()
            .WithNonce(largeVec);
        
        REQUIRE(ctx.vAuthNonce.size() == 10000);
        REQUIRE(ctx.vAuthNonce[0] == 0xFF);
    }
}


TEST_CASE("MiningContext Helper Methods", "[context][immutability][helpers]")
{
    SECTION("GetPayoutAddress returns reward address when bound")
    {
        uint256_t reward = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithRewardAddress(reward);
        
        REQUIRE(ctx.GetPayoutAddress() == reward);
    }
    
    SECTION("GetPayoutAddress returns zero when not bound")
    {
        MiningContext ctx = CreateAuthenticatedContext();
        
        /* Authenticated but no reward bound */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("HasValidPayout with genesis")
    {
        uint256_t genesis = CreateTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(genesis);
        
        REQUIRE(ctx.HasValidPayout() == true);
    }
    
    SECTION("HasValidPayout with username")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("test_user");
        
        REQUIRE(ctx.HasValidPayout() == true);
    }
    
    SECTION("HasValidPayout returns false when neither set")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.HasValidPayout() == false);
    }
}


TEST_CASE("MiningContext Builder Pattern", "[context][immutability][builder]")
{
    SECTION("Builder creates fully configured context")
    {
        MiningContext ctx = MiningContextBuilder()
            .AsFullyConfigured()
            .Build();
        
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.fEncryptionReady == true);
        REQUIRE(ctx.nChannel > 0);
        REQUIRE(ctx.nHeight > 0);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        REQUIRE(ctx.hashRewardAddress != uint256_t(0));
        REQUIRE(ctx.hashKeyID != uint256_t(0));
    }
    
    SECTION("Builder allows selective configuration")
    {
        MiningContext ctx = MiningContextBuilder()
            .WithRandomGenesis()
            .WithAuthenticated()
            .WithChannel(1)
            .Build();
        
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nChannel == 1);
        REQUIRE(ctx.hashGenesis != uint256_t(0));
        
        /* Not fully configured */
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.fEncryptionReady == false);
    }
}
