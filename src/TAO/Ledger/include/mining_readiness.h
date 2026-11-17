/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLC/types/uint1024.h>
#include <Util/include/debug.h>

#include <chrono>
#include <cstddef>
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
        uint32_t ignored{0};
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

        /* An unresolved peer tip may pause mining only while it can still be
         * resolved by normal block relay or branch recovery. Past this bound it
         * stops vetoing (it never supports), so one stale, forked or lying peer
         * cannot stall template delivery indefinitely. */
        static constexpr auto UNRESOLVED_VETO_MAX = std::chrono::seconds(90);

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

            /* Re-announcing, changing, or reconnecting an unresolved tip must
             * not restart the veto bound. Disconnect removes the connection
             * pointer, so the bound is kept by address until evidence resolves. */
            Observation observation{address, hash, height, now, false};
            if(it != observations.end() && it->second.address == address)
            {
                observation.unresolvedSince = it->second.unresolvedSince;
                observation.vetoExpiredLogged = it->second.vetoExpiredLogged;
            }
            ApplyAddressVeto(observation);
            observations[peer] = observation;
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

        /* Status polls must not resolve peer tips. Mining admission updates this
         * after each evaluation; a poll returns that result without ledger I/O. */
        void RememberStatus(const MiningStatus& status)
        {
            std::lock_guard<std::mutex> lock(mutex);
            lastStatus = status;
            hasLastStatus = true;
        }

        MiningStatus LastStatus()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(!hasLastStatus)
            {
                MiningStatus status;
                status.reason = "not-yet-evaluated";
                return status;
            }
            return lastStatus;
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
            std::set<std::string> unresolvedAddresses;
            std::set<std::string> resolvedAddresses;
            if(!bypass)
            {
                for(auto& item : observations)
                {
                    auto& observation = item.second;
                    const auto evidence = observation.uncertain ? MiningPeerEvidence::UNRESOLVED
                        : resolve(observation.hash, observation.height);
                    if(evidence != MiningPeerEvidence::UNRESOLVED)
                    {
                        observation.unresolvedSince = Clock::time_point{};
                        observation.vetoExpiredLogged = false;
                        resolvedAddresses.insert(observation.address);
                        if(evidence == MiningPeerEvidence::SUPPORTING)
                            supporting.insert(observation.address);
                        continue;
                    }

                    /* A new connection for an address already inside its veto
                     * window continues that window. Only a fully resolved
                     * address may start another one. */
                    ApplyAddressVeto(observation);
                    if(observation.unresolvedSince == Clock::time_point{})
                        observation.unresolvedSince = now;
                    unresolvedAddresses.insert(observation.address);
                    RememberAddressVeto(observation, now);
                    if(now - observation.unresolvedSince < UNRESOLVED_VETO_MAX)
                    {
                        unresolved = true;
                        continue;
                    }

                    ++status.ignored;
                    if(!observation.vetoExpiredLogged)
                    {
                        observation.vetoExpiredLogged = true;
                        RememberAddressVeto(observation, now);
                        debug::log(0, "MiningPeerTracker: ignoring unresolved tip from ",
                                   observation.address, " height=", observation.height,
                                   " hash=", observation.hash.SubString(),
                                   " after veto bound expired");
                    }
                }
                for(const auto& address : resolvedAddresses)
                {
                    if(unresolvedAddresses.count(address) == 0)
                        addressVetoes.erase(address);
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
            Clock::time_point unresolvedSince{};
            bool vetoExpiredLogged{false};
        };

        struct AddressVeto
        {
            Clock::time_point since{};
            bool logged{false};
        };

        /* Disconnect replaces the connection pointer and would otherwise
         * restart the veto. Keep one window per address until that address
         * has no unresolved observation left. */
        void ApplyAddressVeto(Observation& observation) const
        {
            const auto it = addressVetoes.find(observation.address);
            if(it == addressVetoes.end())
                return;

            if(observation.unresolvedSince == Clock::time_point{}
            || it->second.since < observation.unresolvedSince)
                observation.unresolvedSince = it->second.since;
            observation.vetoExpiredLogged = observation.vetoExpiredLogged || it->second.logged;
        }

        void RememberAddressVeto(const Observation& observation, Clock::time_point now)
        {
            auto it = addressVetoes.find(observation.address);
            if(it != addressVetoes.end())
            {
                it->second.since = observation.unresolvedSince;
                it->second.logged = observation.vetoExpiredLogged;
                return;
            }

            if(addressVetoes.size() >= MAX_ADDRESS_VETOES)
                DropExpiredAbsentVetoes(now);
            if(addressVetoes.size() >= MAX_ADDRESS_VETOES)
                return;

            addressVetoes.emplace(observation.address,
                                  AddressVeto{observation.unresolvedSince, observation.vetoExpiredLogged});
        }

        void DropExpiredAbsentVetoes(Clock::time_point now)
        {
            std::set<std::string> live;
            for(const auto& item : observations)
                live.insert(item.second.address);

            for(auto it = addressVetoes.begin(); it != addressVetoes.end();)
            {
                const bool expired = it->second.since != Clock::time_point{}
                    && now - it->second.since >= UNRESOLVED_VETO_MAX;
                if(expired && live.count(it->first) == 0)
                    it = addressVetoes.erase(it);
                else
                    ++it;
            }
        }

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

        static constexpr std::size_t MAX_ADDRESS_VETOES = 4096;

        std::mutex mutex;
        std::map<const void*, Observation> observations;
        std::map<std::string, AddressVeto> addressVetoes;
        using CacheKey = std::tuple<uint1024_t, uint32_t, bool, bool, bool, bool>;
        CacheKey cachedKey;
        MiningStatus cachedStatus;
        MiningStatus lastStatus;
        Clock::time_point refreshAt{}, expireAt{};
        bool cacheValid{false};
        bool hasLastStatus{false};
    };

    void ObserveMiningPeer(const void* peer, const std::string& address,
                           const uint1024_t& hash, uint32_t height);
    void UpdateMiningPeerHeight(const void* peer, uint32_t height);
    void InvalidatePartialMiningPeer(const void* peer, bool hasHash, const uint1024_t& hash,
                                     bool hasHeight, uint32_t height);
    MiningStatus MiningReadiness(bool cached = false);
    /* Last admission result. Does not resolve peer tips or read the ledger. */
    MiningStatus MiningReadinessSnapshot();
    bool MiningWorkCurrent(uint64_t epoch);
}
