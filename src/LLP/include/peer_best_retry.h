/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#pragma once

#include <LLP/include/block_inventory_requests.h>

#include <chrono>

namespace LLP
{
    class PeerBestRetry
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr auto INTERVAL = std::chrono::seconds(30);
        static constexpr auto MAX_AGE = std::chrono::minutes(30);

        void Observe(bool changed, Clock::time_point now = Clock::now())
        {
            if(!retryScheduled)
            {
                /* Tip churn must not extend a continuously unresolved mismatch. */
                nextUnresolvedRetry = now + INTERVAL;
                nextBlockRetry = now + BlockInventoryRequests::TIMEOUT;
            }
            if(changed || !retryScheduled)
                nextRetry = now + INTERVAL;
            retryScheduled = true;
            observed = true;
            lastObserved = now;
        }

        bool Due(Clock::time_point now = Clock::now())
        {
            if(!retryScheduled || !Fresh(now)
            || (now < nextRetry && now < nextUnresolvedRetry))
                return false;
            nextRetry = now + INTERVAL;
            nextUnresolvedRetry = now + INTERVAL;
            return true;
        }

        bool Fresh(Clock::time_point now = Clock::now(), Clock::duration maxAge = MAX_AGE) const
        {
            return observed && now - lastObserved <= maxAge;
        }

        bool BlockDue(Clock::time_point now = Clock::now())
        {
            if(!retryScheduled || !Fresh(now) || now < nextBlockRetry)
                return false;
            nextBlockRetry = now + BlockInventoryRequests::TIMEOUT;
            return true;
        }

        bool RefreshDue(Clock::time_point now = Clock::now())
        {
            if(refreshScheduled && now < nextRefresh)
                return false;
            refreshScheduled = true;
            nextRefresh = now + INTERVAL;
            return true;
        }

        void Resolved()
        {
            retryScheduled = false;
        }

        void Reset()
        {
            Resolved();
            observed = false;
            refreshScheduled = false;
        }

    private:
        bool observed{false};
        bool retryScheduled{false};
        Clock::time_point lastObserved{};
        Clock::time_point nextRetry{};
        Clock::time_point nextUnresolvedRetry{};
        Clock::time_point nextBlockRetry{};
        bool refreshScheduled{false};
        Clock::time_point nextRefresh{};
    };
}
