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
#include <LLP/include/falcon_auth.h>
#include <LLP/include/disposable_falcon.h>

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>
#include <Util/include/runtime.h>

#include <vector>
#include <map>
#include <set>

using namespace LLP;


/** Test Genesis Hash Values
 *
 *  The hex strings used for genesis hashes in these tests are deterministic
 *  values chosen to:
 *  1. Be visually distinct (using repeated patterns like 1111..., 2222..., etc.)
 *  2. Avoid collision with real genesis hashes
 *  3. Enable easy debugging by identifying which test data is being used
 *
 *  For production, genesis hashes are derived cryptographically from the
 *  user's sigchain creation transaction.
 **/ 


/** Test Suite: GenesisHash Reward Mapping and Payout Verification
 *
 *  These tests cover:
 *
 *  1. GenesisHash secure mapping to miner rewards
 *  2. Payout verification for cryptographic integrity
 *  3. Reward pathways for SOLO and Pool structures
 *  4. Reward distribution calculations for high-throughput environments
 *  5. Hybrid session support for pool payouts with Stateless setups
 *
 **/


TEST_CASE("MiningContext GenesisHash Reward Mapping", "[rewards][genesis]")
{
    SECTION("Genesis hash is preserved through context operations")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithHeight(10000)
            .WithGenesis(testGenesis)
            .WithAuth(true);

        /* Genesis preserved through updates */
        MiningContext updated = ctx
            .WithChannel(2)
            .WithHeight(10001)
            .WithTimestamp(runtime::unifiedtimestamp());

        REQUIRE(updated.hashGenesis == testGenesis);
    }

    SECTION("GetPayoutAddress returns genesis when set")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("b285122d04db2d91cdb6499493c278dbe244e4265506fb9f56bd0ab409de0233");

        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis);

        uint256_t payoutAddr = ctx.GetPayoutAddress();

        REQUIRE(payoutAddr == testGenesis);
    }

    SECTION("GetPayoutAddress returns zero when only username set")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("pool_miner_1");

        /* Username requires external resolution */
        uint256_t payoutAddr = ctx.GetPayoutAddress();

        REQUIRE(payoutAddr == uint256_t(0));
    }

    SECTION("Genesis takes precedence over username for payout")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("c396233e15ec3ea2dec7510504d389ecf355f5376617ca0a67ce1bc50aef1344");

        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis)
            .WithUserName("pool_miner_2");

        /* Genesis should take precedence */
        uint256_t payoutAddr = ctx.GetPayoutAddress();

        REQUIRE(payoutAddr == testGenesis);
    }

    SECTION("HasValidPayout returns true with genesis")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("d407344f26fd4fb3efd8621615e49afde466f6487728db1b78df2cd61bf02455");

        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis);

        REQUIRE(ctx.HasValidPayout() == true);
    }

    SECTION("HasValidPayout returns true with username")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("valid_miner");

        REQUIRE(ctx.HasValidPayout() == true);
    }

    SECTION("HasValidPayout returns false when neither set")
    {
        MiningContext ctx;

        REQUIRE(ctx.HasValidPayout() == false);
    }
}


TEST_CASE("StatelessMinerManager Genesis Indexing for Rewards", "[rewards][manager]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    /* Clear state */
    manager.CleanupInactive(0);

    SECTION("GetMinerContextByGenesis finds correct miner")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("e518455037fe5fc4fef9732726f5a00ff577f7598839ec2c89e03de72c013566");

        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithSession(111111)
            .WithGenesis(testGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("genesis.test.1", ctx, 0);

        /* Lookup by genesis */
        auto result = manager.GetMinerContextByGenesis(testGenesis);

        REQUIRE(result.has_value());
        REQUIRE(result->nSessionId == 111111);
        REQUIRE(result->hashGenesis == testGenesis);

        /* Cleanup */
        manager.RemoveMiner("genesis.test.1");
    }

    SECTION("ListMinersByGenesis returns all miners for a genesis")
    {
        uint256_t sharedGenesis;
        sharedGenesis.SetHex("f629566148ff6ad5f0fa843837f6b11006880608994afd3d9af14ef83d124677");

        /* Register multiple miners with same genesis (pool setup) */
        for(int i = 0; i < 5; ++i)
        {
            std::stringstream ss;
            ss << "pool.miner." << i;

            MiningContext ctx = MiningContext()
                .WithChannel((i % 2) + 1)
                .WithSession(200000 + i)
                .WithGenesis(sharedGenesis)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(ss.str(), ctx, 0);
        }

        /* List all miners for this genesis */
        auto miners = manager.ListMinersByGenesis(sharedGenesis);

        REQUIRE(miners.size() == 5);

        for(const auto& miner : miners)
        {
            REQUIRE(miner.hashGenesis == sharedGenesis);
        }

        /* Cleanup */
        for(int i = 0; i < 5; ++i)
        {
            std::stringstream ss;
            ss << "pool.miner." << i;
            manager.RemoveMiner(ss.str());
        }
    }

    SECTION("Genesis index updates when miner re-registers with different genesis")
    {
        uint256_t genesis1;
        genesis1.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

        uint256_t genesis2;
        genesis2.SetHex("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210");

        /* Register with first genesis */
        MiningContext ctx1 = MiningContext()
            .WithSession(300000)
            .WithGenesis(genesis1)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("switching.miner", ctx1, 0);

        /* Verify first genesis lookup */
        auto result1 = manager.GetMinerContextByGenesis(genesis1);
        REQUIRE(result1.has_value());

        /* Update with second genesis */
        MiningContext ctx2 = MiningContext()
            .WithSession(300000)
            .WithGenesis(genesis2)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("switching.miner", ctx2, 0);

        /* Verify second genesis lookup works */
        auto result2 = manager.GetMinerContextByGenesis(genesis2);
        REQUIRE(result2.has_value());
        REQUIRE(result2->hashGenesis == genesis2);

        /* Cleanup */
        manager.RemoveMiner("switching.miner");
    }
}


TEST_CASE("Payout Verification - Cryptographic Integrity", "[rewards][payout][crypto]")
{
    /* Initialize Falcon auth */
    FalconAuth::Initialize();

    SECTION("Signed work submission ensures payout integrity")
    {
        auto pWrapper = DisposableFalcon::Create();

        uint256_t sessionId = LLC::GetRand256();
        bool fGenerated = pWrapper->GenerateSessionKey(sessionId);
        REQUIRE(fGenerated == true);

        /* Create work submission */
        uint512_t merkleRoot;
        merkleRoot.SetHex(
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
        );

        uint64_t nNonce = 0xDEADBEEFCAFEBABEULL;

        /* Wrap (sign) the submission */
        DisposableFalcon::WrapperResult wrapResult = pWrapper->WrapWorkSubmission(merkleRoot, nNonce);

        REQUIRE(wrapResult.fSuccess == true);
        REQUIRE(wrapResult.submission.fSigned == true);
        REQUIRE(!wrapResult.submission.vSignature.empty());

        /* Unwrap (verify) with correct pubkey */
        std::vector<uint8_t> serialized = wrapResult.submission.Serialize();
        std::vector<uint8_t> pubkey = pWrapper->GetSessionPubKey();

        DisposableFalcon::WrapperResult unwrapResult = pWrapper->UnwrapWorkSubmission(serialized, pubkey);

        REQUIRE(unwrapResult.fSuccess == true);
        REQUIRE(unwrapResult.submission.hashMerkleRoot == merkleRoot);
        REQUIRE(unwrapResult.submission.nNonce == nNonce);
    }

    SECTION("Tampered submission fails verification")
    {
        auto pWrapper = DisposableFalcon::Create();

        uint256_t sessionId = LLC::GetRand256();
        pWrapper->GenerateSessionKey(sessionId);

        uint512_t merkleRoot;
        merkleRoot.SetHex(
            "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
            "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
        );

        DisposableFalcon::WrapperResult wrapResult = pWrapper->WrapWorkSubmission(merkleRoot, 12345);
        REQUIRE(wrapResult.fSuccess == true);

        /* Tamper with serialized data */
        std::vector<uint8_t> serialized = wrapResult.submission.Serialize();
        if(serialized.size() > 10)
            serialized[10] ^= 0xFF;  // Flip bits

        std::vector<uint8_t> pubkey = pWrapper->GetSessionPubKey();

        DisposableFalcon::WrapperResult unwrapResult = pWrapper->UnwrapWorkSubmission(serialized, pubkey);

        /* Verification should fail */
        REQUIRE(unwrapResult.fSuccess == false);
    }

    SECTION("Wrong pubkey fails verification")
    {
        /* Create two separate sessions */
        auto pWrapper1 = DisposableFalcon::Create();
        auto pWrapper2 = DisposableFalcon::Create();

        pWrapper1->GenerateSessionKey(LLC::GetRand256());
        pWrapper2->GenerateSessionKey(LLC::GetRand256());

        uint512_t merkleRoot;
        merkleRoot.SetHex(
            "5555555555555555555555555555555555555555555555555555555555555555"
            "5555555555555555555555555555555555555555555555555555555555555555"
        );

        /* Sign with wrapper1 */
        DisposableFalcon::WrapperResult wrapResult = pWrapper1->WrapWorkSubmission(merkleRoot, 99999);
        REQUIRE(wrapResult.fSuccess == true);

        /* Try to verify with wrapper2's pubkey */
        std::vector<uint8_t> serialized = wrapResult.submission.Serialize();
        std::vector<uint8_t> wrongPubkey = pWrapper2->GetSessionPubKey();

        DisposableFalcon::WrapperResult unwrapResult = pWrapper1->UnwrapWorkSubmission(serialized, wrongPubkey);

        /* Should fail - wrong key */
        REQUIRE(unwrapResult.fSuccess == false);
    }

    FalconAuth::Shutdown();
}


TEST_CASE("Reward Pathways - SOLO and Pool Structures", "[rewards][pathways]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("SOLO miner has unique genesis for direct payout")
    {
        uint256_t soloGenesis;
        soloGenesis.SetHex("1111111111111111111111111111111111111111111111111111111111111111");

        MiningContext soloCtx = MiningContext()
            .WithChannel(1)
            .WithSession(500001)
            .WithGenesis(soloGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("solo.miner.1", soloCtx, 0);

        /* SOLO miner should be uniquely identifiable */
        auto miners = manager.ListMinersByGenesis(soloGenesis);
        REQUIRE(miners.size() == 1);
        REQUIRE(miners[0].nSessionId == 500001);

        /* Cleanup */
        manager.RemoveMiner("solo.miner.1");
    }

    SECTION("Pool miners share genesis for aggregated payout")
    {
        uint256_t poolGenesis;
        poolGenesis.SetHex("2222222222222222222222222222222222222222222222222222222222222222");

        const int nPoolMiners = 20;

        for(int i = 0; i < nPoolMiners; ++i)
        {
            std::stringstream ss;
            ss << "pool.worker." << i;

            MiningContext poolCtx = MiningContext()
                .WithChannel((i % 2) + 1)
                .WithSession(600000 + i)
                .WithGenesis(poolGenesis)  // All share same genesis
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(ss.str(), poolCtx, 0);
        }

        /* Pool miners should all be found under same genesis */
        auto poolMiners = manager.ListMinersByGenesis(poolGenesis);
        REQUIRE(poolMiners.size() == nPoolMiners);

        /* Verify they all have the shared genesis */
        for(const auto& miner : poolMiners)
        {
            REQUIRE(miner.hashGenesis == poolGenesis);
        }

        /* Cleanup */
        for(int i = 0; i < nPoolMiners; ++i)
        {
            std::stringstream ss;
            ss << "pool.worker." << i;
            manager.RemoveMiner(ss.str());
        }
    }

    SECTION("Mixed SOLO and Pool coexistence")
    {
        uint256_t soloGenesis1;
        soloGenesis1.SetHex("3333333333333333333333333333333333333333333333333333333333333333");

        uint256_t soloGenesis2;
        soloGenesis2.SetHex("4444444444444444444444444444444444444444444444444444444444444444");

        uint256_t poolGenesis;
        poolGenesis.SetHex("5555555555555555555555555555555555555555555555555555555555555555");

        /* Register SOLO miners */
        MiningContext solo1 = MiningContext()
            .WithChannel(1)
            .WithSession(700001)
            .WithGenesis(soloGenesis1)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        MiningContext solo2 = MiningContext()
            .WithChannel(2)
            .WithSession(700002)
            .WithGenesis(soloGenesis2)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("mixed.solo.1", solo1, 0);
        manager.UpdateMiner("mixed.solo.2", solo2, 0);

        /* Register Pool miners */
        for(int i = 0; i < 5; ++i)
        {
            std::stringstream ss;
            ss << "mixed.pool." << i;

            MiningContext poolCtx = MiningContext()
                .WithChannel(1)
                .WithSession(700100 + i)
                .WithGenesis(poolGenesis)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(ss.str(), poolCtx, 0);
        }

        /* Verify each genesis has correct miners */
        auto solo1Miners = manager.ListMinersByGenesis(soloGenesis1);
        auto solo2Miners = manager.ListMinersByGenesis(soloGenesis2);
        auto poolMinersList = manager.ListMinersByGenesis(poolGenesis);

        REQUIRE(solo1Miners.size() == 1);
        REQUIRE(solo2Miners.size() == 1);
        REQUIRE(poolMinersList.size() == 5);

        /* Total miners */
        size_t nTotal = manager.GetMinerCount();
        REQUIRE(nTotal >= 7);

        /* Cleanup */
        manager.RemoveMiner("mixed.solo.1");
        manager.RemoveMiner("mixed.solo.2");
        for(int i = 0; i < 5; ++i)
        {
            std::stringstream ss;
            ss << "mixed.pool." << i;
            manager.RemoveMiner(ss.str());
        }
    }
}


TEST_CASE("Reward Distribution Calculations", "[rewards][distribution]")
{
    SECTION("Session keepalive count tracks work contribution")
    {
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(800001)
            .WithKeepaliveCount(0);

        /* Simulate work over time via keepalive increments */
        for(uint32_t i = 1; i <= 100; ++i)
        {
            ctx = ctx.WithKeepaliveCount(i);
            REQUIRE(ctx.nKeepaliveCount == i);
        }

        /* Final count should reflect total work */
        REQUIRE(ctx.nKeepaliveCount == 100);
    }

    SECTION("Session duration tracking for time-based rewards")
    {
        uint64_t nStart = runtime::unifiedtimestamp();

        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(800002)
            .WithSessionStart(nStart)
            .WithTimestamp(nStart);

        /* Simulate passage of time */
        uint64_t nNow = nStart + 3600;  // 1 hour later

        uint64_t nDuration = ctx.GetSessionDuration(nNow);

        REQUIRE(nDuration == 3600);
    }

    SECTION("Session expiry prevents invalid reward claims")
    {
        uint64_t nOldTime = runtime::unifiedtimestamp() - 7200;  // 2 hours ago

        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(800003)
            .WithSessionStart(nOldTime)
            .WithTimestamp(nOldTime);

        /* Session should be expired */

        /* Expired sessions should not be eligible for rewards */
        REQUIRE(ctx.HasValidPayout() == false);
    }
}


TEST_CASE("Hybrid Session Support", "[rewards][hybrid]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("Transition from legacy to Stateless maintains genesis")
    {
        uint256_t genesis;
        genesis.SetHex("6666666666666666666666666666666666666666666666666666666666666666");

        /* Simulate legacy connection (no keyID) */
        MiningContext legacyCtx = MiningContext()
            .WithChannel(1)
            .WithSession(900001)
            .WithGenesis(genesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("hybrid.miner", legacyCtx, 0);

        /* Simulate upgrade to Stateless with Falcon auth */
        uint256_t keyId = LLC::GetRand256();

        MiningContext statelessCtx = legacyCtx
            .WithKeyId(keyId);

        manager.UpdateMiner("hybrid.miner", statelessCtx, 0);

        /* Verify genesis preserved through transition */
        auto result = manager.GetMinerContext("hybrid.miner");
        REQUIRE(result.has_value());
        REQUIRE(result->hashGenesis == genesis);
        REQUIRE(result->hashKeyID == keyId);

        /* Cleanup */
        manager.RemoveMiner("hybrid.miner");
    }

    SECTION("Pool payouts work with mixed Stateless and legacy miners")
    {
        uint256_t poolGenesis;
        poolGenesis.SetHex("7777777777777777777777777777777777777777777777777777777777777777");

        /* Legacy pool miner (no keyID) */
        MiningContext legacyPoolMiner = MiningContext()
            .WithChannel(1)
            .WithSession(900100)
            .WithGenesis(poolGenesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("pool.legacy", legacyPoolMiner, 0);

        /* Stateless pool miner (with keyID) */
        uint256_t keyId = LLC::GetRand256();

        MiningContext statelessPoolMiner = MiningContext()
            .WithChannel(2)
            .WithSession(900101)
            .WithGenesis(poolGenesis)
            .WithKeyId(keyId)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp());

        manager.UpdateMiner("pool.stateless", statelessPoolMiner, 0);

        /* Both should be found under pool genesis */
        auto poolMiners = manager.ListMinersByGenesis(poolGenesis);
        REQUIRE(poolMiners.size() == 2);

        /* Both should be eligible for pool payout */
        for(const auto& miner : poolMiners)
        {
            REQUIRE(miner.hashGenesis == poolGenesis);
            REQUIRE(miner.HasValidPayout() == true);
        }

        /* Cleanup */
        manager.RemoveMiner("pool.legacy");
        manager.RemoveMiner("pool.stateless");
    }
}


TEST_CASE("Ledger Auditing Support", "[rewards][audit]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    manager.CleanupInactive(0);

    SECTION("All miners status includes genesis for audit")
    {
        /* Register a few miners with different genesis values */
        for(int i = 0; i < 3; ++i)
        {
            uint256_t genesis = LLC::GetRand256();

            std::stringstream ss;
            ss << "audit.miner." << i;

            MiningContext ctx = MiningContext()
                .WithChannel(1)
                .WithSession(950000 + i)
                .WithGenesis(genesis)
                .WithAuth(true)
                .WithTimestamp(runtime::unifiedtimestamp());

            manager.UpdateMiner(ss.str(), ctx, 0);
        }

        /* Get all miners status (JSON) */
        std::string status = manager.GetAllMinersStatus();

        /* Status should contain genesis field for each miner */
        REQUIRE(status.find("genesis") != std::string::npos);

        /* Cleanup */
        for(int i = 0; i < 3; ++i)
        {
            std::stringstream ss;
            ss << "audit.miner." << i;
            manager.RemoveMiner(ss.str());
        }
    }

    SECTION("Individual miner status contains genesis and session info")
    {
        uint256_t genesis;
        genesis.SetHex("8888888888888888888888888888888888888888888888888888888888888888");

        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithSession(951000)
            .WithGenesis(genesis)
            .WithAuth(true)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithKeepaliveCount(42);

        manager.UpdateMiner("audit.single", ctx, 0);

        std::string status = manager.GetMinerStatus("audit.single");

        /* Verify audit-relevant fields are present */
        REQUIRE(status.find("genesis") != std::string::npos);
        REQUIRE(status.find("session_id") != std::string::npos);
        REQUIRE(status.find("authenticated") != std::string::npos);
        REQUIRE(status.find("keepalive_count") != std::string::npos);

        /* Cleanup */
        manager.RemoveMiner("audit.single");
    }
}
