/* Distributed under the MIT software license, see COPYING. */
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace LLP
{
    inline bool CanAttemptOutgoing(uint32_t connections, uint32_t maximum)
    {
        return connections < maximum;
    }

    template<typename T, typename Random>
    std::shared_ptr<T> SelectConnectedPeer(const std::vector<std::shared_ptr<T>>& peers, Random random)
    {
        std::vector<std::shared_ptr<T>> connected;
        for(const auto& peer : peers)
        {
            if(peer && peer->Connected())
                connected.push_back(peer);
        }
        if(connected.empty())
            return nullptr;
        auto selected = connected.at(random(connected.size()));
        return selected->Connected() ? selected : nullptr;
    }

    /* All times are monotonic seconds; callers supply jitter in [0, 1000]. */
    class PeerRecovery
    {
    public:
        void Observe(uint64_t now, bool healthy)
        {
            if(healthy && !wasHealthy)
            {
                attempts = 0;
                nextAttempt = std::min(nextAttempt, now + 15);
            }
            wasHealthy = healthy;
        }

        bool Due(uint64_t now, bool enabled, bool suspended, bool capacity,
                 bool periodic, bool degraded, bool requested)
        {
            if(!enabled || suspended)
            {
                degradedSince = 0;
                return false;
            }

            if(degraded)
            {
                if(degradedSince == 0)
                    degradedSince = now + 1;
            }
            else
                degradedSince = 0;

            const bool prolonged = degradedSince != 0 && now >= degradedSince - 1
                && now - (degradedSince - 1) >= 30;
            return now >= nextAttempt && (periodic || (capacity && (prolonged || requested)));
        }

        void Completed(uint64_t now, uint32_t jitter, bool healthy, bool success)
        {
            if(healthy && success)
                attempts = 0;
            else
                attempts = std::min(attempts + 1, uint32_t(6));

            const uint64_t base = std::min(uint64_t(300), uint64_t(15) << std::min(attempts ? attempts - 1 : 0, uint32_t(5)));
            const uint64_t delay = std::min(uint64_t(300), base * (800 + std::min(jitter, uint32_t(1000)) * 400 / 1000) / 1000);
            nextAttempt = now + std::max(uint64_t(1), delay);
        }

        uint64_t NextAttempt() const { return nextAttempt; }

    private:
        uint64_t nextAttempt = 0;
        uint64_t degradedSince = 0;
        uint32_t attempts = 0;
        bool wasHealthy = false;
    };
}
