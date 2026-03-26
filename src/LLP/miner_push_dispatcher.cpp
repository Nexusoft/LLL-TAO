/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/global.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/mining_constants.h>
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

    /* Per-channel UNIX timestamps of the last successful push dispatch.
     * Zero = never dispatched (skip heartbeat until first real push fires). */
    std::atomic<uint64_t> MinerPushDispatcher::s_nLastPrimeDispatchTime{0};
    std::atomic<uint64_t> MinerPushDispatcher::s_nLastHashDispatchTime{0};

    /* Async push queue and synchronisation primitives. */
    std::queue<std::pair<uint32_t, uint1024_t>> MinerPushDispatcher::s_pushQueue;
    std::mutex                                   MinerPushDispatcher::s_pushMutex;
    std::condition_variable                      MinerPushDispatcher::s_pushCV;
    std::thread                                  MinerPushDispatcher::s_pushThread;
    std::atomic<bool>                            MinerPushDispatcher::s_pushRunning{false};


    /* Pack (height, hashPrefix4) into a single 64-bit dedup key. */
    static inline uint64_t make_dedup_key(uint32_t nHeight, uint32_t nHashPrefix4)
    {
        return (static_cast<uint64_t>(nHeight) << 32) | static_cast<uint64_t>(nHashPrefix4);
    }


    /* BroadcastChannel — send one channel notification to both lanes. */
    void MinerPushDispatcher::BroadcastChannel(uint32_t nChannel,
                                               uint32_t nHeight,
                                               uint32_t hashPrefix4,
                                               bool fHeartbeat)
    {
        const char* strChannel = (nChannel == 1) ? "Prime" : "Hash";

        /* Stateless lane */
        uint32_t nStateless = 0;
        if (LLP::STATELESS_MINER_SERVER)
            nStateless = LLP::STATELESS_MINER_SERVER->NotifyChannelMiners(nChannel, fHeartbeat);
        else
            debug::log(1, FUNCTION, "[PUSH][Stateless][", strChannel, "] Server not active");

        /* Legacy lane */
        uint32_t nLegacy = 0;
        if (LLP::MINING_SERVER)
            nLegacy = LLP::MINING_SERVER->NotifyChannelMiners(nChannel, fHeartbeat);
        else
            debug::log(1, FUNCTION, "[PUSH][Legacy][", strChannel, "] Server not active");

        /* Summary: one line confirms dedup and exact send counts for this channel. */
        debug::log(0, FUNCTION,
                   (fHeartbeat ? "[HEARTBEAT]" : ""),
                   "[PUSH][", strChannel, "] height=", nHeight,
                   " hash=", std::hex, hashPrefix4, std::dec,
                   " | Stateless=", nStateless, " Legacy=", nLegacy,
                   " | no duplicates (one send per miner per lane)");
    }


    /* DispatchPushEvent — canonical entry-point for all miner push broadcasts. */
    void MinerPushDispatcher::DispatchPushEvent(uint32_t nHeight,
                                                const uint1024_t& hashBestChain)
    {
        if (config::fShutdown.load())
            return;

        /* Derive 32-bit dedup prefix from the first 64-bit limb of hashBestChain.
         * uint1024_t stores bytes little-endian, so the low 32 bits of Get64(0)
         * correspond to the first 4 bytes previously obtained via GetBytes(). */
        const uint32_t hashPrefix4 =
            static_cast<uint32_t>(hashBestChain.Get64(0) & 0xffffffffULL);

        const uint64_t nNewKey = make_dedup_key(nHeight, hashPrefix4);
        const uint64_t nNow    = runtime::unifiedtimestamp();

        /* --- Prime channel dedup --- */
        {
            uint64_t nOldPrime = s_nPrimeDedup.load(std::memory_order_acquire);
            if (nOldPrime != nNewKey &&
                s_nPrimeDedup.compare_exchange_strong(nOldPrime, nNewKey,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed))
            {
                BroadcastChannel(1 /* Prime */, nHeight, hashPrefix4);
                s_nLastPrimeDispatchTime.store(nNow, std::memory_order_release);
            }
            else if (nOldPrime == nNewKey)
            {
                debug::log(1, FUNCTION,
                           "[PUSH][Prime] Dedup: already dispatched for height=", nHeight,
                           " hash=", std::hex, hashPrefix4, std::dec, "; skipping");
            }
        }

        /* --- Hash channel dedup --- */
        {
            uint64_t nOldHash = s_nHashDedup.load(std::memory_order_acquire);
            if (nOldHash != nNewKey &&
                s_nHashDedup.compare_exchange_strong(nOldHash, nNewKey,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed))
            {
                BroadcastChannel(2 /* Hash */, nHeight, hashPrefix4);
                s_nLastHashDispatchTime.store(nNow, std::memory_order_release);
            }
            else if (nOldHash == nNewKey)
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
        if (config::fShutdown.load())
            return;

        /* If the async worker is running, enqueue the event and wake the worker. */
        if (s_pushRunning.load(std::memory_order_acquire))
        {
            {
                std::lock_guard<std::mutex> lock(s_pushMutex);
                s_pushQueue.emplace(nHeight, hashBestChain);
            }
            s_pushCV.notify_one();
            return;
        }

        /* Fallback: worker not started (e.g. unit-test context) — dispatch synchronously. */
        DispatchPushEvent(nHeight, hashBestChain);
    }


    /* PushWorkerThread — drains the async queue and dispatches events. */
    void MinerPushDispatcher::PushWorkerThread()
    {
        debug::log(1, FUNCTION, "Push-worker thread started");

        while (true)
        {
            std::pair<uint32_t, uint1024_t> event;
            bool fGotEvent = false;

            {
                std::unique_lock<std::mutex> lock(s_pushMutex);

                /* Wait until there is work to do or we are asked to stop. */
                s_pushCV.wait(lock, []
                {
                    return !s_pushQueue.empty() || !s_pushRunning.load(std::memory_order_acquire);
                });

                if (!s_pushQueue.empty())
                {
                    event = std::move(s_pushQueue.front());
                    s_pushQueue.pop();
                    fGotEvent = true;
                }
            }

            if (fGotEvent)
            {
                DispatchPushEvent(event.first, event.second);
            }
            else if (!s_pushRunning.load(std::memory_order_acquire))
            {
                /* Drain any remaining items before exiting. */
                std::unique_lock<std::mutex> lock(s_pushMutex);
                while (!s_pushQueue.empty())
                {
                    event = std::move(s_pushQueue.front());
                    s_pushQueue.pop();
                    lock.unlock();
                    DispatchPushEvent(event.first, event.second);
                    lock.lock();
                }
                break;
            }
        }

        debug::log(1, FUNCTION, "Push-worker thread exiting");
    }


    /* StartPushWorker — launch the dedicated push-notification worker thread. */
    void MinerPushDispatcher::StartPushWorker()
    {
        bool fExpected = false;
        if (!s_pushRunning.compare_exchange_strong(fExpected, true,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire))
        {
            debug::log(1, FUNCTION, "Push-worker already running; ignoring duplicate start");
            return;
        }

        s_pushThread = std::thread(&MinerPushDispatcher::PushWorkerThread);
        debug::log(0, FUNCTION, "Push-worker thread launched");
    }


    /* StopPushWorker — signal the worker thread to finish and join it. */
    void MinerPushDispatcher::StopPushWorker()
    {
        if (!s_pushRunning.load(std::memory_order_acquire))
            return;

        s_pushRunning.store(false, std::memory_order_release);
        s_pushCV.notify_all();

        if (s_pushThread.joinable())
            s_pushThread.join();

        debug::log(0, FUNCTION, "Push-worker thread stopped");
    }


    /* HeartbeatRefreshCheck — proactively re-push templates during dry spells. */
    void MinerPushDispatcher::HeartbeatRefreshCheck()
    {
        if(config::fShutdown.load())
            return;

        const uint64_t nNow = runtime::unifiedtimestamp();

        /* Per-channel state table for concise iteration */
        struct ChannelEntry {
            uint32_t                  nChannel;
            std::atomic<uint64_t>&    nLastTime;
            const char*               strName;
        };

        ChannelEntry channels[] = {
            { 1, s_nLastPrimeDispatchTime, "Prime" },
            { 2, s_nLastHashDispatchTime,  "Hash"  }
        };

        for(auto& ch : channels)
        {
            const uint64_t nLastTime = ch.nLastTime.load(std::memory_order_acquire);

            /* Skip if no dispatch has ever been made (node just started). */
            if(nLastTime == 0)
                continue;

            const uint64_t nElapsed = (nNow > nLastTime) ? (nNow - nLastTime) : 0;

            /* Operator-visible dry spell warnings before the heartbeat fires */
            if(nElapsed >= 550)
            {
                debug::log(0, FUNCTION,
                    "[HEARTBEAT][", ch.strName, "] CRITICAL: No push in ", nElapsed,
                    "s — heartbeat refresh fires at ",
                    FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS, "s");
            }
            else if(nElapsed >= 450)
            {
                debug::log(0, FUNCTION,
                    "[HEARTBEAT][", ch.strName, "] WARNING: No push in ", nElapsed,
                    "s — approaching heartbeat refresh threshold (",
                    FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS, "s)");
            }
            else if(nElapsed >= 300)
            {
                debug::log(0, FUNCTION,
                    "[HEARTBEAT][", ch.strName, "] NOTICE: No new ", ch.strName,
                    " block in ", nElapsed, "s — heartbeat refresh will fire at ",
                    FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS, "s");
            }

            /* Fire heartbeat refresh when threshold exceeded */
            if(nElapsed < FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS)
                continue;

            /* CAS on the dispatch timestamp prevents double-firing when both the
             * Legacy and Stateless Meter threads call HeartbeatRefreshCheck()
             * concurrently.  Only the first thread that succeeds the CAS proceeds. */
            uint64_t nExpected = nLastTime;
            if(!ch.nLastTime.compare_exchange_strong(nExpected, nNow,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed))
                continue;  /* Another thread already fired the heartbeat for this channel */

            debug::log(0, FUNCTION,
                "[HEARTBEAT][", ch.strName, "] Firing heartbeat refresh — ",
                nElapsed, "s since last push (threshold=",
                FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS, "s); ",
                "re-pushing current template to all subscribed miners");

            /* Read current chain state for logging height/hash in BroadcastChannel.
             * Load both from the same tStateBest snapshot to keep height and hash
             * consistent even if the chain tip advances between two separate loads.
             * SendChannelNotification() re-reads chain state independently, so this
             * snapshot is only used for the log line — correctness is unaffected. */
            const TAO::Ledger::BlockState stateBest =
                TAO::Ledger::ChainState::tStateBest.load();
            const uint32_t nHeight      = stateBest.nHeight;
            const uint1024_t hashBestChain =
                PushNotificationBuilder::BestChainHashForNotification(stateBest);
            const uint32_t nHashPrefix4 = static_cast<uint32_t>(
                hashBestChain.Get64(0) & 0xffffffffULL);

            BroadcastChannel(ch.nChannel, nHeight, nHashPrefix4, true /* fHeartbeat */);
        }
    }

} // namespace LLP
