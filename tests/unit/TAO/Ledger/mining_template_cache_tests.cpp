/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/API/types/transaction.h>
#include <LLD/include/global.h>
#include <LLC/include/random.h>
#include <Legacy/types/transaction.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace
{
    struct TemplateStateGuard
    {
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::tStateBest.load();
        uint1024_t hashBest = TAO::Ledger::ChainState::hashBestChain.load();
        uint512_t hashLegacy = 0;
        uint512_t hashFunding = 0;
        uint512_t hashUnselected = 0;

        ~TemplateStateGuard()
        {
            if(hashLegacy != 0)
                TAO::Ledger::mempool.Remove(hashLegacy);
            if(hashUnselected != 0)
                TAO::Ledger::mempool.Remove(hashUnselected);
            if(hashFunding != 0)
                LLD::Legacy->EraseSpend(hashFunding, 0);
            TAO::Ledger::ChainState::tStateBest.store(state);
            TAO::Ledger::ChainState::hashBestChain.store(hashBest);
            TAO::Ledger::ChainState::CommitMiningTip(hashBest);
            TAO::Ledger::ClearMiningTemplateCaches("test cleanup");
        }

        void Publish(TAO::Ledger::BlockState tip)
        {
            TAO::Ledger::ChainState::tStateBest.store(tip);
            TAO::Ledger::ChainState::hashBestChain.store(tip.GetHash());
            TAO::Ledger::ChainState::CommitMiningTip(tip.GetHash());
        }
    };

    uint256_t MakeReward(uint8_t nDistinguisher)
    {
        uint256_t r = 0;
        std::vector<uint8_t> bytes(32, 0);
        bytes[0] = 0xA1;
        bytes[1] = nDistinguisher;
        bytes[31] = 0xCC;
        r.SetBytes(bytes);
        return r;
    }
}

TEST_CASE("Mining templates use the committed producer predecessor after tip changes",
    "[tao][ledger][llp][template_freshness]")
{
    TemplateStateGuard guard;
    TAO::Ledger::ClearMiningTemplateCaches("test setup");
    memory::encrypted_ptr<TAO::Ledger::Credentials> user =
        new TAO::Ledger::Credentials(LLC::GetRand256().ToString().c_str(), "template-test");

    TAO::Ledger::BlockState tip;
    tip.nChannel = 1;
    tip.nHeight = 1;
    tip.nChannelHeight = 1;
    guard.Publish(tip);

    TAO::Ledger::TritiumBlock initial;
    REQUIRE(TAO::Ledger::CreateBlock(user, "1234", 1, initial));

    TAO::Ledger::Transaction committed = initial.producer;
    TAO::API::Transaction session = initial.producer;
    SECTION("locally accepted producer advances the ledger before session indexing")
    {
        committed.nSequence++;
        committed.hashPrevTx = initial.producer.GetHash();
    }
    SECTION("reorg replaces the predecessor at the same sequence")
    {
        committed.nTimestamp++;
    }
    SECTION("reorg rewinds below the disconnected session tail")
    {
        session.nSequence += 2;
    }

    REQUIRE(LLD::Sessions->WriteTx(session.GetHash(), session));
    REQUIRE(LLD::Sessions->WriteLast(user->Genesis(), session.GetHash()));
    REQUIRE(LLD::Ledger->WriteTx(committed.GetHash(), committed));
    REQUIRE(LLD::Ledger->WriteLast(user->Genesis(), committed.GetHash()));
    tip.nHeight++;
    tip.nNonce++;
    guard.Publish(tip);

    TAO::Ledger::TritiumBlock next;
    REQUIRE(TAO::Ledger::CreateBlock(user, "1234", 1, next));
    REQUIRE(next.hashPrevBlock == tip.GetHash());
    REQUIRE(next.producer.hashPrevTx == committed.GetHash());
    REQUIRE(next.producer.nSequence == committed.nSequence + 1);

    TAO::Ledger::TritiumBlock cached;
    REQUIRE(TAO::Ledger::CreateBlock(user, "1234", 1, cached));
    REQUIRE(cached.producer.hashPrevTx == committed.GetHash());

    TAO::Ledger::Transaction unselected = committed;
    unselected.nSequence++;
    unselected.hashPrevTx = committed.GetHash();
    guard.hashUnselected = unselected.GetHash();
    REQUIRE(TAO::Ledger::mempool.AddUnchecked(unselected));
    REQUIRE(TAO::Ledger::CreateBlock(user, "1234", 1, next));
    REQUIRE(next.producer.hashPrevTx == committed.GetHash());

    /* Explicit in-block predecessors still take priority over the ledger. */
    TAO::Ledger::Transaction producer;
    REQUIRE(TAO::Ledger::CreateProducer(user, "1234", producer, tip, next.nVersion,
        1, 0, nullptr, uint256_t(0), &unselected));
    REQUIRE(producer.hashPrevTx == unselected.GetHash());
    REQUIRE(producer.nSequence == unselected.nSequence + 1);

    /* A selected vtx hash that can no longer be read must not look like "no
     * in-block predecessor", which would fall back to the older ledger tail. */
    TAO::Ledger::TritiumBlock selected;
    selected.vtx.emplace_back(TAO::Ledger::TRANSACTION::TRITIUM, guard.hashUnselected);
    TAO::Ledger::Transaction txFound;
    bool fComplete = false;
    REQUIRE(TAO::Ledger::FindProducerGenesisTxInVtx(selected, user->Genesis(), txFound, fComplete));
    REQUIRE(fComplete);
    REQUIRE(txFound.GetHash() == unselected.GetHash());

    REQUIRE(TAO::Ledger::mempool.Remove(guard.hashUnselected));
    guard.hashUnselected = 0;
    REQUIRE_FALSE(TAO::Ledger::FindProducerGenesisTxInVtx(selected, user->Genesis(), txFound, fComplete));
    REQUIRE_FALSE(fComplete);
}

TEST_CASE("Mining templates skip spent legacy inputs and reconsider them after reorg",
    "[tao][ledger][llp][template_freshness]")
{
    TemplateStateGuard guard;
    Legacy::Transaction funding;
    funding.nTime = runtime::unifiedtimestamp() - 1;
    funding.vin.emplace_back(LLC::GetRand512(), 0);
    funding.vout.emplace_back(10000000, Legacy::Script() << Legacy::OP_TRUE);
    guard.hashFunding = funding.GetHash();
    REQUIRE(LLD::Legacy->WriteTx(guard.hashFunding, funding));
    REQUIRE(LLD::Ledger->IndexBlock(guard.hashFunding, guard.hashBest));

    Legacy::Transaction candidate;
    candidate.nTime = runtime::unifiedtimestamp();
    candidate.vin.emplace_back(guard.hashFunding, 0);
    candidate.vout.emplace_back(9000000, Legacy::Script() << Legacy::OP_TRUE);
    guard.hashLegacy = candidate.GetHash();
    REQUIRE(TAO::Ledger::mempool.AddUnchecked(candidate));

    TAO::Ledger::TritiumBlock block;
    TAO::Ledger::AddTransactions(block);
    const auto containsCandidate = [&]() {
        for(const auto& entry : block.vtx)
            if(entry.second == guard.hashLegacy)
                return true;
        return false;
    };
    REQUIRE(containsCandidate());

    REQUIRE(LLD::Legacy->WriteSpend(guard.hashFunding, 0));
    debug::strLastError.clear();
    TAO::Ledger::AddTransactions(block);
    REQUIRE_FALSE(containsCandidate());
    REQUIRE(TAO::Ledger::mempool.Has(guard.hashLegacy));
    REQUIRE(debug::GetLastError().find("prev tx " + guard.hashFunding.SubString()
        + " is already spent") == std::string::npos);

    REQUIRE(LLD::Legacy->EraseSpend(guard.hashFunding, 0));
    TAO::Ledger::AddTransactions(block);
    REQUIRE(containsCandidate());
}

TEST_CASE("Mining template cache singleflight coalesces same reward", "[tao][ledger][singleflight]")
{
    constexpr uint32_t CHANNEL = 1;
    const uint256_t hashReward = MakeReward(0x11);

    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    std::atomic<int> nOwners{0};
    std::atomic<int> nBuilds{0};
    std::atomic<int> nJoined{0};
    std::atomic<int> nZeroTokens{0};
    std::atomic<int> nMismatchedRewards{0};

    std::vector<std::thread> threads;
    threads.reserve(10);

    for(int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&]() {
            bool fIsOwner = false;
            auto nToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fIsOwner);
            if(nToken == 0)
            {
                ++nZeroTokens;
                return;
            }

            if(fIsOwner)
            {
                ++nOwners;
                ++nBuilds;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nToken, CHANNEL, hashReward, 7);
            }
            else
            {
                uint256_t hashOut = 0;
                const bool fJoined = TAO::Ledger::Testing::WaitForMiningTemplateInFlight(
                    CHANNEL, nToken, std::chrono::milliseconds(500), hashOut);
                if(fJoined)
                {
                    if(hashOut == hashReward)
                        ++nJoined;
                    else
                        ++nMismatchedRewards;
                }
            }
        });
    }

    for(auto& t : threads)
        t.join();

    REQUIRE(nOwners.load() == 1);
    REQUIRE(nBuilds.load() == 1);
    REQUIRE(nJoined.load() == 9);
    REQUIRE(nZeroTokens.load() == 0);
    REQUIRE(nMismatchedRewards.load() == 0);
    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);
}


TEST_CASE("Mining template cache singleflight owner abandonment unblocks waiters", "[tao][ledger][singleflight]")
{
    constexpr uint32_t CHANNEL = 1;
    const uint256_t hashReward = MakeReward(0x12);

    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    bool fOwner = false;
    const auto nOwnerToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fOwner);
    REQUIRE(fOwner);
    REQUIRE(nOwnerToken != 0);

    std::atomic<int> nTimeouts{0};
    std::atomic<int> nUnexpectedOwners{0};
    std::atomic<int> nWaitersReady{0};
    std::atomic<int> nZeroTokens{0};

    std::vector<std::thread> threads;
    threads.reserve(5);

    for(int i = 0; i < 5; ++i)
    {
        threads.emplace_back([&]() {
            bool fIsOwner = false;
            auto nToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fIsOwner);
            if(nToken == 0)
            {
                ++nZeroTokens;
                return;
            }

            if(fIsOwner)
            {
                ++nUnexpectedOwners;
                TAO::Ledger::Testing::AbandonMiningTemplateInFlight(nToken, CHANNEL);
            }
            else
            {
                ++nWaitersReady;
                uint256_t hashOut = 0;
                const bool fJoined = TAO::Ledger::Testing::WaitForMiningTemplateInFlight(
                    CHANNEL, nToken, std::chrono::milliseconds(500), hashOut);
                if(!fJoined)
                    ++nTimeouts;
            }
        });
    }

    const auto nWaitersReadyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while(nWaitersReady.load() < 5 && std::chrono::steady_clock::now() < nWaitersReadyDeadline)
        std::this_thread::yield();

    const bool fAllWaitersReady = (nWaitersReady.load() == 5);

    TAO::Ledger::Testing::AbandonMiningTemplateInFlight(nOwnerToken, CHANNEL);

    for(auto& t : threads)
        t.join();

    REQUIRE(nZeroTokens.load() == 0);
    REQUIRE(fAllWaitersReady);
    REQUIRE(nTimeouts.load() == 5);
    REQUIRE(nUnexpectedOwners.load() == 0);
    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);
}


TEST_CASE("Mining template cache singleflight timeout fallback", "[tao][ledger][singleflight]")
{
    constexpr uint32_t CHANNEL = 1;
    const uint256_t hashReward = MakeReward(0x13);

    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    bool fOwner = false;
    const auto nOwnerToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fOwner);
    REQUIRE(fOwner);
    REQUIRE(nOwnerToken != 0);

    bool fWaiterOwner = true;
    const auto nWaiterToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fWaiterOwner);
    REQUIRE_FALSE(fWaiterOwner);
    REQUIRE(nWaiterToken != 0);

    std::thread owner([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nOwnerToken, CHANNEL, hashReward, 9);
    });

    uint256_t hashOut = 0;
    const bool fJoined = TAO::Ledger::Testing::WaitForMiningTemplateInFlight(
        CHANNEL, nWaiterToken, std::chrono::milliseconds(30), hashOut);
    REQUIRE_FALSE(fJoined);

    TAO::Ledger::Testing::StoreMiningTemplateCacheEntryForTesting(CHANNEL, hashReward, 99);

    owner.join();

    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);
}

TEST_CASE("Mining template cache singleflight timeout retry may become new owner", "[tao][ledger][singleflight]")
{
    constexpr uint32_t CHANNEL = 1;
    constexpr auto WAITER_TIMEOUT = std::chrono::milliseconds(120);
    const uint256_t hashReward = MakeReward(0x14);

    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    bool fOwner = false;
    const auto nOwnerToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fOwner);
    REQUIRE(fOwner);
    REQUIRE(nOwnerToken != 0);

    bool fWaiterOwner = true;
    const auto nWaiterToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fWaiterOwner);
    REQUIRE_FALSE(fWaiterOwner);
    REQUIRE(nWaiterToken != 0);

    uint256_t hashOut = 0;
    const bool fJoined = TAO::Ledger::Testing::WaitForMiningTemplateInFlight(
        CHANNEL, nWaiterToken, WAITER_TIMEOUT, hashOut);
    REQUIRE_FALSE(fJoined);

    TAO::Ledger::Testing::AbandonMiningTemplateInFlight(nOwnerToken, CHANNEL);

    bool fRetryOwner = false;
    const auto nRetryToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fRetryOwner);
    REQUIRE(fRetryOwner);
    REQUIRE(nRetryToken != 0);

    TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nRetryToken, CHANNEL, hashReward, 10);
    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);
}


TEST_CASE("Mining template cache singleflight isolates different rewards", "[tao][ledger][singleflight]")
{
    constexpr uint32_t CHANNEL = 1;
    const uint256_t hashRewardX = MakeReward(0x21);
    const uint256_t hashRewardY = MakeReward(0x22);

    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    bool fOwnerX = false;
    const auto nTokenX = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashRewardX, fOwnerX);
    REQUIRE(fOwnerX);
    REQUIRE(nTokenX != 0);

    bool fOwnerY = false;
    const auto nTokenY = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashRewardY, fOwnerY);
    REQUIRE(fOwnerY);
    REQUIRE(nTokenY != 0);

    TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nTokenX, CHANNEL, hashRewardX, 1);
    TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nTokenY, CHANNEL, hashRewardY, 2);

    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);
}


TEST_CASE("Mining template cache rejects publication after a committed tip change",
    "[tao][ledger][singleflight][template_freshness]")
{
    constexpr uint32_t CHANNEL = 1;
    const uint256_t hashReward = MakeReward(0x31);

    TemplateStateGuard guard;
    TAO::Ledger::Testing::ClearMiningTemplateCacheForTesting(CHANNEL);

    TAO::Ledger::BlockState tipOld;
    tipOld.nChannel = 1;
    tipOld.nHeight = 1;
    guard.Publish(tipOld);
    const uint1024_t hashOldTip = tipOld.GetHash();

    /* In-flight owner and joined waiter pass their tip check on the old tip. */
    bool fOwner = false;
    const auto nOwnerToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fOwner, hashOldTip);
    REQUIRE(fOwner);
    bool fWaiterOwner = true;
    const auto nWaiterToken = TAO::Ledger::Testing::BeginOrJoinMiningTemplateInFlight(CHANNEL, hashReward, fWaiterOwner, hashOldTip);
    REQUIRE_FALSE(fWaiterOwner);

    /* SetBest() commits a new tip and clears the caches before publication. */
    TAO::Ledger::BlockState tipNew = tipOld;
    tipNew.nHeight++;
    tipNew.nNonce++;
    guard.Publish(tipNew);
    TAO::Ledger::ClearMiningTemplateCaches("test committed tip change");

    REQUIRE_FALSE(TAO::Ledger::Testing::CompleteMiningTemplateInFlight(nOwnerToken, CHANNEL, hashReward, 1));
    REQUIRE_FALSE(TAO::Ledger::Testing::StoreMiningTemplateCacheEntryForTesting(CHANNEL, hashReward, 2, hashOldTip));
    REQUIRE(TAO::Ledger::Testing::MiningTemplateCacheSizeForTesting(CHANNEL) == 0);
    REQUIRE(TAO::Ledger::Testing::MiningTemplateInFlightCountForTesting(CHANNEL) == 0);

    uint256_t hashOut = 0;
    REQUIRE_FALSE(TAO::Ledger::Testing::WaitForMiningTemplateInFlight(
        CHANNEL, nWaiterToken, std::chrono::milliseconds(50), hashOut));

    /* Builds against the current tip still publish normally. */
    REQUIRE(TAO::Ledger::Testing::StoreMiningTemplateCacheEntryForTesting(CHANNEL, hashReward, 3, tipNew.GetHash()));
    REQUIRE(TAO::Ledger::Testing::MiningTemplateCacheSizeForTesting(CHANNEL) == 1);
}
