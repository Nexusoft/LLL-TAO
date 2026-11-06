/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLC/types/uint1024.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>

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
            std::lock_guard<std::mutex> lock(mutex);
            Expire(now);
            const auto it = observations.find(peer);
            if(it == observations.end() || it->second.hash != hash
            || it->second.height != height || it->second.address != address)
                ++epoch;
            observations[peer] = {address, hash, height, now};
        }

        void Height(const void* peer, uint32_t height)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = observations.find(peer);
            if(it != observations.end() && it->second.height != height)
            {
                ++epoch;
                it->second.height = height;
            }
        }

        void Remove(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(observations.erase(peer))
                ++epoch;
        }

        template<typename Resolve>
        MiningStatus Check(uint32_t required, bool bypass, bool synchronized,
                           bool reorg, bool shutdown, Resolve resolve,
                           Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
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
                    const auto evidence = resolve(item.second.hash, item.second.height);
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
                ++epoch;
            wasReady = status.ready;
            status.epoch = epoch;
            return status;
        }

    private:
        struct Observation
        {
            std::string address;
            uint1024_t hash;
            uint32_t height;
            Clock::time_point observed;
        };

        void Expire(Clock::time_point now)
        {
            for(auto it = observations.begin(); it != observations.end();)
            {
                if(now - it->second.observed > MAX_AGE)
                {
                    it = observations.erase(it);
                    ++epoch;
                }
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<const void*, Observation> observations;
        uint64_t epoch{1};
        bool wasReady{false};
    };

    void ObserveMiningPeer(const void* peer, const std::string& address,
                           const uint1024_t& hash, uint32_t height);
    void UpdateMiningPeerHeight(const void* peer, uint32_t height);
    MiningStatus MiningReadiness();
    bool MiningWorkCurrent(uint64_t epoch);
}
