/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLC/types/uint1024.h>
#include <Util/include/debug.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <tuple>

namespace TAO::Ledger::ChainState
{
    struct MiningStatus
    {
        bool ready{false};
        bool bypass{false};
        uint64_t epoch{0};
        uint32_t peers{0};
        uint32_t required{2};
        std::string reason;
    };

    enum class MiningPeerEvidence { SUPPORTING, BEHIND, UNRESOLVED };

    /* No consensus state is changed here. An epoch revokes work even if
     * connectivity is restored before the next mining request. */
    class MiningPeerTracker
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto MAX_AGE = std::chrono::seconds(90);

        void Observe(const void* peer, const std::string& address,
                     const uint1024_t& hash, uint32_t height,
                     Clock::time_point now = Clock::now())
        {
            ObserveConfirmedTip(peer, address, hash, height, [] { return false; }, now);
        }

        /* A report proven to be the current local tip can only add supporting
         * evidence. Other changes remain revocations, including a change of IP. */
        template<typename ConfirmTip>
        void ObserveConfirmedTip(const void* peer, const std::string& address,
                                 const uint1024_t& hash, uint32_t height,
                                 ConfirmTip confirmTip, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            Expire(now);
            const auto it = observations.find(peer);
            if(it == observations.end() || it->second.hash != hash
            || it->second.height != height || it->second.address != address
            || it->second.uncertain)
            {
                cacheValid = false;
                if((it != observations.end() && it->second.address != address)
                || (it != observations.end() && it->second.pendingCatchUp
                    && (it->second.pendingTip != hash
                     || (it->second.pendingHash && it->second.partialHash != hash)
                     || (it->second.pendingHeight && it->second.partialHeight != height)))
                || !confirmTip())
                    AdvanceEpoch("peer-tip-change");
            }
            observations[peer] = {address, hash, height, now, false};
        }

        /* A contradictory unpaired field cannot refresh or continue to
         * support the old tip while its counterpart is outstanding. */
        template<typename ConfirmCatchUp>
        void InvalidatePartial(const void* peer, bool hasHash, const uint1024_t& hash,
                               bool hasHeight, uint32_t height, ConfirmCatchUp confirmCatchUp)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = observations.find(peer);
            if(it == observations.end())
                return;

            auto& observation = it->second;
            if(observation.uncertain)
            {
                if(observation.pendingCatchUp
                && (confirmCatchUp() != observation.pendingTip
                 || (hasHash && observation.pendingHash && observation.partialHash != hash)
                 || (hasHeight && observation.pendingHeight && observation.partialHeight != height)))
                {
                    observation.pendingCatchUp = false;
                    AdvanceEpoch("contradictory-partial-tip");
                }
                return;
            }

            if((hasHash && observation.hash != hash)
            || (hasHeight && observation.height != height))
            {
                observation.uncertain = true;
                cacheValid = false;
                observation.pendingTip = confirmCatchUp();
                observation.pendingCatchUp = observation.pendingTip != 0;
                if(observation.pendingCatchUp)
                {
                    observation.pendingHash = hasHash;
                    observation.partialHash = hash;
                    observation.pendingHeight = hasHeight;
                    observation.partialHeight = height;
                }
                else
                    AdvanceEpoch("partial-peer-tip");
            }
        }

        void InvalidatePartial(const void* peer, bool hasHash, const uint1024_t& hash,
                               bool hasHeight, uint32_t height)
        {
            InvalidatePartial(peer, hasHash, hash, hasHeight, height, [] { return uint1024_t(0); });
        }

        void Height(const void* peer, uint32_t height)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = observations.find(peer);
            if(it != observations.end() && it->second.height != height)
            {
                cacheValid = false;
                AdvanceEpoch("peer-height-change");
                it->second.height = height;
            }
        }

        void Remove(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(observations.erase(peer))
            {
                cacheValid = false;
                AdvanceEpoch("peer-disconnected");
            }
        }

        template<typename Resolve>
        MiningStatus Check(uint32_t required, bool bypass, bool synchronized,
                           bool reorg, bool shutdown, Resolve resolve,
                           Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            cacheValid = false;
            return CheckLocked(required, bypass, synchronized, reorg, shutdown, resolve, now);
        }

        /* Work checks reuse evidence for at most one second. Peer changes,
         * policy changes, tip changes and MAX_AGE expiry invalidate it sooner. */
        template<typename Resolve>
        MiningStatus CheckCached(const uint1024_t& tip, uint32_t required, bool bypass,
                                 bool synchronized, bool reorg, bool shutdown, Resolve resolve,
                                 Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            const CacheKey key{tip, required, bypass, synchronized, reorg, shutdown};
            if(cacheValid && cachedKey == key && cachedStatus.epoch == epoch
            && now < refreshAt && now <= expireAt)
                return cachedStatus;

            cachedStatus = CheckLocked(required, bypass, synchronized, reorg, shutdown, resolve, now);
            cachedKey = key;
            refreshAt = now + std::chrono::seconds(1);
            expireAt = Clock::time_point::max();
            for(const auto& item : observations)
            {
                const auto expiry = item.second.observed + MAX_AGE;
                if(expiry < expireAt)
                    expireAt = expiry;
            }
            cacheValid = true;
            return cachedStatus;
        }

    private:
        template<typename Resolve>
        MiningStatus CheckLocked(uint32_t required, bool bypass, bool synchronized,
                                 bool reorg, bool shutdown, Resolve resolve, Clock::time_point now)
        {
            Expire(now);
            MiningStatus status;
            status.bypass = bypass;
            status.required = required;
            bool unresolved = false;
            std::set<std::string> supporting;
            if(!bypass)
            {
                for(const auto& item : observations)
                {
                    const auto evidence = item.second.uncertain ? MiningPeerEvidence::UNRESOLVED
                        : resolve(item.second.hash, item.second.height);
                    if(evidence == MiningPeerEvidence::UNRESOLVED)
                        unresolved = true;
                    else if(evidence == MiningPeerEvidence::SUPPORTING)
                        supporting.insert(item.second.address);
                }
            }
            status.peers = supporting.size();
            if(shutdown)
                status.reason = "shutdown";
            else if(reorg)
                status.reason = "chain-reorganization";
            else if(!bypass && !synchronized)
                status.reason = "initial-synchronization";
            else if(!bypass && (required == 0 || required > 16))
                status.reason = "invalid-miningminpeers";
            else if(!bypass && unresolved)
                status.reason = "unresolved-peer-tip";
            else if(!bypass && status.peers < required)
                status.reason = "insufficient-fresh-peers";
            else if(epoch > UINT32_MAX)
                status.reason = "mining-epoch-exhausted";
            else
            {
                status.ready = true;
                status.reason = bypass ? "local-network" : "ready";
            }
            if(wasReady && !status.ready)
                AdvanceEpoch("readiness-paused");
            wasReady = status.ready;
            status.epoch = epoch;
            return status;
        }

        struct Observation
        {
            std::string address;
            uint1024_t hash;
            uint32_t height;
            Clock::time_point observed;
            bool uncertain;
            bool pendingCatchUp{false};
            uint1024_t pendingTip{0};
            bool pendingHash{false};
            uint1024_t partialHash{0};
            bool pendingHeight{false};
            uint32_t partialHeight{0};
        };

        void AdvanceEpoch(const char* reason)
        {
            ++epoch;
            debug::log(0, "MiningPeerTracker: work epoch advanced to ", epoch, " reason=", reason);
        }

        void Expire(Clock::time_point now)
        {
            for(auto it = observations.begin(); it != observations.end();)
            {
                if(now - it->second.observed > MAX_AGE)
                {
                    it = observations.erase(it);
                    AdvanceEpoch("peer-evidence-expired");
                }
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<const void*, Observation> observations;
        uint64_t epoch{1};
        bool wasReady{false};
        using CacheKey = std::tuple<uint1024_t, uint32_t, bool, bool, bool, bool>;
        CacheKey cachedKey;
        MiningStatus cachedStatus;
        Clock::time_point refreshAt{}, expireAt{};
        bool cacheValid{false};
    };

    void ObserveMiningPeer(const void* peer, const std::string& address,
                           const uint1024_t& hash, uint32_t height);
    void UpdateMiningPeerHeight(const void* peer, uint32_t height);
    void InvalidatePartialMiningPeer(const void* peer, bool hasHash, const uint1024_t& hash,
                                     bool hasHeight, uint32_t height);
    MiningStatus MiningReadiness(bool cached = false);
    bool MiningWorkCurrent(uint64_t epoch);
}
