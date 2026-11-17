/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <utility>

namespace TAO::Ledger::ChainState
{
    /* Shared with PeerHeightTracker::MaxSupported(). A lone advertisement
     * outside this window is not chain evidence. */
    inline constexpr uint64_t PEER_HEIGHT_TOLERANCE = 2;

    /* Genesis block time (src/TAO/Ledger/genesis_block.cpp). One block per
     * second since then, plus a year of slack, is faster than any channel
     * schedule. Heights above that — including UINT32_MAX — are unsupported. */
    inline constexpr uint64_t PEER_HEIGHT_GENESIS_TIME = 1409456199ULL;
    inline constexpr uint64_t PEER_HEIGHT_UNREACHABLE_SLACK = 86400ULL * 365;

    inline bool IsUnsupportedPeerHeight(uint32_t height)
    {
        if(height == std::numeric_limits<uint32_t>::max())
            return true;

        const uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if(now <= PEER_HEIGHT_GENESIS_TIME)
            return height > 1;

        const uint64_t reachable =
            (now - PEER_HEIGHT_GENESIS_TIME) + PEER_HEIGHT_UNREACHABLE_SLACK;
        return uint64_t(height) > reachable;
    }

    /* maxPeerHeight must be a supported height (see MaxSupportedPeerHeight).
     * Unreachable advertisements are quarantined here as well, so a raw
     * UINT32_MAX watermark cannot veto completion when the local tip matches
     * the sync peer. Repeating the advertisement does not change that. */
    inline bool CanFinalizeSyncFromPeer(uint32_t localHeight, uint32_t peerHeight, uint32_t maxPeerHeight)
    {
        if(maxPeerHeight != 0 && maxPeerHeight != localHeight
        && IsUnsupportedPeerHeight(maxPeerHeight))
            maxPeerHeight = 0;

        return localHeight >= peerHeight
            && (maxPeerHeight == 0
                || uint64_t(peerHeight) + PEER_HEIGHT_TOLERANCE >= maxPeerHeight);
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

        /* Greatest height that may hard-gate sync completion.
         * Includes a height only when it is within PEER_HEIGHT_TOLERANCE of
         * the committed local tip, or at least two peers advertise heights
         * within that tolerance of each other. Unreachable outliers are
         * dropped even if corroborated. Re-announcing a lone outlier refreshes
         * its expiry but does not create a veto. */
        uint32_t MaxSupported(uint32_t localHeight, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            PruneExpired(now);

            uint32_t maximum = 0;
            for(const auto& observation : observations)
            {
                const uint32_t height = observation.second.first;
                if(IsUnsupportedPeerHeight(height))
                    continue;

                const bool fLocal =
                    uint64_t(height) <= uint64_t(localHeight) + PEER_HEIGHT_TOLERANCE;
                if(!fLocal && !IsCorroborated(height))
                    continue;

                if(height > maximum)
                    maximum = height;
            }
            return maximum;
        }

        bool Get(const void* peer, uint32_t& height, Clock::time_point now = Clock::now(),
                 Clock::duration maxAge = MAX_AGE)
        {
            std::lock_guard<std::mutex> lock(mutex);
            PruneExpired(now);
            const auto it = observations.find(peer);
            if(it == observations.end() || now - it->second.second > maxAge)
                return false;
            height = it->second.first;
            return true;
        }

    private:
        bool IsCorroborated(uint32_t height) const
        {
            uint32_t nSupporters = 0;
            for(const auto& observation : observations)
            {
                const uint32_t other = observation.second.first;
                const uint64_t nDelta = other > height
                    ? uint64_t(other) - height
                    : uint64_t(height) - other;
                if(nDelta <= PEER_HEIGHT_TOLERANCE)
                    ++nSupporters;
            }
            return nSupporters >= 2;
        }

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
