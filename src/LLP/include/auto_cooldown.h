/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_AUTO_COOLDOWN_H
#define NEXUS_LLP_INCLUDE_AUTO_COOLDOWN_H

#include <chrono>

namespace LLP
{

    /** AutoCoolDown
     *
     *  Rate-limit guard for repeated requests.
     *
     *  Replaces ad-hoc magic-number cooldown comments with a self-contained
     *  object that tracks elapsed time and provides a clean API.
     *
     *  Intended for per-connection use (e.g. GET_BLOCK safety-net cooldown).
     *  Not thread-safe on its own — callers must hold an appropriate mutex.
     *
     *  Usage:
     *    AutoCoolDown cd(std::chrono::seconds(200));
     *    if (!cd.Ready()) return;   // still cooling down
     *    cd.Reset();                // start new cooldown
     *
     **/
    class AutoCoolDown
    {
    public:

        /** Constructor
         *
         *  @param[in] duration Cooldown duration.
         *
         **/
        explicit AutoCoolDown(std::chrono::milliseconds duration)
            : m_duration(duration)
            , m_last{}
        {
        }


        /** Ready
         *
         *  Returns true when the cooldown has elapsed (or has never been started).
         *
         *  @return true if ready (no active cooldown), false otherwise.
         *
         **/
        bool Ready() const
        {
            if(m_last == std::chrono::steady_clock::time_point{})
                return true;

            return (std::chrono::steady_clock::now() - m_last) >= m_duration;
        }


        /** Reset
         *
         *  Starts (or restarts) the cooldown from now.
         *
         **/
        void Reset()
        {
            m_last = std::chrono::steady_clock::now();
        }


        /** Remaining
         *
         *  Returns the time remaining in the current cooldown.
         *  Returns zero if the cooldown has elapsed or was never started.
         *
         *  @return Milliseconds remaining until cooldown expires.
         *
         **/
        std::chrono::milliseconds Remaining() const
        {
            if(Ready())
                return std::chrono::milliseconds(0);

            auto elapsed = std::chrono::steady_clock::now() - m_last;
            return std::chrono::duration_cast<std::chrono::milliseconds>(m_duration - elapsed);
        }

    private:

        /** Cooldown duration **/
        std::chrono::milliseconds m_duration;

        /** Time of last Reset() call (epoch means "never reset") **/
        std::chrono::steady_clock::time_point m_last;
    };

} // namespace LLP

#endif
