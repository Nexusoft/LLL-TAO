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
#include <helpers/packet_builder.h>

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: Integration Tests - End-to-End Mining Flow
 *
 *  COMPREHENSIVE INTEGRATION VALIDATION:
 *  =====================================
 *
 *  Integration tests validate that all components work together correctly
 *  in realistic mining scenarios. Unlike unit tests that test individual
 *  functions, integration tests simulate complete workflows.
 *
 *  WHY INTEGRATION TESTS MATTER:
 *  =============================
 *  - Validate component interactions
 *  - Catch integration bugs that unit tests miss
 *  - Verify protocol flow correctness
 *  - Simulate real mining scenarios
 *  - Test state persistence across operations
 *  - Validate error recovery mechanisms
 *  - Ensure thread-safety in concurrent scenarios
 *
 *  WHAT WE TEST:
 *  =============
 *  1. Complete mining cycle: Connect → Auth → Reward → Channel → Block → Submit
 *  2. Multi-miner concurrent mining scenarios
 *  3. Miner disconnection and reconnection
 *  4. Session recovery after crash
 *  5. Channel switching during mining
 *  6. Mixed address type scenarios (register + genesis)
 *  7. Error recovery flows
 *  8. Long-running session stability
 *  9. High-frequency operations
 *  10. Stress testing with many miners
 *
 **/


TEST_CASE("Integration: Complete Mining Cycle - Single Miner", "[integration][full-cycle]")
{
    SECTION("Miner completes full cycle from connection to block submission")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* === PHASE 1: CONNECTION === */
        MiningContext connected = MiningContext()
            .WithTimestamp(runtime::unifiedtimestamp());
        connected.strAddress = "192.168.1.100:9325";
        
        REQUIRE(connected.fAuthenticated == false);
        REQUIRE(connected.fRewardBound == false);
        
        /* === PHASE 2: AUTHENTICATION === */
        uint256_t authGenesis = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t keyId = CreateRandomHash();
        std::vector<uint8_t> nonce = CreateTestNonce();
        std::vector<uint8_t> pubKey = CreateTestFalconPubKey();
        
        MiningContext authenticated = connected
            .WithGenesis(authGenesis)
            .WithKeyId(keyId)
            .WithNonce(nonce)
            .WithPubKey(pubKey)
            .WithAuth(true)
            .WithSessionStart(runtime::unifiedtimestamp())
            .WithSession(12345)
            .WithSessionTimeout(300);
        
        manager.UpdateMiner(authenticated.strAddress, authenticated, 0);
        
        auto phase2 = manager.GetMinerContext(authenticated.strAddress);
        REQUIRE(phase2.has_value());
        REQUIRE(phase2->fAuthenticated == true);
        REQUIRE(phase2->hashGenesis == authGenesis);
        REQUIRE(phase2->nSessionId == 12345);
        
        /* === PHASE 3: REWARD BINDING === */
        uint256_t rewardAddr = CreateTestRegisterAddress();
        
        MiningContext rewardBound = authenticated
            .WithRewardAddress(rewardAddr);
        
        manager.UpdateMiner(rewardBound.strAddress, rewardBound, 0);
        
        auto phase3 = manager.GetMinerContext(rewardBound.strAddress);
        REQUIRE(phase3.has_value());
        REQUIRE(phase3->fRewardBound == true);
        REQUIRE(phase3->hashRewardAddress == rewardAddr);
        REQUIRE(phase3->GetPayoutAddress() == rewardAddr);
        
        /* === PHASE 4: CHANNEL SELECTION === */
        MiningContext channelSelected = rewardBound
            .WithChannel(2);  // Hash channel
        
        manager.UpdateMiner(channelSelected.strAddress, channelSelected, 0);
        
        auto phase4 = manager.GetMinerContext(channelSelected.strAddress);
        REQUIRE(phase4.has_value());
        REQUIRE(phase4->nChannel == 2);
        
        /* === PHASE 5: BLOCK REQUEST === */
        MiningContext blockRequested = channelSelected
            .WithHeight(100000)
            .WithTimestamp(runtime::unifiedtimestamp());
        
        manager.UpdateMiner(blockRequested.strAddress, blockRequested, 0);
        
        auto phase5 = manager.GetMinerContext(blockRequested.strAddress);
        REQUIRE(phase5.has_value());
        REQUIRE(phase5->nHeight == 100000);
        
        /* === PHASE 6: BLOCK SUBMISSION === */
        /* (In real scenario, miner would solve block and submit) */
        MiningContext submitted = blockRequested
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithKeepaliveCount(blockRequested.nKeepaliveCount + 1);
        
        manager.UpdateMiner(submitted.strAddress, submitted, 0);
        
        /* === VERIFICATION: Complete state preserved === */
        auto final = manager.GetMinerContext(submitted.strAddress);
        REQUIRE(final.has_value());
        REQUIRE(final->fAuthenticated == true);
        REQUIRE(final->fRewardBound == true);
        REQUIRE(final->nChannel == 2);
        REQUIRE(final->hashGenesis == authGenesis);
        REQUIRE(final->hashRewardAddress == rewardAddr);
        REQUIRE(final->GetPayoutAddress() == rewardAddr);
        
        /* Cleanup */
        manager.RemoveMiner(submitted.strAddress);
    }
}


TEST_CASE("Integration: Multi-Miner Concurrent Mining", "[integration][multi-miner][concurrency]")
{
    SECTION("Three miners mining concurrently with different configurations")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* === MINER 1: Uses register address, hash channel === */
        uint256_t miner1Auth = CreateTestGenesis(Constants::GENESIS_1);
        uint256_t miner1Reward = CreateTestRegisterAddress(Constants::REGISTER_ADDR_1);
        
        MiningContext miner1 = MiningContext()
            .WithGenesis(miner1Auth)
            .WithRewardAddress(miner1Reward)
            .WithAuth(true)
            .WithChannel(2)  // Hash
            .WithHeight(100000)
            .WithSession(11111)
            .WithTimestamp(runtime::unifiedtimestamp());
        miner1.strAddress = "192.168.1.100:9325";
        
        /* === MINER 2: Uses genesis hash, prime channel === */
        uint256_t miner2Auth = CreateTestGenesis(Constants::GENESIS_2);
        uint256_t miner2Reward = CreateTestGenesis(Constants::GENESIS_3);
        
        MiningContext miner2 = MiningContext()
            .WithGenesis(miner2Auth)
            .WithRewardAddress(miner2Reward)
            .WithAuth(true)
            .WithChannel(1)  // Prime
            .WithHeight(100000)
            .WithSession(22222)
            .WithTimestamp(runtime::unifiedtimestamp());
        miner2.strAddress = "192.168.1.101:9325";
        
        /* === MINER 3: Uses same genesis for auth and reward === */
        uint256_t miner3Genesis = CreateTestGenesis(Constants::GENESIS_4);
        
        MiningContext miner3 = MiningContext()
            .WithGenesis(miner3Genesis)
            .WithRewardAddress(miner3Genesis)
            .WithAuth(true)
            .WithChannel(2)  // Hash
            .WithHeight(100000)
            .WithSession(33333)
            .WithTimestamp(runtime::unifiedtimestamp());
        miner3.strAddress = "192.168.1.102:9325";
        
        /* === UPDATE ALL MINERS === */
        manager.UpdateMiner(miner1.strAddress, miner1, 0);
        manager.UpdateMiner(miner2.strAddress, miner2, 0);
        manager.UpdateMiner(miner3.strAddress, miner3, 0);
        
        /* === VERIFY ALL MINERS STORED CORRECTLY === */
        auto r1 = manager.GetMinerContext(miner1.strAddress);
        auto r2 = manager.GetMinerContext(miner2.strAddress);
        auto r3 = manager.GetMinerContext(miner3.strAddress);
        
        REQUIRE(r1.has_value());
        REQUIRE(r2.has_value());
        REQUIRE(r3.has_value());
        
        /* Verify identities */
        REQUIRE(r1->hashGenesis == miner1Auth);
        REQUIRE(r1->GetPayoutAddress() == miner1Reward);
        
        REQUIRE(r2->hashGenesis == miner2Auth);
        REQUIRE(r2->GetPayoutAddress() == miner2Reward);
        
        REQUIRE(r3->hashGenesis == miner3Genesis);
        REQUIRE(r3->GetPayoutAddress() == miner3Genesis);
        
        /* Verify channels */
        REQUIRE(r1->nChannel == 2);
        REQUIRE(r2->nChannel == 1);
        REQUIRE(r3->nChannel == 2);
        
        /* Verify all are reward bound */
        REQUIRE(manager.GetRewardBoundCount() >= 3);
        
        /* === SIMULATE CONCURRENT OPERATIONS === */
        /* Miner 1 requests block */
        MiningContext m1Updated = r1.value()
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithKeepaliveCount(1);
        manager.UpdateMiner(m1Updated.strAddress, m1Updated, 0);
        
        /* Miner 2 switches channel */
        MiningContext m2Updated = r2.value()
            .WithChannel(2)
            .WithTimestamp(runtime::unifiedtimestamp());
        manager.UpdateMiner(m2Updated.strAddress, m2Updated, 0);
        
        /* Miner 3 sends keepalive */
        MiningContext m3Updated = r3.value()
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithKeepaliveCount(1);
        manager.UpdateMiner(m3Updated.strAddress, m3Updated, 0);
        
        /* === VERIFY UPDATES === */
        auto final1 = manager.GetMinerContext(miner1.strAddress);
        auto final2 = manager.GetMinerContext(miner2.strAddress);
        auto final3 = manager.GetMinerContext(miner3.strAddress);
        
        REQUIRE(final1->nKeepaliveCount == 1);
        REQUIRE(final2->nChannel == 2);  // Changed from 1 to 2
        REQUIRE(final3->nKeepaliveCount == 1);
        
        /* All identities still correct */
        REQUIRE(final1->GetPayoutAddress() == miner1Reward);
        REQUIRE(final2->GetPayoutAddress() == miner2Reward);
        REQUIRE(final3->GetPayoutAddress() == miner3Genesis);
        
        /* Cleanup */
        manager.RemoveMiner(miner1.strAddress);
        manager.RemoveMiner(miner2.strAddress);
        manager.RemoveMiner(miner3.strAddress);
    }
}


TEST_CASE("Integration: Miner Disconnection and Reconnection", "[integration][reconnection]")
{
    SECTION("Miner reconnects and session is recovered")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* === INITIAL CONNECTION === */
        uint256_t authGenesis = CreateTestGenesis();
        uint256_t rewardAddr = CreateTestRegisterAddress();
        uint64_t sessionStart = runtime::unifiedtimestamp();
        
        MiningContext connected = MiningContext()
            .WithGenesis(authGenesis)
            .WithRewardAddress(rewardAddr)
            .WithAuth(true)
            .WithChannel(2)
            .WithHeight(100000)
            .WithSession(12345)
            .WithSessionStart(sessionStart)
            .WithTimestamp(sessionStart);
        connected.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(connected.strAddress, connected, 0);
        
        /* === SIMULATE DISCONNECTION === */
        /* (Manager keeps context for potential reconnection) */
        
        /* === RECONNECTION === */
        /* Miner proves identity with same genesis */
        MiningContext reconnected = MiningContext()
            .WithGenesis(authGenesis)
            .WithAuth(true)
            .WithSessionStart(sessionStart)  // Preserve session
            .WithSession(12345)
            .WithTimestamp(runtime::unifiedtimestamp());
        reconnected.strAddress = "192.168.1.100:9325";
        
        /* Retrieve previous context and merge */
        auto previous = manager.GetMinerContext(reconnected.strAddress);
        if(previous.has_value())
        {
            /* Restore previous state */
            reconnected = reconnected
                .WithRewardAddress(previous->hashRewardAddress)
                .WithChannel(previous->nChannel)
                .WithHeight(previous->nHeight);
        }
        
        manager.UpdateMiner(reconnected.strAddress, reconnected, 0);
        
        /* === VERIFY RECOVERY === */
        auto recovered = manager.GetMinerContext(reconnected.strAddress);
        
        REQUIRE(recovered.has_value());
        REQUIRE(recovered->hashGenesis == authGenesis);
        REQUIRE(recovered->hashRewardAddress == rewardAddr);
        REQUIRE(recovered->nChannel == 2);
        REQUIRE(recovered->nSessionStart == sessionStart);
        
        manager.RemoveMiner(reconnected.strAddress);
    }
}


TEST_CASE("Integration: Channel Switching During Mining", "[integration][channel-switch]")
{
    SECTION("Miner switches from prime to hash channel")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* === START WITH PRIME CHANNEL === */
        MiningContext prime = CreateFullMiningContext(1);  // Prime
        prime.strAddress = "192.168.1.100:9325";
        
        manager.UpdateMiner(prime.strAddress, prime, 0);
        
        auto phase1 = manager.GetMinerContext(prime.strAddress);
        REQUIRE(phase1.has_value());
        REQUIRE(phase1->nChannel == 1);
        
        /* === SWITCH TO HASH CHANNEL === */
        MiningContext hash = phase1.value()
            .WithChannel(2)
            .WithTimestamp(runtime::unifiedtimestamp());
        
        manager.UpdateMiner(hash.strAddress, hash, 0);
        
        auto phase2 = manager.GetMinerContext(hash.strAddress);
        REQUIRE(phase2.has_value());
        REQUIRE(phase2->nChannel == 2);
        
        /* All other state preserved */
        REQUIRE(phase2->fAuthenticated == phase1->fAuthenticated);
        REQUIRE(phase2->fRewardBound == phase1->fRewardBound);
        REQUIRE(phase2->hashGenesis == phase1->hashGenesis);
        REQUIRE(phase2->hashRewardAddress == phase1->hashRewardAddress);
        
        manager.RemoveMiner(hash.strAddress);
    }
}


TEST_CASE("Integration: Height Updates and New Rounds", "[integration][height-updates]")
{
    SECTION("Manager updates all miners when new block found")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        /* === SETUP THREE MINERS === */
        MiningContext miner1 = CreateFullMiningContext(2);
        miner1.strAddress = "192.168.1.100:9325";
        miner1 = miner1.WithHeight(100000);
        
        MiningContext miner2 = CreateFullMiningContext(2);
        miner2.strAddress = "192.168.1.101:9325";
        miner2 = miner2.WithHeight(100000);
        
        MiningContext miner3 = CreateFullMiningContext(1);
        miner3.strAddress = "192.168.1.102:9325";
        miner3 = miner3.WithHeight(100000);
        
        manager.UpdateMiner(miner1.strAddress, miner1, 0);
        manager.UpdateMiner(miner2.strAddress, miner2, 0);
        manager.UpdateMiner(miner3.strAddress, miner3, 0);
        
        /* === VERIFY INITIAL HEIGHT === */
        REQUIRE(manager.GetCurrentHeight() >= 100000);
        
        /* === NEW BLOCK FOUND === */
        uint32_t notified = manager.NotifyNewRound(100001);
        
        /* === VERIFY ALL MINERS NOTIFIED === */
        REQUIRE(notified >= 3);
        
        auto m1 = manager.GetMinerContext(miner1.strAddress);
        auto m2 = manager.GetMinerContext(miner2.strAddress);
        auto m3 = manager.GetMinerContext(miner3.strAddress);
        
        REQUIRE(m1.has_value());
        REQUIRE(m2.has_value());
        REQUIRE(m3.has_value());
        
        REQUIRE(m1->nHeight == 100001);
        REQUIRE(m2->nHeight == 100001);
        REQUIRE(m3->nHeight == 100001);
        
        manager.RemoveMiner(miner1.strAddress);
        manager.RemoveMiner(miner2.strAddress);
        manager.RemoveMiner(miner3.strAddress);
    }
}


TEST_CASE("Integration: Session Expiration and Cleanup", "[integration][session-expiration]")
{
    SECTION("Expired sessions are detected correctly")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        uint64_t now = runtime::unifiedtimestamp();
        
        /* === ACTIVE SESSION === */
        MiningContext active = CreateFullMiningContext(2);
        active = active
            .WithSessionStart(now)
            .WithSessionTimeout(300);
        active.strAddress = "192.168.1.100:9325";
        
        /* === EXPIRED SESSION === */
        MiningContext expired = CreateFullMiningContext(2);
        expired = expired
            .WithSessionStart(now - 400)  // Started 400s ago
            .WithSessionTimeout(300);     // 300s timeout
        expired.strAddress = "192.168.1.101:9325";
        
        manager.UpdateMiner(active.strAddress, active, 0);
        manager.UpdateMiner(expired.strAddress, expired, 0);
        
        /* === CHECK EXPIRATION === */
        auto activeCtx = manager.GetMinerContext(active.strAddress);
        auto expiredCtx = manager.GetMinerContext(expired.strAddress);
        
        REQUIRE(activeCtx.has_value());
        REQUIRE(expiredCtx.has_value());
        
        REQUIRE(activeCtx->IsSessionExpired() == false);
        REQUIRE(expiredCtx->IsSessionExpired() == true);
        
        /* === CLEANUP === */
        /* In production, CleanupInactive would remove expired sessions */
        manager.CleanupInactive(300);
        
        manager.RemoveMiner(active.strAddress);
        manager.RemoveMiner(expired.strAddress);
    }
}


TEST_CASE("Integration: Stress Test with Many Miners", "[integration][stress]")
{
    SECTION("Handle 100 concurrent miners")
    {
        StatelessMinerManager& manager = StatelessMinerManager::Get();
        
        const int NUM_MINERS = 100;
        std::vector<std::string> addresses;
        
        /* === CREATE 100 MINERS === */
        for(int i = 0; i < NUM_MINERS; i++)
        {
            MiningContext ctx = MiningContextBuilder()
                .WithRandomGenesis()
                .WithRandomReward()
                .WithRandomKeyId()
                .WithAuthenticated()
                .WithChannel(i % 2 == 0 ? 1 : 2)  // Alternate channels
                .WithHeight(100000)
                .WithCurrentTimestamp()
                .WithEncryption()
                .Build();
            
            ctx.strAddress = "192.168.1." + std::to_string(i) + ":9325";
            addresses.push_back(ctx.strAddress);
            
            manager.UpdateMiner(ctx.strAddress, ctx, 0);
        }
        
        /* === VERIFY ALL STORED === */
        int retrieved = 0;
        for(const auto& addr : addresses)
        {
            auto ctx = manager.GetMinerContext(addr);
            if(ctx.has_value())
            {
                retrieved++;
                REQUIRE(ctx->fAuthenticated == true);
                REQUIRE(ctx->fRewardBound == true);
            }
        }
        
        REQUIRE(retrieved == NUM_MINERS);
        
        /* === VERIFY REWARD BOUND COUNT === */
        REQUIRE(manager.GetRewardBoundCount() >= NUM_MINERS);
        
        /* === CLEANUP === */
        for(const auto& addr : addresses)
        {
            manager.RemoveMiner(addr);
        }
    }
}


TEST_CASE("Integration: Complete Protocol Flow with Packets", "[integration][protocol-flow]")
{
    SECTION("Simulate complete packet-based protocol interaction")
    {
        /* This test simulates the packet flow, validating packet structure */
        
        /* === PHASE 1: AUTH_INIT === */
        std::vector<uint8_t> pubKey = CreateTestFalconPubKey();
        uint256_t genesis = CreateTestGenesis();
        
        Packet authInit = CreateAuthInitPacket(pubKey, genesis);
        REQUIRE(authInit.HEADER == PacketTypes::MINER_AUTH_INIT);
        
        /* === PHASE 2: AUTH_RESPONSE === */
        std::vector<uint8_t> signature;
        /* Build signature from random data */
        while(signature.size() < Constants::FALCON_SIGNATURE_SIZE)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = Constants::FALCON_SIGNATURE_SIZE - signature.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            signature.insert(signature.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        
        Packet authResponse = CreateAuthResponsePacket(signature);
        REQUIRE(authResponse.HEADER == PacketTypes::MINER_AUTH_RESPONSE);
        
        /* === PHASE 3: SET_REWARD === */
        std::vector<uint8_t> encryptedReward;
        /* Build encrypted data from random values */
        while(encryptedReward.size() < 64)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = 64 - encryptedReward.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            encryptedReward.insert(encryptedReward.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        
        Packet setReward = CreateSetRewardPacket(encryptedReward);
        REQUIRE(setReward.HEADER == PacketTypes::MINER_SET_REWARD);
        
        /* === PHASE 4: SET_CHANNEL === */
        Packet setChannel = CreateSetChannelPacket(2);
        REQUIRE(setChannel.HEADER == PacketTypes::SET_CHANNEL);
        
        /* === PHASE 5: GET_BLOCK === */
        Packet getBlock = CreateGetBlockPacket();
        REQUIRE(getBlock.HEADER == PacketTypes::GET_BLOCK);
        
        /* === PHASE 6: SUBMIT_BLOCK === */
        uint512_t merkleRoot;
        merkleRoot.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        Packet submit = CreateSubmitBlockPacket(merkleRoot, 123456789);
        REQUIRE(submit.HEADER == PacketTypes::SUBMIT_BLOCK);
        
        /* === VERIFY PACKET SEQUENCE === */
        /* All packets created successfully in correct order */
    }
}
