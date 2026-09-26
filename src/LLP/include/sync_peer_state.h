/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>

namespace LLP
{
    /** Tracks bounded LIST/SYNC requests without introducing wire request identifiers. */
    class SyncPeerState
    {
        struct Peer
        {
            bool fPending = false;
            bool fActive = false;
            bool fCooling = false;
            uint64_t nLastProgress = 0;
            uint64_t nPausedAt = 0;
        };

        mutable std::mutex MUTEX;
        std::map<uint64_t, Peer> mapPeers;
        std::size_t nActive = 0;

        void Retire(Peer& peer, const uint64_t now)
        {
            if(peer.fActive)
                --nActive;

            peer.fActive = false;
            peer.fCooling = true;
            peer.nPausedAt = now;
        }

    public:
        bool Begin(const uint64_t session, const uint64_t now, const std::size_t maxPeers)
        {
            if(session == 0 || maxPeers == 0)
                return false;

            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it != mapPeers.end())
            {
                Peer& peer = it->second;
                if(peer.fPending || peer.fActive)
                    return false;

                if(peer.fCooling)
                {
                    /* Rebase a rolled-back clock rather than underflow or wait for it to catch up. */
                    if(now < peer.nPausedAt)
                        peer.nPausedAt = now;

                    if(now - peer.nPausedAt < 30)
                        return false;
                }
            }

            if(nActive >= maxPeers)
                return false;

            Peer& peer = mapPeers[session];
            peer.fPending = true;
            peer.fActive = true;
            peer.fCooling = false;
            peer.nLastProgress = now;
            ++nActive;
            return true;
        }

        /** Drain the response tail, including one belonging to an already retired request. */
        bool Complete(const uint64_t session)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it == mapPeers.end())
                return false;

            Peer& peer = it->second;
            const bool fWasActive = peer.fActive;
            if(fWasActive)
                --nActive;

            peer.fPending = false;
            peer.fActive = false;
            return fWasActive;
        }

        bool Active(const uint64_t session) const
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            return it != mapPeers.end() && it->second.fActive && it->second.fPending;
        }

        /** Retired streams still own the response tail and block other LIST requests. */
        bool Pending(const uint64_t session) const
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            return it != mapPeers.end() && it->second.fPending;
        }

        void Progress(const uint64_t session, const uint64_t now)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it != mapPeers.end() && it->second.fActive && it->second.fPending)
                it->second.nLastProgress = now;
        }

        bool Expire(const uint64_t session, const uint64_t now, const uint64_t timeout = 60)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it == mapPeers.end() || !it->second.fActive || !it->second.fPending)
                return false;

            Peer& peer = it->second;
            if(now < peer.nLastProgress)
            {
                peer.nLastProgress = now;
                return false;
            }

            if(now - peer.nLastProgress < timeout)
                return false;

            Retire(peer, now);
            return true;
        }

        /** Keep pending set until the old response tail drains; cooldown alone is not sufficient. */
        void Pause(const uint64_t session, const uint64_t now)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it != mapPeers.end())
                Retire(it->second, now);
        }

        /** Finish synchronization without releasing ownership of outstanding response tails. */
        void RetireAll(const uint64_t now)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            for(auto& entry : mapPeers)
            {
                if(entry.second.fActive)
                    Retire(entry.second, now);
            }
        }

        void Remove(const uint64_t session)
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            const auto it = mapPeers.find(session);
            if(it == mapPeers.end())
                return;

            if(it->second.fActive)
                --nActive;

            mapPeers.erase(it);
        }

        void Clear()
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            mapPeers.clear();
            nActive = 0;
        }

        uint64_t Primary() const
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            for(const auto& entry : mapPeers)
            {
                if(entry.second.fActive && entry.second.fPending)
                    return entry.first;
            }

            return 0;
        }

        std::size_t Size() const
        {
            const std::lock_guard<std::mutex> lock(MUTEX);
            return nActive;
        }
    };
}
