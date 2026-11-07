/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <chrono>
#include <cstddef>
#include <map>
#include <mutex>

namespace LLP
{
    class BlockInventoryRequests
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto TIMEOUT = std::chrono::seconds(5);
        static constexpr std::size_t MAX_HASHES = 4096;
        static constexpr std::size_t MAX_PEERS_PER_HASH = 32;

        bool Add(const uint1024_t& hash, const void* peer, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = requests.find(hash);
            if(it == requests.end())
            {
                if(requests.size() >= MAX_HASHES)
                {
                    Prune(now);
                    if(requests.size() >= MAX_HASHES)
                        return false;
                }
                it = requests.emplace(hash, PeerRequests{}).first;
            }
            PrunePeers(it->second, now);
            if(it->second.size() >= MAX_PEERS_PER_HASH && !it->second.count(peer))
                return false;
            it->second[peer] = now + TIMEOUT;
            return true;
        }

        bool Contains(const uint1024_t& hash, Clock::time_point now = Clock::now())
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = requests.find(hash);
            if(it == requests.end())
                return false;
            PrunePeers(it->second, now);
            if(!it->second.empty())
                return true;
            requests.erase(it);
            return false;
        }

        void Remove(const uint1024_t& hash, const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = requests.find(hash);
            if(it == requests.end())
                return;
            it->second.erase(peer);
            if(it->second.empty())
                requests.erase(it);
        }

        void RemovePeer(const void* peer)
        {
            std::lock_guard<std::mutex> lock(mutex);
            for(auto it = requests.begin(); it != requests.end();)
            {
                it->second.erase(peer);
                if(it->second.empty())
                    it = requests.erase(it);
                else
                    ++it;
            }
        }

    private:
        using PeerRequests = std::map<const void*, Clock::time_point>;

        static void PrunePeers(PeerRequests& peers, Clock::time_point now)
        {
            for(auto it = peers.begin(); it != peers.end();)
            {
                if(it->second <= now)
                    it = peers.erase(it);
                else
                    ++it;
            }
        }

        void Prune(Clock::time_point now)
        {
            for(auto it = requests.begin(); it != requests.end();)
            {
                PrunePeers(it->second, now);
                if(it->second.empty())
                    it = requests.erase(it);
                else
                    ++it;
            }
        }

        std::mutex mutex;
        std::map<uint1024_t, PeerRequests> requests;
    };
}
