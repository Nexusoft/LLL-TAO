/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_template_delivery.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    class SimAsyncPushWorker
    {
    public:
        using BuildHandler = std::function<void(const uint1024_t&, uint32_t)>;

        explicit SimAsyncPushWorker(BuildHandler onBuild)
            : m_onBuild(std::move(onBuild))
            , m_thread(&SimAsyncPushWorker::Loop, this)
        {
        }

        ~SimAsyncPushWorker()
        {
            Stop();
        }

        bool SchedulePush(const uint1024_t& hashExpectedTip, uint32_t nChannel)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!m_running)
                return false;

            if(LLP::ShouldCoalesceAsyncPush(hashExpectedTip, nChannel,
                                            m_pending, m_pendingTip, m_pendingChannel,
                                            m_inFlight, m_inFlightTip, m_inFlightChannel))
            {
                ++m_coalesced;
                return false;
            }

            m_pending = true;
            m_pendingTip = hashExpectedTip;
            m_pendingChannel = nChannel;
            m_cv.notify_one();
            return true;
        }

        void Stop()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if(!m_running)
                    return;
                m_running = false;
                m_pending = false;
            }
            m_cv.notify_all();
            if(m_thread.joinable())
                m_thread.join();
        }

        uint32_t CoalescedCount() const
        {
            return m_coalesced.load();
        }

    private:
        void Loop()
        {
            while(true)
            {
                uint1024_t tip;
                uint32_t channel = 0;

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [this](){ return m_pending || !m_running; });
                    if(!m_running && !m_pending)
                        return;

                    tip = m_pendingTip;
                    channel = m_pendingChannel;
                    m_pending = false;
                    m_inFlight = true;
                    m_inFlightTip = tip;
                    m_inFlightChannel = channel;
                }

                m_onBuild(tip, channel);

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_inFlight = false;
                }
            }
        }

        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::thread m_thread;
        BuildHandler m_onBuild;
        bool m_running{true};
        bool m_pending{false};
        bool m_inFlight{false};
        uint1024_t m_pendingTip{0};
        uint32_t m_pendingChannel{0};
        uint1024_t m_inFlightTip{0};
        uint32_t m_inFlightChannel{0};
        std::atomic<uint32_t> m_coalesced{0};
    };
}


TEST_CASE("Async PUSH worker coalesces duplicate same-tip schedules", "[llp][async_push]")
{
    std::mutex gateMutex;
    std::condition_variable gateCv;
    bool fBuildEntered = false;
    bool fReleaseBuild = false;

    std::atomic<uint32_t> nBuildCount{0};
    std::thread::id buildThreadId;
    const std::thread::id dataThreadId = std::this_thread::get_id();

    SimAsyncPushWorker worker(
        [&](const uint1024_t&, uint32_t)
        {
            nBuildCount.fetch_add(1, std::memory_order_relaxed);
            buildThreadId = std::this_thread::get_id();

            std::unique_lock<std::mutex> lock(gateMutex);
            fBuildEntered = true;
            gateCv.notify_all();
            gateCv.wait(lock, [&](){ return fReleaseBuild; });
        });

    const uint1024_t tipA(0xAA);
    REQUIRE(worker.SchedulePush(tipA, 1));

    {
        std::unique_lock<std::mutex> lock(gateMutex);
        gateCv.wait(lock, [&](){ return fBuildEntered; });
    }

    REQUIRE_FALSE(worker.SchedulePush(tipA, 1));
    REQUIRE(worker.CoalescedCount() == 1);

    {
        std::lock_guard<std::mutex> lock(gateMutex);
        fReleaseBuild = true;
    }
    gateCv.notify_all();
    worker.Stop();

    REQUIRE(nBuildCount.load(std::memory_order_relaxed) == 1);
    REQUIRE(buildThreadId != dataThreadId);
}


TEST_CASE("Async PUSH tip-fence discards stale build under tip race", "[llp][async_push][template_staleness]")
{
    std::mutex gateMutex;
    std::condition_variable gateCv;
    bool fBuildEntered = false;
    bool fReleaseBuild = false;

    std::atomic<uint32_t> nQueued{0};
    std::atomic<uint32_t> nDiscarded{0};
    uint1024_t hashCurrentTip(0xAA);
    std::mutex tipMutex;

    SimAsyncPushWorker worker(
        [&](const uint1024_t& hashExpectedTip, uint32_t)
        {
            {
                std::unique_lock<std::mutex> lock(gateMutex);
                fBuildEntered = true;
                gateCv.notify_all();
                gateCv.wait(lock, [&](){ return fReleaseBuild; });
            }

            uint1024_t hashCurrent;
            {
                std::lock_guard<std::mutex> lock(tipMutex);
                hashCurrent = hashCurrentTip;
            }

            if(hashCurrent != hashExpectedTip)
            {
                nDiscarded.fetch_add(1, std::memory_order_relaxed);
                return;
            }

            nQueued.fetch_add(1, std::memory_order_relaxed);
        });

    const uint1024_t expectedTip(0xAA);
    REQUIRE(worker.SchedulePush(expectedTip, 2));

    {
        std::unique_lock<std::mutex> lock(gateMutex);
        gateCv.wait(lock, [&](){ return fBuildEntered; });
    }

    {
        std::lock_guard<std::mutex> lock(tipMutex);
        hashCurrentTip = uint1024_t(0xBB);
    }
    {
        std::lock_guard<std::mutex> lock(gateMutex);
        fReleaseBuild = true;
    }
    gateCv.notify_all();
    worker.Stop();

    REQUIRE(nDiscarded.load(std::memory_order_relaxed) == 1);
    REQUIRE(nQueued.load(std::memory_order_relaxed) == 0);
}


TEST_CASE("Async PUSH coalescing keeps channel when non-PUSH schedule has channel=0", "[llp][async_push]")
{
    const uint1024_t tip(0xAA);
    uint32_t nPendingChannel = 1;

    /* Match ScheduleTemplateWork behavior: channel is only updated when meaningful. */
    const uint32_t nIncomingChannel = 0;
    if(nIncomingChannel != 0)
        nPendingChannel = nIncomingChannel;

    REQUIRE(nPendingChannel == 1);
    REQUIRE(LLP::ShouldCoalesceAsyncPush(
        tip, 1,
        true, tip, nPendingChannel,
        false, uint1024_t(0), 0));
}


TEST_CASE("Async PUSH coalescing updates channel when non-PUSH schedule has non-zero channel", "[llp][async_push]")
{
    const uint1024_t tip(0xAA);
    uint32_t nPendingChannel = 1;

    /* Match ScheduleTemplateWork behavior: meaningful channel updates worker channel state. */
    const uint32_t nIncomingChannel = 2;
    if(nIncomingChannel != 0)
        nPendingChannel = nIncomingChannel;

    REQUIRE(nPendingChannel == 2);
    REQUIRE_FALSE(LLP::ShouldCoalesceAsyncPush(
        tip, 1,
        true, tip, nPendingChannel,
        false, uint1024_t(0), 0));
    REQUIRE(LLP::ShouldCoalesceAsyncPush(
        tip, 2,
        true, tip, nPendingChannel,
        false, uint1024_t(0), 0));
}


TEST_CASE("Async PUSH burst handles tip change during in-flight build", "[llp][async_push][template_staleness]")
{
    std::mutex gateMutex;
    std::condition_variable gateCv;
    bool firstBuildEntered = false;
    bool releaseFirstBuild = false;
    bool secondBuildDone = false;

    const uint1024_t tipT1(0xAA);
    const uint1024_t tipT2(0xBB);

    std::atomic<uint32_t> nBuildCount{0};
    std::atomic<uint32_t> nQueued{0};
    std::atomic<uint32_t> nDiscarded{0};
    std::vector<uint1024_t> vBuildOrder;
    std::mutex orderMutex;

    uint1024_t hashCurrentTip = tipT1;
    std::mutex tipMutex;

    SimAsyncPushWorker worker(
        [&](const uint1024_t& hashExpectedTip, uint32_t)
        {
            const uint32_t nBuildIndex = nBuildCount.fetch_add(1, std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> lock(orderMutex);
                vBuildOrder.push_back(hashExpectedTip);
            }

            if(nBuildIndex == 0)
            {
                std::unique_lock<std::mutex> lock(gateMutex);
                firstBuildEntered = true; /* Burst phase 1: T1 build entered. */
                gateCv.notify_all();
                gateCv.wait(lock, [&](){ return releaseFirstBuild; }); /* Hold T1 until test advances to the next phase. */
            }

            uint1024_t hashLiveTip;
            {
                std::lock_guard<std::mutex> lock(tipMutex);
                hashLiveTip = hashCurrentTip;
            }

            if(hashLiveTip != hashExpectedTip)
                nDiscarded.fetch_add(1, std::memory_order_relaxed);
            else
                nQueued.fetch_add(1, std::memory_order_relaxed);

            if(nBuildIndex == 1)
            {
                std::lock_guard<std::mutex> lock(gateMutex);
                secondBuildDone = true;
                gateCv.notify_all();
            }
        });

    REQUIRE(worker.SchedulePush(tipT1, 1));

    {
        std::unique_lock<std::mutex> lock(gateMutex);
        gateCv.wait(lock, [&](){ return firstBuildEntered; });
    }

    /* Burst phase 2: tip advances during T1 build; schedule T2 while T1 is still in-flight. */
    {
        std::lock_guard<std::mutex> lock(tipMutex);
        hashCurrentTip = tipT2;
    }
    REQUIRE(worker.SchedulePush(tipT2, 1));

    {
        std::lock_guard<std::mutex> lock(gateMutex);
        releaseFirstBuild = true; /* Burst phase 3: release T1 so tip-fence can discard it, then T2 builds. */
        gateCv.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(gateMutex);
        gateCv.wait(lock, [&](){ return secondBuildDone; });
    }

    worker.Stop();

    INFO("Burst scenario expected: T1 discarded after tip advance, T2 queued");
    REQUIRE(nBuildCount.load(std::memory_order_relaxed) == 2);
    REQUIRE(nDiscarded.load(std::memory_order_relaxed) == 1);
    REQUIRE(nQueued.load(std::memory_order_relaxed) == 1);
    REQUIRE(worker.CoalescedCount() == 0);

    REQUIRE(vBuildOrder.size() == 2);
    REQUIRE(vBuildOrder[0] == tipT1);
    REQUIRE(vBuildOrder[1] == tipT2);
}
