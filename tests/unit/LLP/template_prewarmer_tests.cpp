/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_session_health.h>
#include <LLP/include/template_prewarmer.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/tritium.h>
#include <Util/include/args.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    uint256_t MakeReward(uint8_t nDistinguisher)
    {
        uint256_t reward = nDistinguisher;
        reward.SetType(TAO::Ledger::GENESIS::UserType());
        return reward;
    }

    uint1024_t CurrentTip()
    {
        return TAO::Ledger::ChainState::hashBestChain.load();
    }

    template<typename Predicate>
    bool WaitUntil(Predicate predicate)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while(std::chrono::steady_clock::now() < deadline)
        {
            if(predicate())
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return predicate();
    }

    struct Gate
    {
        std::mutex mutex;
        std::condition_variable cv;
        std::size_t entered{0};
        bool released{false};

        void Enter()
        {
            std::unique_lock<std::mutex> lock(mutex);
            ++entered;
            cv.notify_all();
            cv.wait(lock, [this] { return released; });
        }

        bool WaitFor(std::size_t count)
        {
            std::unique_lock<std::mutex> lock(mutex);
            return cv.wait_for(lock, std::chrono::seconds(3),
                               [this, count] { return entered >= count; });
        }

        void Release()
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                released = true;
            }
            cv.notify_all();
        }
    };

    class PrewarmerFixture
    {
    public:
        LLP::RecentRewardRegistry& registry = LLP::RecentRewardRegistry::Instance();
        LLP::MiningTemplatePrewarmer& warmer = LLP::MiningTemplatePrewarmer::Instance();

        explicit PrewarmerFixture(std::function<void()> release = {})
            : wasRunning(warmer.IsRunning()), releaseWorkers(std::move(release))
        {
            warmer.Stop();
            for(const auto& key : keys)
            {
                const auto it = config::mapArgs.find(key);
                if(it != config::mapArgs.end())
                {
                    savedArgs.emplace(it->first, it->second);
                    config::mapArgs.erase(it);
                }
            }
            warmer.ResetBuildFnForTesting();
            registry.ClearForTesting();
        }

        ~PrewarmerFixture()
        {
            if(releaseWorkers)
                releaseWorkers();
            warmer.Stop();
            warmer.ResetBuildFnForTesting();
            registry.ClearForTesting();
            for(const auto& key : keys)
                config::mapArgs.erase(key);
            for(const auto& arg : savedArgs)
                config::mapArgs[arg.first] = arg.second;
            if(wasRunning)
                warmer.Start();
        }

    private:
        const std::vector<std::string> keys{
            "-prewarm", "-prewarm.reward_ttl", "-prewarm.queue_max", "-prewarm.workers"};
        bool wasRunning;
        std::function<void()> releaseWorkers;
        std::map<std::string, std::string> savedArgs;
    };
}


TEST_CASE("RecentRewardRegistry deduplicates and respects TTL",
          "[template_prewarmer][registry]")
{
    PrewarmerFixture fixture;
    auto& reg = fixture.registry;

    const uint256_t rA = MakeReward(1);
    const uint256_t rB = MakeReward(2);
    REQUIRE(rA.GetType() == TAO::Ledger::GENESIS::UserType());

    reg.Register(0, rA);
    reg.Register(1, uint256_t(0));
    uint256_t invalid = rA;
    invalid.SetType(TAO::Ledger::GENESIS::UserType() ^ 1);
    reg.Register(1, invalid);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).empty());

    reg.Register(1, rA);
    reg.Register(2, rA);
    reg.Register(1, rA);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).size() == 2);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(0)).empty());

    reg.Register(1, rB);
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).size() == 3);
    reg.Prune(std::chrono::seconds(0));
    REQUIRE(reg.SnapshotFresh(std::chrono::seconds(60)).empty());
}


TEST_CASE("MiningTemplatePrewarmer warms one request per registered tuple per tip",
          "[template_prewarmer][warm]")
{
    struct Record
    {
        uint32_t channel;
        uint256_t reward;
        uint1024_t tip;
    };
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<Record> records;
    PrewarmerFixture fixture;
    auto& warmer = fixture.warmer;
    auto& reg = fixture.registry;

    warmer.SetBuildFnForTesting([&](uint32_t channel, const uint256_t& reward, const uint1024_t& tip)
    {
        std::lock_guard<std::mutex> lock(mutex);
        records.push_back({channel, reward, tip});
        cv.notify_all();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint256_t rA = MakeReward(11);
    const uint256_t rB = MakeReward(12);
    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    reg.Register(1, rA);
    reg.Register(2, rA);
    reg.Register(1, rB);

    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(100, tip);
    {
        std::unique_lock<std::mutex> lock(mutex);
        REQUIRE(cv.wait_for(lock, std::chrono::seconds(3), [&] { return records.size() == 3; }));
    }
    warmer.Stop();

    std::map<std::pair<uint32_t, uint256_t>, std::size_t> counts;
    for(const auto& record : records)
    {
        REQUIRE(record.tip == tip);
        ++counts[{record.channel, record.reward}];
    }
    REQUIRE(counts.size() == 3);
    REQUIRE(counts[{1, rA}] == 1);
    REQUIRE(counts[{2, rA}] == 1);
    REQUIRE(counts[{1, rB}] == 1);
    const auto after = warmer.GetStats();
    REQUIRE(after.nEnqueued == before.nEnqueued + 3);
    REQUIRE(after.nWarmed == before.nWarmed + 3);
}


TEST_CASE("MiningTemplatePrewarmer coalesces queued and in-flight requests",
          "[template_prewarmer][coalesce]")
{
    Gate gate;
    PrewarmerFixture fixture([&] { gate.Release(); });
    auto& warmer = fixture.warmer;
    auto& reg = fixture.registry;
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        gate.Enter();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    reg.Register(1, MakeReward(21));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(gate.WaitFor(1));

    reg.Register(1, MakeReward(22));
    reg.Register(2, MakeReward(21));
    warmer.NotifyTipAdvance(200, tip);
    warmer.NotifyTipAdvance(200, tip);
    std::vector<std::jthread> notifiers;
    for(int i = 0; i < 8; ++i)
        notifiers.emplace_back([&] { warmer.NotifyTipAdvance(200, tip); });
    for(auto& notifier : notifiers)
        notifier.join();
    REQUIRE(warmer.GetStats().nEnqueued == before.nEnqueued + 3);

    gate.Release();
    REQUIRE(gate.WaitFor(3));
    REQUIRE(WaitUntil([&] { return warmer.GetStats().nWarmed == before.nWarmed + 3; }));

    reg.ClearForTesting();
    reg.Register(1, MakeReward(21));
    REQUIRE(WaitUntil([&]
    {
        warmer.NotifyTipAdvance(200, tip);
        return warmer.GetStats().nEnqueued == before.nEnqueued + 4;
    }));
    REQUIRE(gate.WaitFor(4));
    warmer.Stop();
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed + 4);
}


TEST_CASE("MiningTemplatePrewarmer retries after failed or throwing builds",
          "[template_prewarmer][failure]")
{
    bool throwFirst = false;
    SECTION("builder returns no template") {}
    SECTION("builder throws") { throwFirst = true; }

    std::atomic<std::size_t> attempts{0};
    PrewarmerFixture fixture;
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&) -> TAO::Ledger::TritiumBlock*
    {
        if(attempts.fetch_add(1) == 0)
        {
            if(throwFirst)
                throw std::runtime_error("test prewarmer failure");
            return nullptr;
        }
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    fixture.registry.Register(1, MakeReward(23));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(WaitUntil([&] { return attempts.load() == 1; }));
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed);

    REQUIRE(WaitUntil([&]
    {
        warmer.NotifyTipAdvance(200, tip);
        return warmer.GetStats().nEnqueued == before.nEnqueued + 2;
    }));
    REQUIRE(WaitUntil([&] { return warmer.GetStats().nWarmed == before.nWarmed + 1; }));
    warmer.Stop();
    REQUIRE(attempts.load() == 2);
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed + 1);
}


TEST_CASE("MiningTemplatePrewarmer keeps different tip requests independent",
          "[template_prewarmer][coalesce][stale]")
{
    Gate gate;
    PrewarmerFixture fixture([&] { gate.Release(); });
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        gate.Enter();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    uint1024_t staleTip = tip;
    staleTip.SetType(tip.GetType() ^ 1);
    REQUIRE(staleTip != 0);
    fixture.registry.Register(1, MakeReward(26));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(gate.WaitFor(1));
    warmer.NotifyTipAdvance(201, staleTip);
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(warmer.GetStats().nEnqueued == before.nEnqueued + 2);

    gate.Release();
    REQUIRE(WaitUntil([&] { return warmer.GetStats().nStaleTipSkipped == before.nStaleTipSkipped + 1; }));
    warmer.Stop();
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed + 1);
}


TEST_CASE("MiningTemplatePrewarmer releases stale requests",
          "[template_prewarmer][stale]")
{
    std::atomic<std::size_t> builds{0};
    PrewarmerFixture fixture;
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        ++builds;
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    uint1024_t staleTip = CurrentTip();
    REQUIRE(staleTip != 0);
    staleTip.SetType(staleTip.GetType() ^ 1);
    fixture.registry.Register(1, MakeReward(24));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(200, staleTip);
    REQUIRE(WaitUntil([&] { return warmer.GetStats().nStaleTipSkipped == before.nStaleTipSkipped + 1; }));
    REQUIRE(WaitUntil([&]
    {
        warmer.NotifyTipAdvance(200, staleTip);
        return warmer.GetStats().nEnqueued == before.nEnqueued + 2;
    }));
    REQUIRE(WaitUntil([&] { return warmer.GetStats().nStaleTipSkipped == before.nStaleTipSkipped + 2; }));
    warmer.Stop();
    REQUIRE(builds.load() == 0);
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed);
}


TEST_CASE("MiningTemplatePrewarmer releases in-flight requests across stop and restart",
          "[template_prewarmer][lifecycle]")
{
    Gate gate;
    PrewarmerFixture fixture([&] { gate.Release(); });
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        gate.Enter();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    fixture.registry.Register(1, MakeReward(25));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(gate.WaitFor(1));
    std::thread stopper([&] { warmer.Stop(); });
    const bool stopped = WaitUntil([&] { return !warmer.IsRunning(); });
    gate.Release();
    stopper.join();
    REQUIRE(stopped);

    warmer.Start();
    warmer.NotifyTipAdvance(200, tip);
    REQUIRE(gate.WaitFor(2));
    warmer.Stop();
    const auto after = warmer.GetStats();
    REQUIRE(after.nEnqueued == before.nEnqueued + 2);
    REQUIRE(after.nWarmed == before.nWarmed + 2);
}


TEST_CASE("MiningTemplatePrewarmer worker pool warms in parallel",
          "[template_prewarmer][pool]")
{
    Gate gate;
    PrewarmerFixture fixture([&] { gate.Release(); });
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.workers"] = "4";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        gate.Enter();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();
    REQUIRE(warmer.GetStats().nWorkers == 4);

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    for(uint8_t i = 0; i < 4; ++i)
        fixture.registry.Register(1, MakeReward(51 + i));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(500, tip);
    REQUIRE(gate.WaitFor(4));
    gate.Release();
    warmer.Stop();
    REQUIRE(warmer.GetStats().nWarmed == before.nWarmed + 4);
}


TEST_CASE("MiningTemplatePrewarmer drops oldest when queue exceeds cap",
          "[template_prewarmer][drop_oldest]")
{
    Gate gate;
    PrewarmerFixture fixture([&] { gate.Release(); });
    auto& warmer = fixture.warmer;
    config::mapArgs["-prewarm.queue_max"] = "2";
    config::mapArgs["-prewarm.workers"] = "1";
    warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        gate.Enter();
        return new TAO::Ledger::TritiumBlock();
    });
    warmer.Start();

    const uint1024_t tip = CurrentTip();
    REQUIRE(tip != 0);
    for(uint8_t i = 0; i < 5; ++i)
        fixture.registry.Register(1, MakeReward(31 + i));
    const auto before = warmer.GetStats();
    warmer.NotifyTipAdvance(300, tip);
    REQUIRE(warmer.GetStats().nDropped == before.nDropped + 3);
    gate.Release();
    REQUIRE(gate.WaitFor(2));
    warmer.Stop();
    const auto after = warmer.GetStats();
    REQUIRE(after.nEnqueued == before.nEnqueued + 5);
    REQUIRE(after.nWarmed == before.nWarmed + 2);
}


TEST_CASE("MiningTemplatePrewarmer is a no-op when -prewarm=false",
          "[template_prewarmer][disabled]")
{
    std::atomic<std::size_t> builds{0};
    PrewarmerFixture fixture;
    config::mapArgs["-prewarm"] = "0";
    fixture.warmer.SetBuildFnForTesting([&](uint32_t, const uint256_t&, const uint1024_t&)
    {
        ++builds;
        return new TAO::Ledger::TritiumBlock();
    });
    fixture.warmer.Start();
    fixture.registry.Register(1, MakeReward(41));
    const auto before = fixture.warmer.GetStats();
    fixture.warmer.NotifyTipAdvance(400, CurrentTip());
    fixture.warmer.Stop();
    REQUIRE(builds.load() == 0);
    REQUIRE(fixture.warmer.GetStats().nEnqueued == before.nEnqueued);
}


TEST_CASE("MiningTemplatePrewarmer skips notify/build when default session is unavailable",
          "[template_prewarmer][session]")
{
    PrewarmerFixture fixture;
    REQUIRE_FALSE(LLP::IsDefaultSessionReady());
    fixture.warmer.Start();
    const auto before = fixture.warmer.GetStats();
    fixture.registry.Register(1, MakeReward(42));
    fixture.warmer.NotifyTipAdvance(420, CurrentTip());
    fixture.warmer.Stop();
    const auto after = fixture.warmer.GetStats();
    REQUIRE(after.nEnqueued == before.nEnqueued);
    REQUIRE(after.nWarmed == before.nWarmed);
}
