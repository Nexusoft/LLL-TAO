/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_DUAL_CONNECTION_MANAGER_H
#define NEXUS_LLP_INCLUDE_DUAL_CONNECTION_MANAGER_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <cstdint>

namespace LLP
{
    /** DualConnectionManager
     *
     *  Manages dual-connection SIM Link architecture and failover logic for mining nodes.
     *
     *  DESIGN OVERVIEW:
     *  - Tracks liveness of STATELESS (primary, port 9323) and LEGACY (secondary, port 8323) lanes
     *  - Handles automatic failover to backup node when primary node drops
     *  - Coordinates retry logic with exponential backoff
     *  - Ensures SIM Link secondary connection follows active node endpoint
     *
     *  FAILOVER BEHAVIOR:
     *  1. Primary 9323 drops → SIM Link: secondary 8323 keeps workers alive; GET_BLOCK sent on legacy lane
     *  2. Both ports on primary drop → Failover: switch to failover node:9323, SIM Link re-establishes on failover
     *  3. Failover also drops (N retries) → Cycle back to primary, exponential backoff
     *  4. Any successful connect → Reset fail counter, preserve m_using_failover flag for correct side tracking
     *
     *  USAGE:
     *  - Call set_lane_alive() when connections succeed
     *  - Call set_lane_dead() when connections drop
     *  - Call set_failover_active() when failover state changes
     *  - Query is_using_failover() to determine which node is active
     *  - Query get_active_endpoint() to get current primary node IP
     *  - Query get_secondary_endpoint() to get current secondary (legacy) node IP
     *
     **/
    class DualConnectionManager
    {
    public:
        /** Get the global singleton instance **/
        static DualConnectionManager& Get();

        /** set_lane_alive
         *
         *  Mark a connection lane as alive/healthy.
         *  Resets fail counters on successful connection.
         *
         *  @param[in] is_stateless  True for STATELESS lane (9323), false for LEGACY lane (8323)
         *
         **/
        void set_lane_alive(bool is_stateless);

        /** set_lane_dead
         *
         *  Mark a connection lane as dead/unhealthy.
         *  Triggers failover logic if both lanes are down.
         *
         *  @param[in] is_stateless  True for STATELESS lane (9323), false for LEGACY lane (8323)
         *
         **/
        void set_lane_dead(bool is_stateless);

        /** set_failover_active
         *
         *  Set the active node endpoint (primary or failover).
         *  Called by retry_connect() when failover state changes.
         *
         *  @param[in] using_failover  True if using failover node, false if using primary
         *  @param[in] endpoint        IP:port of the active primary node (e.g., "192.168.1.10:9323" or "192.168.1.11:9323")
         *
         **/
        void set_failover_active(bool using_failover, const std::string& endpoint);

        /** is_using_failover
         *
         *  Check if currently using the failover node.
         *
         *  @return True if failover node is active, false if primary node is active
         *
         **/
        bool is_using_failover() const;

        /** get_active_endpoint
         *
         *  Get the currently active primary node endpoint.
         *  Returns the IP:port string for the active STATELESS lane connection.
         *
         *  @return Active node IP:port string (e.g., "192.168.1.10:9323")
         *
         **/
        std::string get_active_endpoint() const;

        /** get_secondary_endpoint
         *
         *  Get the secondary (LEGACY) lane endpoint for the active node.
         *  Automatically derives the LEGACY port (8323) from the active primary endpoint.
         *
         *  @return Secondary node IP:port string (e.g., "192.168.1.10:8323")
         *
         **/
        std::string get_secondary_endpoint() const;

        /** is_stateless_lane_alive
         *
         *  Check if the STATELESS lane (primary, port 9323) is currently alive.
         *
         *  @return True if STATELESS lane is alive
         *
         **/
        bool is_stateless_lane_alive() const;

        /** is_legacy_lane_alive
         *
         *  Check if the LEGACY lane (secondary, port 8323) is currently alive.
         *
         *  @return True if LEGACY lane is alive
         *
         **/
        bool is_legacy_lane_alive() const;

        /** is_sim_link_active
         *
         *  Check if both lanes are active (SIM Link mode).
         *
         *  @return True if both STATELESS and LEGACY lanes are alive
         *
         **/
        bool is_sim_link_active() const;

        /** get_primary_fail_count
         *
         *  Get the number of consecutive primary node connection failures.
         *
         *  @return Primary fail counter value
         *
         **/
        uint32_t get_primary_fail_count() const;

        /** increment_fail_count
         *
         *  Increment the fail counter for the currently active node.
         *  Used by retry_connect() to track consecutive failures.
         *
         **/
        void increment_fail_count();

        /** reset_fail_count
         *
         *  Reset the fail counter to zero (called on successful connection).
         *
         **/
        void reset_fail_count();

    private:
        /** Private constructor — use Get() for singleton access **/
        DualConnectionManager();

        /** Destructor **/
        ~DualConnectionManager() = default;

        /* Non-copyable */
        DualConnectionManager(const DualConnectionManager&) = delete;
        DualConnectionManager& operator=(const DualConnectionManager&) = delete;

        /** Mutex for thread-safe state updates **/
        mutable std::mutex m_mutex;

        /** Lane liveness tracking **/
        std::atomic<bool> m_stateless_lane_alive{false};  // STATELESS lane (9323) status
        std::atomic<bool> m_legacy_lane_alive{false};     // LEGACY lane (8323) status

        /** Failover state **/
        bool m_using_failover{false};                     // True if currently using failover node
        uint32_t m_primary_fail_count{0};                 // Consecutive failures of active node

        /** Active endpoint tracking **/
        std::string m_active_endpoint;                    // Current primary node IP:port (e.g., "192.168.1.10:9323")

        /** Last state change timestamp (for logging) **/
        std::chrono::steady_clock::time_point m_last_state_change;
    };

} // namespace LLP

#endif
