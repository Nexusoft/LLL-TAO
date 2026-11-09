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

    /* Only committed best-chain transitions change work identity. Keep the
     * hash and generation together so a reader cannot stamp work for one tip
     * with another tip's epoch, including an A -> B -> A reorganization. */
    class MiningWorkEpoch
    {
    public:
        void Commit(const uint1024_t& tip)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(tip == committedTip)
                return;

            committedTip = tip;
            ++epoch;
            debug::log(0, "MiningWorkEpoch: work epoch advanced to ", epoch,
                       " reason=committed-best-chain-change best=", tip.SubString());
        }

        uint64_t ForTip(const uint1024_t& tip)
        {
            std::lock_guard<std::mutex> lock(mutex);
            return tip == committedTip ? epoch : 0;
        }

    private:
        std::mutex mutex;
        uint1024_t committedTip{0};
        uint64_t epoch{0};
    };

    /* Peer evidence gates readiness, not the identity of existing work. */
    class MiningPeerTracker
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto MAX_AGE = std::chrono::seconds(90);

        void Observe(const void* peer, const std::string& address,
                     const uint1024_t& hash, uint32_t height,
                     Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            Expire(now);
            const auto it = observations.find(peer);
            if(it == observations.end() || it->second.hash != hash
            || it->second.height != height || it->second.address != address
            || it->second.uncertain)
                cacheValid = false;
            observations[peer] = {address, hash, height, now, false};
        }

        /* A contradictory unpaired field cannot refresh or continue to
         * support the old tip while its counterpart is outstanding. */
        void InvalidatePartial(const void* peer, bool hasHash, const uint1024_t& hash,
                               bool hasHeight, uint32_t height)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = observations.find(peer);
            if(it == observations.end())
                return;

            auto& observation = it->second;
            if((hasHash && observation.hash != hash)
            || (hasHeight && observation.height != height))
            {
                observation.uncertain = true;
                cacheValid = false;
            }
        }

        void Height(const void* peer, uint32_t height)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = observations.find(peer);
            if(it != observations.end() && it->second.height != height)
            {
                cacheValid = false;
                it->second.height = height;
            }
        }

        void Remove(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(observations.erase(peer))
                cacheValid = false;
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
            if(cacheValid && cachedKey == key && now < refreshAt && now <= expireAt)
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
            else
            {
                status.ready = true;
                status.reason = bypass ? "local-network" : "ready";
            }
            return status;
        }

        struct Observation
        {
            std::string address;
            uint1024_t hash;
            uint32_t height;
            Clock::time_point observed;
            bool uncertain;
        };

        void Expire(Clock::time_point now)
        {
            for(auto it = observations.begin(); it != observations.end();)
            {
                if(now - it->second.observed > MAX_AGE)
                {
                    it = observations.erase(it);
                    cacheValid = false;
                }
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<const void*, Observation> observations;
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
