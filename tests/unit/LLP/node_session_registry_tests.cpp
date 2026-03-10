/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLP/include/node_session_registry.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/runtime.h>

#include <unit/catch2/catch.hpp>

using namespace LLP;

TEST_CASE("NodeSessionRegistry - Basic Registration", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Create test context */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    /* Register new session on stateless lane */
    auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
    );

    REQUIRE(isNew == true);
    REQUIRE(sessionId != 0);
    REQUIRE(NodeSessionRegistry::Get().Count() == 1);

    /* Verify session can be looked up by ID */
    auto entry = NodeSessionRegistry::Get().Lookup(sessionId);
    REQUIRE(entry.has_value());
    REQUIRE(entry->nSessionId == sessionId);
    REQUIRE(entry->hashKeyID == hashKeyID);
    REQUIRE(entry->hashGenesis == hashGenesis);
    REQUIRE(entry->fStatelessLive == true);
    REQUIRE(entry->fLegacyLive == false);
    REQUIRE(entry->AnyPortLive() == true);
}

TEST_CASE("NodeSessionRegistry - Cross-Port Session Recovery", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Create test identity */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();

    /* First auth on stateless port (9323) */
    MiningContext context1;
    context1 = context1.WithKeyId(hashKeyID);
    context1 = context1.WithGenesis(hashGenesis);
    context1 = context1.WithAuth(true);
    context1 = context1.WithChannel(1);  // Prime channel

    auto [sessionId1, isNew1] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context1, ProtocolLane::STATELESS
    );

    REQUIRE(isNew1 == true);
    REQUIRE(sessionId1 != 0);

    /* Second auth on legacy port (8323) - SAME identity */
    MiningContext context2;
    context2 = context2.WithKeyId(hashKeyID);
    context2 = context2.WithGenesis(hashGenesis);
    context2 = context2.WithAuth(true);
    context2 = context2.WithChannel(2);  // Hash channel (different from first)

    auto [sessionId2, isNew2] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context2, ProtocolLane::LEGACY
    );

    /* CRITICAL ASSERTION: Session ID must be the same! */
    REQUIRE(isNew2 == false);
    REQUIRE(sessionId2 == sessionId1);  // Same session ID across ports
    REQUIRE(NodeSessionRegistry::Get().Count() == 1);  // Only one entry

    /* Verify both ports are marked live */
    auto entry = NodeSessionRegistry::Get().Lookup(sessionId1);
    REQUIRE(entry.has_value());
    REQUIRE(entry->fStatelessLive == true);
    REQUIRE(entry->fLegacyLive == true);
    REQUIRE(entry->AnyPortLive() == true);
}

TEST_CASE("NodeSessionRegistry - Session ID Derivation", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Create test identity */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    /* Register session */
    auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
    );

    /* Verify session ID is derived from lower 32 bits of hashKeyID */
    uint32_t expectedSessionId = static_cast<uint32_t>(hashKeyID.Get64(0));
    REQUIRE(sessionId == expectedSessionId);
}

TEST_CASE("NodeSessionRegistry - Lookup by Session ID", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register multiple sessions */
    std::vector<uint32_t> sessionIds;
    for(int i = 0; i < 5; ++i)
    {
        uint256_t hashKeyID = LLC::GetRand256();
        uint256_t hashGenesis = LLC::GetRand256();
        MiningContext context;
        context = context.WithKeyId(hashKeyID);
        context = context.WithGenesis(hashGenesis);
        context = context.WithAuth(true);

        auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
            hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
        );
        sessionIds.push_back(sessionId);
    }

    REQUIRE(NodeSessionRegistry::Get().Count() == 5);

    /* Verify each session can be looked up */
    for(uint32_t sessionId : sessionIds)
    {
        auto entry = NodeSessionRegistry::Get().Lookup(sessionId);
        REQUIRE(entry.has_value());
        REQUIRE(entry->nSessionId == sessionId);
    }
}

TEST_CASE("NodeSessionRegistry - Lookup by Key", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register session */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
    );

    /* Lookup by key */
    auto entry = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry.has_value());
    REQUIRE(entry->hashKeyID == hashKeyID);
    REQUIRE(entry->nSessionId == sessionId);
}

TEST_CASE("NodeSessionRegistry - Update Context", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register session */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);
    context = context.WithChannel(1);  // Start with Prime

    auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
    );

    /* Update context with new channel */
    MiningContext newContext = context.WithChannel(2);  // Switch to Hash
    bool updated = NodeSessionRegistry::Get().UpdateContext(hashKeyID, newContext);
    REQUIRE(updated == true);

    /* Verify update */
    auto entry = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry.has_value());
    REQUIRE(entry->context.nChannel == 2);
}

TEST_CASE("NodeSessionRegistry - Mark Disconnected", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register session on both ports */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    /* Auth on both ports */
    NodeSessionRegistry::Get().RegisterOrRefresh(hashKeyID, hashGenesis, context, ProtocolLane::STATELESS);
    NodeSessionRegistry::Get().RegisterOrRefresh(hashKeyID, hashGenesis, context, ProtocolLane::LEGACY);

    /* Verify both are live */
    auto entry1 = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry1->fStatelessLive == true);
    REQUIRE(entry1->fLegacyLive == true);

    /* Disconnect stateless port */
    bool marked = NodeSessionRegistry::Get().MarkDisconnected(hashKeyID, ProtocolLane::STATELESS);
    REQUIRE(marked == true);

    /* Verify stateless is dead, legacy still alive */
    auto entry2 = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry2->fStatelessLive == false);
    REQUIRE(entry2->fLegacyLive == true);
    REQUIRE(entry2->AnyPortLive() == true);

    /* Disconnect legacy port */
    NodeSessionRegistry::Get().MarkDisconnected(hashKeyID, ProtocolLane::LEGACY);

    /* Verify both are dead */
    auto entry3 = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry3->fStatelessLive == false);
    REQUIRE(entry3->fLegacyLive == false);
    REQUIRE(entry3->AnyPortLive() == false);
}

TEST_CASE("NodeSessionRegistry - Sweep Expired", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register session with no live connections */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    auto [sessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
        hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
    );

    /* Mark disconnected */
    NodeSessionRegistry::Get().MarkDisconnected(hashKeyID, ProtocolLane::STATELESS);

    /* Verify session exists but is inactive */
    auto entry = NodeSessionRegistry::Get().LookupByKey(hashKeyID);
    REQUIRE(entry.has_value());
    REQUIRE(entry->AnyPortLive() == false);

    /* Sweep with short timeout should remove it */
    uint32_t removed = NodeSessionRegistry::Get().SweepExpired(0);  // 0 second timeout
    REQUIRE(removed == 1);
    REQUIRE(NodeSessionRegistry::Get().Count() == 0);
}

TEST_CASE("NodeSessionRegistry - Live Sessions Not Swept", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register live session */
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    context = context.WithKeyId(hashKeyID);
    context = context.WithGenesis(hashGenesis);
    context = context.WithAuth(true);

    NodeSessionRegistry::Get().RegisterOrRefresh(hashKeyID, hashGenesis, context, ProtocolLane::STATELESS);

    /* Session is still live, should not be swept */
    uint32_t removed = NodeSessionRegistry::Get().SweepExpired(0);  // Even with 0 timeout
    REQUIRE(removed == 0);
    REQUIRE(NodeSessionRegistry::Get().Count() == 1);
}

TEST_CASE("NodeSessionRegistry - Count and CountLive", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Register 3 sessions */
    std::vector<uint256_t> keys;
    for(int i = 0; i < 3; ++i)
    {
        uint256_t hashKeyID = LLC::GetRand256();
        uint256_t hashGenesis = LLC::GetRand256();
        MiningContext context;
        context = context.WithKeyId(hashKeyID);
        context = context.WithGenesis(hashGenesis);
        context = context.WithAuth(true);

        NodeSessionRegistry::Get().RegisterOrRefresh(hashKeyID, hashGenesis, context, ProtocolLane::STATELESS);
        keys.push_back(hashKeyID);
    }

    REQUIRE(NodeSessionRegistry::Get().Count() == 3);
    REQUIRE(NodeSessionRegistry::Get().CountLive() == 3);

    /* Disconnect one */
    NodeSessionRegistry::Get().MarkDisconnected(keys[0], ProtocolLane::STATELESS);

    REQUIRE(NodeSessionRegistry::Get().Count() == 3);  // Still exists
    REQUIRE(NodeSessionRegistry::Get().CountLive() == 2);  // But not live
}

TEST_CASE("NodeSessionRegistry - Entry IsExpired", "[llp]")
{
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    MiningContext context;
    uint64_t nNow = runtime::unifiedtimestamp();

    /* Create entry with current activity */
    NodeSessionEntry entry(
        12345,
        hashKeyID,
        hashGenesis,
        true,   // stateless live
        false,  // legacy not live
        nNow,   // last activity now
        context
    );

    /* Not expired if any port is live */
    REQUIRE(entry.IsExpired(300, nNow) == false);

    /* Mark both ports dead */
    entry = entry.WithStatelessLive(false);

    /* Not expired if activity is recent */
    REQUIRE(entry.IsExpired(300, nNow) == false);

    /* Expired if activity is old and no ports live */
    REQUIRE(entry.IsExpired(0, nNow + 1) == true);
}

TEST_CASE("NodeSessionRegistry - Shared binding and consistency helpers", "[llp]")
{
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    uint256_t hashReward = LLC::GetRand256();

    MiningContext context = MiningContext()
        .WithSession(55555)
        .WithKeyId(hashKeyID)
        .WithGenesis(hashGenesis)
        .WithAuth(true)
        .WithPubKey(std::vector<uint8_t>{0xaa, 0xbb, 0xcc, 0xdd, 0x10, 0x20, 0x30, 0x40})
        .WithRewardAddress(hashReward)
        .WithChaChaKey(std::vector<uint8_t>(32, 0x4c))
        .WithProtocolLane(ProtocolLane::STATELESS);

    NodeSessionEntry entry(
        55555,
        hashKeyID,
        hashGenesis,
        true,
        false,
        runtime::unifiedtimestamp(),
        context
    );

    const SessionBinding binding = entry.GetSessionBinding();
    const CryptoContext crypto = entry.GetCryptoContext();

    REQUIRE(binding.nSessionId == 55555);
    REQUIRE(binding.hashKeyID == hashKeyID);
    REQUIRE(binding.hashGenesis == hashGenesis);
    REQUIRE(binding.hashRewardAddress == hashReward);
    REQUIRE(binding.nProtocolLane == ProtocolLane::STATELESS);
    REQUIRE(binding.strKeyFingerprint == "aabbccdd10203040");
    REQUIRE(entry.ValidateConsistency() == SessionConsistencyResult::Ok);

    REQUIRE(crypto.nSessionId == 55555);
    REQUIRE(crypto.hashKeyID == hashKeyID);
    REQUIRE(crypto.hashGenesis == hashGenesis);
    REQUIRE(crypto.fEncryptionReady == true);
    REQUIRE(crypto.HasUsableKey() == true);
    REQUIRE(crypto.strKeyFingerprint == "4c4c4c4c4c4c4c4c");
}

TEST_CASE("NodeSessionRegistry - ValidateConsistency catches identity mismatch", "[llp]")
{
    uint256_t hashKeyID = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();

    MiningContext mismatchedContext = MiningContext()
        .WithSession(99999)
        .WithKeyId(hashKeyID)
        .WithGenesis(hashGenesis)
        .WithAuth(true);

    NodeSessionEntry entry(
        12345,
        hashKeyID,
        hashGenesis,
        true,
        false,
        runtime::unifiedtimestamp(),
        mismatchedContext
    );

    REQUIRE(entry.ValidateConsistency() == SessionConsistencyResult::SessionIdMismatch);

    MiningContext genesisMismatchContext = MiningContext()
        .WithSession(12345)
        .WithKeyId(hashKeyID)
        .WithGenesis(LLC::GetRand256())
        .WithAuth(true);

    NodeSessionEntry genesisMismatchEntry(
        12345,
        hashKeyID,
        hashGenesis,
        true,
        false,
        runtime::unifiedtimestamp(),
        genesisMismatchContext
    );

    REQUIRE(genesisMismatchEntry.ValidateConsistency() == SessionConsistencyResult::GenesisMismatch);

    MiningContext keyMismatchContext = MiningContext()
        .WithSession(12345)
        .WithKeyId(LLC::GetRand256())
        .WithGenesis(hashGenesis)
        .WithAuth(true);

    NodeSessionEntry keyMismatchEntry(
        12345,
        hashKeyID,
        hashGenesis,
        true,
        false,
        runtime::unifiedtimestamp(),
        keyMismatchContext
    );

    REQUIRE(keyMismatchEntry.ValidateConsistency() == SessionConsistencyResult::FalconKeyMismatch);
}

TEST_CASE("NodeSessionRegistry - Concurrent Access", "[llp]")
{
    /* Clear registry for clean test */
    NodeSessionRegistry::Get().Clear();

    /* Launch multiple threads registering sessions */
    const int numThreads = 10;
    const int sessionsPerThread = 10;
    std::vector<std::thread> threads;

    for(int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]()
        {
            for(int j = 0; j < sessionsPerThread; ++j)
            {
                uint256_t hashKeyID = LLC::GetRand256();
                uint256_t hashGenesis = LLC::GetRand256();
                MiningContext context;
                context = context.WithKeyId(hashKeyID);
                context = context.WithGenesis(hashGenesis);
                context = context.WithAuth(true);

                NodeSessionRegistry::Get().RegisterOrRefresh(
                    hashKeyID, hashGenesis, context, ProtocolLane::STATELESS
                );
            }
        });
    }

    /* Wait for all threads */
    for(auto& thread : threads)
        thread.join();

    /* Verify all sessions registered */
    REQUIRE(NodeSessionRegistry::Get().Count() == numThreads * sessionsPerThread);
}
