/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <map>
#include <mutex>
#include <vector>

namespace LLP
{
    class BlockInventoryRequests
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto TIMEOUT = std::chrono::seconds(5);
        static constexpr std::size_t MAX_HASHES = 4096;
        static constexpr std::size_t MAX_PEERS_PER_HASH = 32;
        /* Partition the budget so one advertising peer cannot starve another.
         * Deferred inventory has its own bounded, peer-local FIFO. */
        static constexpr std::size_t MAX_PEERS = 512;
        static constexpr std::size_t MAX_HASHES_PER_PEER = MAX_HASHES / MAX_PEERS;
        static constexpr std::size_t MAX_PENDING_PER_PEER = 128;
        enum class Result { Added, Duplicate, Capacity };

        bool Add(const uint1024_t& hash, const void* peer, Clock::time_point now = Clock::now())
        {
            return Reserve(hash, peer, now) == Result::Added;
        }

        Result Reserve(const uint1024_t& hash, const void* peer, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            Prune(now);
            auto it = requests.find(peer);
            if(it == requests.end())
            {
                if(requests.size() >= MAX_PEERS)
                    return Result::Capacity;
                it = requests.emplace(peer, PeerRequests{}).first;
            }
            auto& state = it->second;
            if(state.active.count(hash))
                return Result::Duplicate;
            if(state.active.size() >= MAX_HASHES_PER_PEER)
                return Result::Capacity;
            std::size_t peers = 0;
            for(const auto& entry : requests)
                peers += entry.second.active.count(hash);
            if(peers >= MAX_PEERS_PER_HASH)
                return Result::Capacity;
            state.active.emplace(hash, now + TIMEOUT);
            const auto pending = std::find(state.pending.begin(), state.pending.end(), hash);
            if(pending != state.pending.end())
                state.pending.erase(pending);
            return Result::Added;
        }

        bool Defer(const uint1024_t& hash, const void* peer, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            Prune(now);
            auto it = requests.find(peer);
            if(it == requests.end())
            {
                if(requests.size() >= MAX_PEERS)
                    return false;
                it = requests.emplace(peer, PeerRequests{}).first;
            }
            auto& state = it->second;
            if(state.active.count(hash)
            || std::find(state.pending.begin(), state.pending.end(), hash) != state.pending.end())
                return true;
            if(state.pending.size() >= MAX_PENDING_PER_PEER)
                return false;
            if(state.pending.empty())
                state.retryAt = now + TIMEOUT;
            state.pending.push_back(hash);
            return true;
        }

        std::vector<uint1024_t> Pending(const void* peer, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = requests.find(peer);
            if(it == requests.end() || now < it->second.retryAt)
                return {};
            it->second.retryAt = now + TIMEOUT;
            const auto& pending = it->second.pending;
            return {pending.begin(), pending.end()};
        }

        bool Contains(const uint1024_t& hash, const void* peer, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = requests.find(peer);
            if(it == requests.end())
                return false;
            PrunePeer(it->second, now);
            if(!Empty(it->second))
                return it->second.active.count(hash) != 0;
            requests.erase(it);
            return false;
        }

        void Remove(const uint1024_t& hash, const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = requests.find(peer);
            if(it == requests.end())
                return;
            it->second.active.erase(hash);
            auto& pending = it->second.pending;
            const auto deferred = std::find(pending.begin(), pending.end(), hash);
            if(deferred != pending.end())
                pending.erase(deferred);
            if(Empty(it->second))
                requests.erase(it);
        }

        void RemovePeer(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            requests.erase(peer);
        }

    private:
        struct PeerRequests
        {
            std::map<uint1024_t, Clock::time_point> active;
            std::deque<uint1024_t> pending;
            Clock::time_point retryAt{};
        };

        static bool Empty(const PeerRequests& peer)
        {
            return peer.active.empty() && peer.pending.empty();
        }

        static void PrunePeer(PeerRequests& peer, Clock::time_point now)
        {
            for(auto it = peer.active.begin(); it != peer.active.end();)
            {
                if(it->second <= now)
                    it = peer.active.erase(it);
                else
                    ++it;
            }
        }

        void Prune(Clock::time_point now)
        {
            for(auto it = requests.begin(); it != requests.end();)
            {
                PrunePeer(it->second, now);
                if(Empty(it->second))
                    it = requests.erase(it);
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<const void*, PeerRequests> requests;
    };
}
