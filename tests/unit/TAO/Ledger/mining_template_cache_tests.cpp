/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/Ledger/include/create.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace
{
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
