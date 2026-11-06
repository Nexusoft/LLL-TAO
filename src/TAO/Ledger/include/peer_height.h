/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <utility>

namespace TAO::Ledger::ChainState
{
    inline bool CanFinalizeSyncFromPeer(uint32_t localHeight, uint32_t peerHeight, uint32_t maxPeerHeight)
    {
        constexpr uint64_t HEIGHT_TOLERANCE = 2;
        return localHeight >= peerHeight
            && (maxPeerHeight == 0 || uint64_t(peerHeight) + HEIGHT_TOLERANCE >= maxPeerHeight);
    }

    class PeerHeightTracker
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto MAX_AGE = std::chrono::minutes(30);

        void Update(const void* peer, uint32_t height, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            PruneExpired(now);
            observations[peer] = {height, now};
        }

        void Remove(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            observations.erase(peer);
        }

        uint32_t Max(Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            PruneExpired(now);
            uint32_t maximum = 0;
            for(const auto& observation : observations)
            {
                if(observation.second.first > maximum)
                    maximum = observation.second.first;
            }
            return maximum;
        }

    private:
        void PruneExpired(Clock::time_point now)
        {
            for(auto it = observations.begin(); it != observations.end();)
            {
                if(now - it->second.second > MAX_AGE)
                    it = observations.erase(it);
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<const void*, std::pair<uint32_t, Clock::time_point>> observations;
    };
}
