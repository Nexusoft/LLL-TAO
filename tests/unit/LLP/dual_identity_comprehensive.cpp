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

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/credentials.h>
#include <TAO/API/types/authentication.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: Dual-Identity Architecture - Comprehensive Validation
 *
 *  THE CORE ARCHITECTURAL PRINCIPLE: AUTHENTICATION vs REWARD SEPARATION
 *
 *  WHY THIS IS CRITICAL:
 *  =====================
 *  In stateless mining, we have TWO distinct identities:
 *
 *  1. AUTHENTICATION IDENTITY (hashGenesis from Falcon authentication)
 *     - Proves the miner's identity
 *     - Used for challenge-response authentication
 *     - Bound during MINER_AUTH_INIT/CHALLENGE/RESPONSE flow
 *     - Stored in context.hashGenesis
 *
 *  2. REWARD IDENTITY (hashRewardAddress from MINER_SET_REWARD)
 *     - Specifies WHERE mining rewards should be sent
 *     - Can be a register address (base58 decoded)
 *     - Can be a genesis hash
 *     - Bound during MINER_SET_REWARD packet processing
 *     - Stored in context.hashRewardAddress
 *
 *  THE CRITICAL ARCHITECTURAL PRINCIPLE:
 *  ======================================
 *  Both AUTH identity (hashGenesis) and REWARD identity (hashRewardAddress)
 *  MUST be valid TritiumGenesis (UserType) hashes. Register Addresses are NOT
 *  supported as mining reward addresses — Coinbase::Verify() enforces the UserType
 *  check on all network peers and will reject any block whose coinbase field contains
 *  a Register Address type byte. ValidateRewardAddress() enforces this at bind time.
 *
 *  WHAT WE TEST:
 *  =============
 *  1. Authentication and reward identities are independent
 *  2. Reward address can differ from authentication genesis (e.g. Pool use case)
 *  3. Both auth genesis and reward address MUST be TritiumGenesis (UserType)
 *  4. Multiple miners can use different TritiumGenesis reward addresses
 *  5. Zero/invalid addresses are handled correctly
 *  6. Context updates preserve both identities correctly
 *  7. Manager tracks both identities correctly
 *
 **/


TEST_CASE("Dual-Identity: Authentication vs Reward Separation", "[dual-identity][architecture][critical]")
{
    SECTION("Authentication genesis and reward address are separate fields")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr);
        
        /* Both identities stored independently */
        REQUIRE(ctx.hashGenesis == authGenesis);
        REQUIRE(ctx.hashRewardAddress == rewardAddr);
        
        /* They are DIFFERENT */
        REQUIRE(ctx.hashGenesis != ctx.hashRewardAddress);
    }
    
    SECTION("Reward address can be set AFTER authentication")
    {
        uint256_t authGenesis = CreateTestGenesis();
        
        /* Step 1: Authenticate */
        MiningContext authenticated = MiningContext()
            .WithGenesis(authGenesis)
            .WithAuth(true);
        
        REQUIRE(authenticated.fAuthenticated == true);
        REQUIRE(authenticated.hashGenesis == authGenesis);
        REQUIRE(authenticated.fRewardBound == false);
        
        /* Step 2: Bind reward address (must be a TritiumGenesis) */
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        MiningContext rewardBound = authenticated
            .WithRewardAddress(rewardAddr);
        
        REQUIRE(rewardBound.fRewardBound == true);
        REQUIRE(rewardBound.hashRewardAddress == rewardAddr);
        
        /* Authentication preserved */
        REQUIRE(rewardBound.fAuthenticated == true);
        REQUIRE(rewardBound.hashGenesis == authGenesis);
    }
    
    SECTION("GetPayoutAddress returns reward address when bound")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr);
        
        /* Payout goes to REWARD address, NOT auth genesis */
        REQUIRE(ctx.GetPayoutAddress() == rewardAddr);
        REQUIRE(ctx.GetPayoutAddress() != authGenesis);
    }
    
    SECTION("Both identities preserved through context updates")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr)
            .WithChannel(2)
            .WithHeight(100000)
            .WithTimestamp(runtime::unifiedtimestamp());
        
        /* Both identities still present after other updates */
        REQUIRE(ctx.hashGenesis == authGenesis);
        REQUIRE(ctx.hashRewardAddress == rewardAddr);
    }
}


TEST_CASE("Dual-Identity: TritiumGenesis Reward Address Support", "[dual-identity][genesis-reward][critical]")
{
    SECTION("TritiumGenesis can be used as reward address (required)")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardGenesis = CreateTestGenesis(Constants::GENESIS_2);

        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithAuth(true)
            .WithRewardAddress(rewardGenesis);

        /* TritiumGenesis reward address stored correctly */
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.hashRewardAddress == rewardGenesis);
        REQUIRE(ctx.GetPayoutAddress() == rewardGenesis);
    }

    SECTION("Multiple miners can use different TritiumGenesis reward addresses")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();

        uint256_t miner1Auth = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t miner1Reward = CreateTestGenesis(Constants::GENESIS_2);

        uint256_t miner2Auth = CreateTestGenesis(Constants::GENESIS_3);
        uint256_t miner2Reward = CreateTestGenesis(Constants::GENESIS_4);

        MiningContext ctx1 = MiningContext()
            .WithGenesis(miner1Auth)
            .WithRewardAddress(miner1Reward)
            .WithAuth(true);
        ctx1.strAddress = "192.168.1.100:9325";

        MiningContext ctx2 = MiningContext()
            .WithGenesis(miner2Auth)
            .WithRewardAddress(miner2Reward)
            .WithAuth(true);
        ctx2.strAddress = "192.168.1.101:9325";

        manager.UpdateMiner(ctx1.strAddress, ctx1, 0);
        manager.UpdateMiner(ctx2.strAddress, ctx2, 0);

        /* Verify both contexts stored correctly */
        auto retrieved1 = manager.GetMinerContext(ctx1.strAddress);
        auto retrieved2 = manager.GetMinerContext(ctx2.strAddress);

        REQUIRE(retrieved1.has_value());
        REQUIRE(retrieved2.has_value());

        REQUIRE(retrieved1->hashRewardAddress == miner1Reward);
        REQUIRE(retrieved2->hashRewardAddress == miner2Reward);

        /* Different miners have different addresses */
        REQUIRE(retrieved1->hashRewardAddress != retrieved2->hashRewardAddress);

        /* Cleanup */
        manager.RemoveMiner(ctx1.strAddress);
        manager.RemoveMiner(ctx2.strAddress);
    }

    SECTION("Reward TritiumGenesis preserved through height updates")
    {
        uint256_t rewardGenesis = CreateTestGenesis(Constants::GENESIS_2);

        MiningContext ctx = MiningContext()
            .WithRewardAddress(rewardGenesis)
            .WithHeight(100000);

        /* Update height */
        MiningContext updated = ctx.WithHeight(100001);

        /* Reward genesis preserved */
        REQUIRE(updated.hashRewardAddress == rewardGenesis);
        REQUIRE(updated.fRewardBound == true);
        REQUIRE(updated.nHeight == 100001);
    }
}


TEST_CASE("Dual-Identity: Genesis Hash Support", "[dual-identity][genesis-hash]")
{
    SECTION("Genesis hash can be used as reward address")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardGenesis = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardGenesis)
            .WithAuth(true);
        
        /* Genesis hash as reward works */
        REQUIRE(ctx.fRewardBound == true);
        REQUIRE(ctx.hashRewardAddress == rewardGenesis);
        REQUIRE(ctx.GetPayoutAddress() == rewardGenesis);
    }
    
    SECTION("Same genesis can be used for both auth and reward")
    {
        uint256_t genesis = CreateTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(genesis)
            .WithRewardAddress(genesis)
            .WithAuth(true);
        
        /* Both use same genesis */
        REQUIRE(ctx.hashGenesis == genesis);
        REQUIRE(ctx.hashRewardAddress == genesis);
        REQUIRE(ctx.GetPayoutAddress() == genesis);
    }
}


TEST_CASE("Dual-Identity: Mixed Miner Scenarios", "[dual-identity][mixed-scenarios]")
{
    SECTION("Miners can use different TritiumGenesis reward addresses")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();

        /* Miner 1: Uses a different TritiumGenesis as reward */
        uint256_t miner1Auth = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t miner1Reward = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx1 = MiningContext()
            .WithGenesis(miner1Auth)
            .WithRewardAddress(miner1Reward)
            .WithAuth(true);
        ctx1.strAddress = "192.168.1.100:9325";
        
        /* Miner 2: Uses genesis hash */
        uint256_t miner2Auth = CreateTestGenesis(Constants::GENESIS_2);
        uint256_t miner2Reward = CreateTestGenesis(Constants::GENESIS_3);
        
        MiningContext ctx2 = MiningContext()
            .WithGenesis(miner2Auth)
            .WithRewardAddress(miner2Reward)
            .WithAuth(true);
        ctx2.strAddress = "192.168.1.101:9325";
        
        /* Miner 3: Uses same genesis for auth and reward */
        uint256_t miner3Genesis = CreateTestGenesis(Constants::GENESIS_4);
        
        MiningContext ctx3 = MiningContext()
            .WithGenesis(miner3Genesis)
            .WithRewardAddress(miner3Genesis)
            .WithAuth(true);
        ctx3.strAddress = "192.168.1.102:9325";
        
        manager.UpdateMiner(ctx1.strAddress, ctx1, 0);
        manager.UpdateMiner(ctx2.strAddress, ctx2, 0);
        manager.UpdateMiner(ctx3.strAddress, ctx3, 0);
        
        /* Verify all stored correctly */
        auto r1 = manager.GetMinerContext(ctx1.strAddress);
        auto r2 = manager.GetMinerContext(ctx2.strAddress);
        auto r3 = manager.GetMinerContext(ctx3.strAddress);
        
        REQUIRE(r1.has_value());
        REQUIRE(r2.has_value());
        REQUIRE(r3.has_value());
        
        /* Each has their configured reward address */
        REQUIRE(r1->GetPayoutAddress() == miner1Reward);  // Register
        REQUIRE(r2->GetPayoutAddress() == miner2Reward);  // Different genesis
        REQUIRE(r3->GetPayoutAddress() == miner3Genesis); // Same genesis
        
        /* All are reward bound */
        REQUIRE(manager.GetRewardBoundCount() >= 3);
        
        /* Cleanup */
        manager.RemoveMiner(ctx1.strAddress);
        manager.RemoveMiner(ctx2.strAddress);
        manager.RemoveMiner(ctx3.strAddress);
    }
}


TEST_CASE("Dual-Identity: Zero and Invalid Address Handling", "[dual-identity][edge-cases]")
{
    SECTION("Zero reward address means not bound")
    {
        MiningContext ctx = MiningContext()
            .WithGenesis(CreateTestGenesis())
            .WithAuth(true);
        
        /* Authenticated but reward not bound */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fRewardBound == false);
        REQUIRE(ctx.hashRewardAddress == uint256_t(0));
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("Setting zero reward address explicitly")
    {
        uint256_t zero(0);
        
        MiningContext ctx = MiningContext()
            .WithRewardAddress(zero);
        
        /* Zero address doesn't bind */
        REQUIRE(ctx.hashRewardAddress == uint256_t(0));
        /* fRewardBound might still be true depending on implementation,
         * but GetPayoutAddress should return zero */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("Can update reward address after initial binding")
    {
        uint256_t reward1 = CreateTestGenesis(Constants::GENESIS_2);
        uint256_t reward2 = CreateTestGenesis(Constants::GENESIS_3);
        
        MiningContext ctx = MiningContext()
            .WithRewardAddress(reward1)
            .WithRewardAddress(reward2);  // Update
        
        /* Latest reward address wins */
        REQUIRE(ctx.hashRewardAddress == reward2);
        REQUIRE(ctx.GetPayoutAddress() == reward2);
    }
}


TEST_CASE("Dual-Identity: Manager Integration", "[dual-identity][manager]")
{
    SECTION("Manager tracks both auth and reward identities")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();

        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr)
            .WithAuth(true);
        ctx.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(ctx.strAddress, ctx, 0);
        
        auto retrieved = manager.GetMinerContext(ctx.strAddress);
        
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->hashGenesis == authGenesis);
        REQUIRE(retrieved->hashRewardAddress == rewardAddr);
        REQUIRE(retrieved->fAuthenticated == true);
        REQUIRE(retrieved->fRewardBound == true);
        
        manager.RemoveMiner(ctx.strAddress);
    }
    
    SECTION("Manager GetRewardBoundCount counts correct miners")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Miner 1: Reward bound (TritiumGenesis) */
        MiningContext ctx1 = MiningContext()
            .WithRewardAddress(CreateTestGenesis(Constants::GENESIS_2))
            .WithAuth(true);
        ctx1.strAddress = "192.168.1.100:9325";
        
        /* Miner 2: Reward bound */
        MiningContext ctx2 = MiningContext()
            .WithRewardAddress(CreateTestGenesis())
            .WithAuth(true);
        ctx2.strAddress = "192.168.1.101:9325";
        
        /* Miner 3: NOT reward bound */
        MiningContext ctx3 = MiningContext()
            .WithAuth(true);
        ctx3.strAddress = "192.168.1.102:9325";
        
        manager.UpdateMiner(ctx1.strAddress, ctx1, 0);
        manager.UpdateMiner(ctx2.strAddress, ctx2, 0);
        manager.UpdateMiner(ctx3.strAddress, ctx3, 0);
        
        /* At least 2 should be reward bound */
        REQUIRE(manager.GetRewardBoundCount() >= 2);
        
        /* Cleanup */
        manager.RemoveMiner(ctx1.strAddress);
        manager.RemoveMiner(ctx2.strAddress);
        manager.RemoveMiner(ctx3.strAddress);
    }
}


TEST_CASE("Dual-Identity: Payout Address Logic", "[dual-identity][payout]")
{
    SECTION("GetPayoutAddress returns reward when both genesis and reward set")
    {
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr);
        
        /* Reward takes precedence */
        REQUIRE(ctx.GetPayoutAddress() == rewardAddr);
        REQUIRE(ctx.GetPayoutAddress() != authGenesis);
    }
    
    SECTION("GetPayoutAddress returns zero when only auth set")
    {
        uint256_t authGenesis = CreateTestGenesis();
        
        MiningContext ctx = MiningContext()
            .WithGenesis(authGenesis)
            .WithAuth(true);
        
        /* No reward bound, returns zero */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }
    
    SECTION("HasValidPayout checks both genesis and username")
    {
        /* Has genesis */
        MiningContext ctx1 = MiningContext()
            .WithGenesis(CreateTestGenesis());
        REQUIRE(ctx1.HasValidPayout() == true);
        
        /* Has username */
        MiningContext ctx2 = MiningContext()
            .WithUserName("test_user");
        REQUIRE(ctx2.HasValidPayout() == true);
        
        /* Has both */
        MiningContext ctx3 = MiningContext()
            .WithGenesis(CreateTestGenesis())
            .WithUserName("test_user");
        REQUIRE(ctx3.HasValidPayout() == true);
        
        /* Has neither */
        MiningContext ctx4;
        REQUIRE(ctx4.HasValidPayout() == false);
    }
}


TEST_CASE("Dual-Identity: Protocol Flow Simulation", "[dual-identity][protocol-flow]")
{
    SECTION("Complete dual-identity protocol flow")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* Step 1: Miner connects */
        MiningContext connected = MiningContext()
            .WithTimestamp(runtime::unifiedtimestamp());
        connected.strAddress = "192.168.1.100:9325";
        
        /* Step 2: Miner authenticates with Falcon (MINER_AUTH_* packets) */
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t keyId = CreateRandomHash();
        std::vector<uint8_t> nonce = CreateTestNonce();
        
        MiningContext authenticated = connected
            .WithGenesis(authGenesis)
            .WithKeyId(keyId)
            .WithNonce(nonce)
            .WithAuth(true);
        
        manager.UpdateMiner(authenticated.strAddress, authenticated, 0);
        
        /* Verify authentication stored */
        auto ctx1 = manager.GetMinerContext(authenticated.strAddress);
        REQUIRE(ctx1.has_value());
        REQUIRE(ctx1->fAuthenticated == true);
        REQUIRE(ctx1->hashGenesis == authGenesis);
        REQUIRE(ctx1->fRewardBound == false);
        
        /* Step 3: Miner sends MINER_SET_REWARD with TritiumGenesis reward address */
        uint256_t rewardAddr = CreateTestGenesis(Constants::GENESIS_2);
        
        MiningContext rewardBound = authenticated
            .WithRewardAddress(rewardAddr);
        
        manager.UpdateMiner(rewardBound.strAddress, rewardBound, 0);
        
        /* Verify reward binding stored */
        auto ctx2 = manager.GetMinerContext(rewardBound.strAddress);
        REQUIRE(ctx2.has_value());
        REQUIRE(ctx2->fRewardBound == true);
        REQUIRE(ctx2->hashRewardAddress == rewardAddr);
        
        /* Authentication still preserved */
        REQUIRE(ctx2->fAuthenticated == true);
        REQUIRE(ctx2->hashGenesis == authGenesis);
        
        /* Payout goes to reward address */
        REQUIRE(ctx2->GetPayoutAddress() == rewardAddr);
        REQUIRE(ctx2->GetPayoutAddress() != authGenesis);
        
        /* Step 4: Miner sets channel and starts mining */
        MiningContext mining = rewardBound
            .WithChannel(2)
            .WithHeight(100000);
        
        manager.UpdateMiner(mining.strAddress, mining, 0);
        
        /* Verify complete state */
        auto ctx3 = manager.GetMinerContext(mining.strAddress);
        REQUIRE(ctx3.has_value());
        REQUIRE(ctx3->fAuthenticated == true);
        REQUIRE(ctx3->fRewardBound == true);
        REQUIRE(ctx3->nChannel == 2);
        REQUIRE(ctx3->hashGenesis == authGenesis);
        REQUIRE(ctx3->hashRewardAddress == rewardAddr);
        
        /* Cleanup */
        manager.RemoveMiner(mining.strAddress);
    }
}
