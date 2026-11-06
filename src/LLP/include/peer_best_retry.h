/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <chrono>

namespace LLP
{
    class PeerBestRetry
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto INTERVAL = std::chrono::seconds(30);
        static constexpr auto MAX_AGE = std::chrono::minutes(30);

        void Observe(Clock::time_point now = Clock::now())
        {
            if(!observed)
                nextRetry = now + INTERVAL;
            observed = true;
            lastObserved = now;
        }

        bool Due(Clock::time_point now = Clock::now())
        {
            if(!Fresh(now) || now < nextRetry)
                return false;
            nextRetry = now + INTERVAL;
            return true;
        }

        bool Fresh(Clock::time_point now = Clock::now(), Clock::duration maxAge = MAX_AGE) const
        {
            return observed && now - lastObserved <= maxAge;
        }

        bool RefreshDue(Clock::time_point now = Clock::now())
        {
            if(refreshScheduled && now < nextRefresh)
                return false;
            refreshScheduled = true;
            nextRefresh = now + INTERVAL;
            return true;
        }

        void Reset()
        {
            observed = false;
            refreshScheduled = false;
        }

    private:
        bool observed{false};
        Clock::time_point lastObserved{};
        Clock::time_point nextRetry{};
        bool refreshScheduled{false};
        Clock::time_point nextRefresh{};
    };
}
