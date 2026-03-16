/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GET_BLOCK_POLICY_H
#define NEXUS_LLP_INCLUDE_GET_BLOCK_POLICY_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace LLP
{
    constexpr std::size_t GET_BLOCK_ROLLING_LIMIT_PER_MINUTE = 25;
    constexpr std::chrono::seconds GET_BLOCK_ROLLING_WINDOW = std::chrono::seconds(60);

    enum class GetBlockPolicyReason : uint8_t
    {
        NONE                = 0,
        RATE_LIMIT_EXCEEDED = 1,
        SESSION_INVALID     = 2,
        UNAUTHENTICATED     = 3,
        NO_TEMPLATE_READY   = 4
    };

    inline const char* GetBlockPolicyReasonCode(GetBlockPolicyReason eReason)
    {
        switch(eReason)
        {
            case GetBlockPolicyReason::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
            case GetBlockPolicyReason::SESSION_INVALID:     return "SESSION_INVALID";
            case GetBlockPolicyReason::UNAUTHENTICATED:     return "UNAUTHENTICATED";
            case GetBlockPolicyReason::NO_TEMPLATE_READY:   return "NO_TEMPLATE_READY";
            default:                                        return "NONE";
        }
    }

    /**
     * Lightweight rolling limiter used for GET_BLOCK policy enforcement.
     * Key is caller-provided and should include session + lane + connection identity.
     */
    class GetBlockRollingLimiter
    {
    public:
        using clock = std::chrono::steady_clock;

        explicit GetBlockRollingLimiter(
            std::size_t nMaxRequests = GET_BLOCK_ROLLING_LIMIT_PER_MINUTE,
            std::chrono::seconds nWindow = GET_BLOCK_ROLLING_WINDOW)
        : m_nMaxRequests(nMaxRequests)
        , m_nWindow(nWindow)
        {
        }

        bool Allow(const std::string& strKey, const clock::time_point& now,
                   uint32_t& nRetryAfterMs, std::size_t& nCurrentInWindow)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto& dqRequests = mapRequests[strKey];
            Prune(dqRequests, now, m_nWindow);

            const std::size_t nCurrentSize = dqRequests.size();
            nCurrentInWindow = nCurrentSize;
            if(nCurrentSize >= m_nMaxRequests)
            {
                const auto tRetryAt = dqRequests.front() + m_nWindow;
                if(tRetryAt > now)
                {
                    nRetryAfterMs = static_cast<uint32_t>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(tRetryAt - now).count());
                }
                else
                {
                    nRetryAfterMs = 0;
                }

                return false;
            }

            dqRequests.push_back(now);
            nCurrentInWindow = dqRequests.size();
            nRetryAfterMs = 0;
            return true;
        }

        /** Return current request count in the rolling window for a key. **/
        std::size_t Snapshot(const std::string& strKey, const clock::time_point& now)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapRequests.find(strKey);
            if(it == mapRequests.end())
                return 0;

            Prune(it->second, now, m_nWindow);
            if(it->second.empty())
            {
                mapRequests.erase(it);
                return 0;
            }

            return it->second.size();
        }

    private:
        static void Prune(std::deque<clock::time_point>& dqRequests, const clock::time_point& now,
                          const std::chrono::seconds& nWindow)
        {
            while(!dqRequests.empty() && (now - dqRequests.front()) >= nWindow)
                dqRequests.pop_front();
        }

        std::mutex MUTEX;
        const std::size_t m_nMaxRequests;
        const std::chrono::seconds m_nWindow;
        std::unordered_map<std::string, std::deque<clock::time_point>> mapRequests;
    };
}

#endif
