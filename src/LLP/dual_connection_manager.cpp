/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/dual_connection_manager.h>
#include <Util/include/debug.h>
#include <algorithm>

namespace LLP
{

    /* ── Singleton ───────────────────────────────────────────────────────── */

    DualConnectionManager& DualConnectionManager::Get()
    {
        static DualConnectionManager instance;
        return instance;
    }


    /* ── Constructor ─────────────────────────────────────────────────────── */

    DualConnectionManager::DualConnectionManager()
    : m_mutex()
    , m_stateless_lane_alive(false)
    , m_legacy_lane_alive(false)
    , m_using_failover(false)
    , m_primary_fail_count(0)
    , m_active_endpoint()
    , m_last_state_change(std::chrono::steady_clock::now())
    {
    }


    /* ── Public API ──────────────────────────────────────────────────────── */

    void DualConnectionManager::set_lane_alive(bool is_stateless)
    {
        if(is_stateless)
        {
            bool was_dead = !m_stateless_lane_alive.load();
            m_stateless_lane_alive.store(true);

            if(was_dead)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_last_state_change = std::chrono::steady_clock::now();
                debug::log(0, FUNCTION, "[DualConnectionManager] STATELESS lane (9323) is now ALIVE");
            }
        }
        else
        {
            bool was_dead = !m_legacy_lane_alive.load();
            m_legacy_lane_alive.store(true);

            if(was_dead)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_last_state_change = std::chrono::steady_clock::now();
                debug::log(0, FUNCTION, "[DualConnectionManager] LEGACY lane (8323) is now ALIVE");
            }
        }

        /* Check for SIM Link activation */
        if(is_sim_link_active())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            debug::log(0, FUNCTION, "[DualConnectionManager] SIM Link ACTIVE: Both lanes alive on ",
                       m_active_endpoint.empty() ? "unknown endpoint" : m_active_endpoint);
        }
    }


    void DualConnectionManager::set_lane_dead(bool is_stateless)
    {
        if(is_stateless)
        {
            bool was_alive = m_stateless_lane_alive.load();
            m_stateless_lane_alive.store(false);

            if(was_alive)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_last_state_change = std::chrono::steady_clock::now();
                debug::log(0, FUNCTION, "[DualConnectionManager] STATELESS lane (9323) is now DEAD");
            }
        }
        else
        {
            bool was_alive = m_legacy_lane_alive.load();
            m_legacy_lane_alive.store(false);

            if(was_alive)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_last_state_change = std::chrono::steady_clock::now();
                debug::log(0, FUNCTION, "[DualConnectionManager] LEGACY lane (8323) is now DEAD");
            }
        }

        /* Check if both lanes are down — triggers failover logic in retry_connect() */
        if(!m_stateless_lane_alive.load() && !m_legacy_lane_alive.load())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            debug::log(0, FUNCTION, "[DualConnectionManager] BOTH lanes DOWN on ",
                       m_active_endpoint.empty() ? "unknown endpoint" : m_active_endpoint,
                       " — failover may activate");
        }
    }


    void DualConnectionManager::set_failover_active(bool using_failover, const std::string& endpoint)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        bool state_changed = (m_using_failover != using_failover);
        m_using_failover = using_failover;
        m_active_endpoint = endpoint;
        m_last_state_change = std::chrono::steady_clock::now();

        if(state_changed)
        {
            debug::log(0, FUNCTION, "[DualConnectionManager] FAILOVER STATE CHANGED: now using ",
                       using_failover ? "FAILOVER" : "PRIMARY", " node at ", endpoint);
        }
        else
        {
            debug::log(1, FUNCTION, "[DualConnectionManager] Active endpoint updated: ", endpoint,
                       " (", using_failover ? "failover" : "primary", " node)");
        }
    }


    bool DualConnectionManager::is_using_failover() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_using_failover;
    }


    std::string DualConnectionManager::get_active_endpoint() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_active_endpoint;
    }


    std::string DualConnectionManager::get_secondary_endpoint() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if(m_active_endpoint.empty())
            return "";

        /* Derive secondary (LEGACY) endpoint from primary (STATELESS) endpoint
         * Replace port 9323 with 8323 */
        std::string secondary = m_active_endpoint;
        size_t colon_pos = secondary.rfind(':');
        if(colon_pos != std::string::npos)
        {
            secondary = secondary.substr(0, colon_pos + 1) + "8323";
        }

        return secondary;
    }


    bool DualConnectionManager::is_stateless_lane_alive() const
    {
        return m_stateless_lane_alive.load();
    }


    bool DualConnectionManager::is_legacy_lane_alive() const
    {
        return m_legacy_lane_alive.load();
    }


    bool DualConnectionManager::is_sim_link_active() const
    {
        return m_stateless_lane_alive.load() && m_legacy_lane_alive.load();
    }


    uint32_t DualConnectionManager::get_primary_fail_count() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_primary_fail_count;
    }


    void DualConnectionManager::increment_fail_count()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_primary_fail_count++;
        debug::log(1, FUNCTION, "[DualConnectionManager] Fail count incremented to ",
                   m_primary_fail_count, " for ", m_using_failover ? "failover" : "primary", " node");
    }


    void DualConnectionManager::reset_fail_count()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_primary_fail_count > 0)
        {
            debug::log(0, FUNCTION, "[DualConnectionManager] Fail count RESET (was ",
                       m_primary_fail_count, ") — connection successful");
        }
        m_primary_fail_count = 0;
    }

} // namespace LLP
