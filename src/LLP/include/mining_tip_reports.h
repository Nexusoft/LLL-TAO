/* Distributed under the MIT software license, see the accompanying COPYING. */
#pragma once

#include <LLC/types/uint1024.h>

#include <chrono>
#include <cstdint>

namespace LLP
{
    /* Older peers answer a tip subscription with two separate notifications. */
    class MiningTipReports
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto PAIR_WINDOW = std::chrono::seconds(10);

        bool Observe(bool hasHash, const uint1024_t& hash, bool hasHeight, uint32_t height,
                     uint1024_t& pairedHash, uint32_t& pairedHeight,
                     Clock::time_point now = Clock::now())
        {
            if(!hasHash && !hasHeight)
                return false;
            if(hasHash && hasHeight)
            {
                Reset();
                pairedHash = hash;
                pairedHeight = height;
                return true;
            }
            if((pendingHash || pendingHeight) && now - started > PAIR_WINDOW)
                Reset();
            if(!pendingHash && !pendingHeight)
                started = now;
            if(hasHash)
            {
                pendingHash = true;
                savedHash = hash;
            }
            if(hasHeight)
            {
                pendingHeight = true;
                savedHeight = height;
            }
            if(!pendingHash || !pendingHeight)
                return false;
            pairedHash = savedHash;
            pairedHeight = savedHeight;
            Reset();
            return true;
        }

        void Reset()
        {
            pendingHash = false;
            pendingHeight = false;
        }

    private:
        bool pendingHash{false};
        bool pendingHeight{false};
        uint1024_t savedHash{0};
        uint32_t savedHeight{0};
        Clock::time_point started{};
    };
}
