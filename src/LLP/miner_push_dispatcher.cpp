/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/global.h>
#include <LLP/include/push_notification.h>

#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/debug.h>
#include <Util/include/config.h>
#include <Util/include/runtime.h>
#include <vector>

namespace LLP
{

    /* Static dedup-key storage — one per PoW channel.
     * Layout: high-32 bits = unified height, low-32 bits = first 4 bytes of hashBestChain.
     * Initial value 0 ensures first real dispatch always proceeds. */
    std::atomic<uint64_t> MinerPushDispatcher::s_nPrimeDedup{0};
    std::atomic<uint64_t> MinerPushDispatcher::s_nHashDedup{0};

    /* Per-lane async queue storage and synchronisation primitives. */
    std::queue<std::pair<uint32_t, uint1024_t>> MinerPushDispatcher::s_statelessQueue;
    std::mutex                                   MinerPushDispatcher::s_statelessMutex;
    std::condition_variable                      MinerPushDispatcher::s_statelessCV;
    std::thread                                  MinerPushDispatcher::s_statelessThread;
    std::atomic<bool>                            MinerPushDispatcher::s_statelessRunning{false};

    std::queue<std::pair<uint32_t, uint1024_t>> MinerPushDispatcher::s_legacyQueue;
    std::mutex                                   MinerPushDispatcher::s_legacyMutex;
    std::condition_variable                      MinerPushDispatcher::s_legacyCV;
    std::thread                                  MinerPushDispatcher::s_legacyThread;
    std::atomic<bool>                            MinerPushDispatcher::s_legacyRunning{false};


    /* Pack (height, hashPrefix4) into a single 64-bit dedup key. */
    static inline uint64_t make_dedup_key(uint32_t nHeight, uint32_t nHashPrefix4)
    {
        return (static_cast<uint64_t>(nHeight) << 32) | static_cast<uint64_t>(nHashPrefix4);
    }


    /* BroadcastStatelessChannel — send one channel notification to stateless lane. */
    void MinerPushDispatcher::BroadcastStatelessChannel(uint32_t nChannel,
                                                         uint32_t nHeight,
                                                         uint32_t hashPrefix4)
    {
        const char* strChannel = (nChannel == 1) ? "Prime" : "Hash";

        uint32_t nStateless = 0;
        if(LLP::STATELESS_MINER_SERVER)
            nStateless = LLP::STATELESS_MINER_SERVER->NotifyChannelMiners(nChannel);
        else
            debug::log(1, FUNCTION, "[PUSH][Stateless][", strChannel, "] Server not active");

        debug::log(0, FUNCTION,
                   "[PUSH][Stateless][", strChannel, "] height=", nHeight,
                   " hash=", std::hex, hashPrefix4, std::dec,
                   " notified=", nStateless);
    }


    /* BroadcastLegacyChannel — send one channel notification to legacy lane. */
    void MinerPushDispatcher::BroadcastLegacyChannel(uint32_t nChannel,
                                                      uint32_t nHeight,
                                                      uint32_t hashPrefix4)
    {
        const char* strChannel = (nChannel == 1) ? "Prime" : "Hash";

        uint32_t nLegacy = 0;
        if(LLP::MINING_SERVER)
            nLegacy = LLP::MINING_SERVER->NotifyChannelMiners(nChannel);
        else
            debug::log(1, FUNCTION, "[PUSH][Legacy][", strChannel, "] Server not active");

        debug::log(0, FUNCTION,
                   "[PUSH][Legacy][", strChannel, "] height=", nHeight,
                   " hash=", std::hex, hashPrefix4, std::dec,
                   " notified=", nLegacy);
    }


    /* DispatchStatelessPush — stateless lane dispatch with dedup. */
    void MinerPushDispatcher::DispatchStatelessPush(uint32_t nHeight,
                                                     const uint1024_t& hashBestChain)
    {
        if(config::fShutdown.load())
            return;

        const uint32_t hashPrefix4 =
            static_cast<uint32_t>(hashBestChain.Get64(0) & 0xffffffffULL);
        const uint64_t nNewKey = make_dedup_key(nHeight, hashPrefix4);

        /* Prime channel */
        {
            uint64_t nOld = s_nPrimeDedup.load(std::memory_order_acquire);
            if(nOld != nNewKey &&
                s_nPrimeDedup.compare_exchange_strong(nOld, nNewKey,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed))
            {
                BroadcastStatelessChannel(1, nHeight, hashPrefix4);
            }
        }

        /* Hash channel */
        {
            uint64_t nOld = s_nHashDedup.load(std::memory_order_acquire);
            if(nOld != nNewKey &&
                s_nHashDedup.compare_exchange_strong(nOld, nNewKey,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed))
            {
                BroadcastStatelessChannel(2, nHeight, hashPrefix4);
            }
        }
    }


    /* DispatchLegacyPush — legacy lane dispatch. */
    void MinerPushDispatcher::DispatchLegacyPush(uint32_t nHeight,
                                                  const uint1024_t& hashBestChain)
    {
        if(config::fShutdown.load())
            return;

        const uint32_t hashPrefix4 =
            static_cast<uint32_t>(hashBestChain.Get64(0) & 0xffffffffULL);

        /* Legacy lane broadcasts both channels.  Dedup is already handled by the
         * shared atomics (s_nPrimeDedup / s_nHashDedup) at the DispatchPushEvent level. */
        BroadcastLegacyChannel(1, nHeight, hashPrefix4);
        BroadcastLegacyChannel(2, nHeight, hashPrefix4);
    }


    /* DispatchPushEvent — canonical entry-point for all miner push broadcasts.
     * Used as synchronous fallback when workers are not running. */
    void MinerPushDispatcher::DispatchPushEvent(uint32_t nHeight,
                                                const uint1024_t& hashBestChain)
    {
        if(config::fShutdown.load())
            return;

        const uint32_t hashPrefix4 =
            static_cast<uint32_t>(hashBestChain.Get64(0) & 0xffffffffULL);
        const uint64_t nNewKey = make_dedup_key(nHeight, hashPrefix4);

        /* --- Prime channel dedup + broadcast to both lanes --- */
        {
            uint64_t nOldPrime = s_nPrimeDedup.load(std::memory_order_acquire);
            if(nOldPrime != nNewKey &&
                s_nPrimeDedup.compare_exchange_strong(nOldPrime, nNewKey,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed))
            {
                BroadcastStatelessChannel(1, nHeight, hashPrefix4);
                BroadcastLegacyChannel(1, nHeight, hashPrefix4);
            }
            else if(nOldPrime == nNewKey)
            {
                debug::log(1, FUNCTION,
                           "[PUSH][Prime] Dedup: already dispatched for height=", nHeight,
                           " hash=", std::hex, hashPrefix4, std::dec, "; skipping");
            }
        }

        /* --- Hash channel dedup + broadcast to both lanes --- */
        {
            uint64_t nOldHash = s_nHashDedup.load(std::memory_order_acquire);
            if(nOldHash != nNewKey &&
                s_nHashDedup.compare_exchange_strong(nOldHash, nNewKey,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed))
            {
                BroadcastStatelessChannel(2, nHeight, hashPrefix4);
                BroadcastLegacyChannel(2, nHeight, hashPrefix4);
            }
            else if(nOldHash == nNewKey)
            {
                debug::log(1, FUNCTION,
                           "[PUSH][Hash] Dedup: already dispatched for height=", nHeight,
                           " hash=", std::hex, hashPrefix4, std::dec, "; skipping");
            }
        }
    }


    /* EnqueuePushEvent — non-blocking caller for SetBest() on the Tritium data thread. */
    void MinerPushDispatcher::EnqueuePushEvent(uint32_t nHeight,
                                               const uint1024_t& hashBestChain)
    {
        if(config::fShutdown.load())
            return;

        const bool fStatelessUp = s_statelessRunning.load(std::memory_order_acquire);
        const bool fLegacyUp    = s_legacyRunning.load(std::memory_order_acquire);

        /* If either worker is running, enqueue to per-lane queues */
        if(fStatelessUp || fLegacyUp)
        {
            if(fStatelessUp)
            {
                {
                    std::lock_guard<std::mutex> lock(s_statelessMutex);
                    s_statelessQueue.emplace(nHeight, hashBestChain);
                }
                s_statelessCV.notify_one();
            }

            if(fLegacyUp)
            {
                {
                    std::lock_guard<std::mutex> lock(s_legacyMutex);
                    s_legacyQueue.emplace(nHeight, hashBestChain);
                }
                s_legacyCV.notify_one();
            }

            return;
        }

        /* Fallback: workers not started (e.g. unit-test context) — dispatch synchronously. */
        DispatchPushEvent(nHeight, hashBestChain);
    }


    /* StatelessWorkerThread — drains the stateless async queue. */
    void MinerPushDispatcher::StatelessWorkerThread()
    {
        debug::log(1, FUNCTION, "Stateless push-worker thread started");

        while(true)
        {
            std::pair<uint32_t, uint1024_t> event;
            bool fGotEvent = false;

            {
                std::unique_lock<std::mutex> lock(s_statelessMutex);
                s_statelessCV.wait(lock, []
                {
                    return !s_statelessQueue.empty() || !s_statelessRunning.load(std::memory_order_acquire);
                });

                if(!s_statelessQueue.empty())
                {
                    event = std::move(s_statelessQueue.front());
                    s_statelessQueue.pop();
                    fGotEvent = true;
                }
            }

            if(fGotEvent)
            {
                DispatchStatelessPush(event.first, event.second);
            }
            else if(!s_statelessRunning.load(std::memory_order_acquire))
            {
                /* Drain remaining items before exiting. */
                std::unique_lock<std::mutex> lock(s_statelessMutex);
                while(!s_statelessQueue.empty())
                {
                    event = std::move(s_statelessQueue.front());
                    s_statelessQueue.pop();
                    lock.unlock();
                    DispatchStatelessPush(event.first, event.second);
                    lock.lock();
                }
                break;
            }
        }

        debug::log(1, FUNCTION, "Stateless push-worker thread exiting");
    }


    /* LegacyWorkerThread — drains the legacy async queue. */
    void MinerPushDispatcher::LegacyWorkerThread()
    {
        debug::log(1, FUNCTION, "Legacy push-worker thread started");

        while(true)
        {
            std::pair<uint32_t, uint1024_t> event;
            bool fGotEvent = false;

            {
                std::unique_lock<std::mutex> lock(s_legacyMutex);
                s_legacyCV.wait(lock, []
                {
                    return !s_legacyQueue.empty() || !s_legacyRunning.load(std::memory_order_acquire);
                });

                if(!s_legacyQueue.empty())
                {
                    event = std::move(s_legacyQueue.front());
                    s_legacyQueue.pop();
                    fGotEvent = true;
                }
            }

            if(fGotEvent)
            {
                DispatchLegacyPush(event.first, event.second);
            }
            else if(!s_legacyRunning.load(std::memory_order_acquire))
            {
                std::unique_lock<std::mutex> lock(s_legacyMutex);
                while(!s_legacyQueue.empty())
                {
                    event = std::move(s_legacyQueue.front());
                    s_legacyQueue.pop();
                    lock.unlock();
                    DispatchLegacyPush(event.first, event.second);
                    lock.lock();
                }
                break;
            }
        }

        debug::log(1, FUNCTION, "Legacy push-worker thread exiting");
    }


    /* StartPushWorker — launch per-lane push-notification worker threads. */
    void MinerPushDispatcher::StartPushWorker()
    {
        /* Start stateless worker */
        {
            bool fExpected = false;
            if(s_statelessRunning.compare_exchange_strong(fExpected, true,
                                                            std::memory_order_release,
                                                            std::memory_order_acquire))
            {
                s_statelessThread = std::thread(&MinerPushDispatcher::StatelessWorkerThread);
                debug::log(0, FUNCTION, "Stateless push-worker thread launched");
            }
        }

        /* Start legacy worker */
        {
            bool fExpected = false;
            if(s_legacyRunning.compare_exchange_strong(fExpected, true,
                                                         std::memory_order_release,
                                                         std::memory_order_acquire))
            {
                s_legacyThread = std::thread(&MinerPushDispatcher::LegacyWorkerThread);
                debug::log(0, FUNCTION, "Legacy push-worker thread launched");
            }
        }
    }


    /* StopPushWorker — signal both worker threads to finish and join them. */
    void MinerPushDispatcher::StopPushWorker()
    {
        /* Stop stateless worker */
        if(s_statelessRunning.load(std::memory_order_acquire))
        {
            s_statelessRunning.store(false, std::memory_order_release);
            s_statelessCV.notify_all();

            if(s_statelessThread.joinable())
                s_statelessThread.join();

            debug::log(0, FUNCTION, "Stateless push-worker thread stopped");
        }

        /* Stop legacy worker */
        if(s_legacyRunning.load(std::memory_order_acquire))
        {
            s_legacyRunning.store(false, std::memory_order_release);
            s_legacyCV.notify_all();

            if(s_legacyThread.joinable())
                s_legacyThread.join();

            debug::log(0, FUNCTION, "Legacy push-worker thread stopped");
        }
    }

} // namespace LLP
