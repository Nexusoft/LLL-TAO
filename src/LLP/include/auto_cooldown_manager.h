/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_AUTO_COOLDOWN_MANAGER_H
#define NEXUS_LLP_INCLUDE_AUTO_COOLDOWN_MANAGER_H

#include <LLP/include/base_address.h>
#include <Util/include/mutex.h>

#include <map>
#include <cstdint>

namespace LLP
{
    /** AutoCooldownManager
     *
     *  Manages auto-expiring cooldowns for mining connections.
     *  
     *  SECURITY GUARANTEES:
     *  - Fully automated: NO manual add/remove capability exposed to operators
     *  - Not persistent: Cooldowns cleared on node restart (memory-only)
     *  - Auto-expiring: Cooldowns automatically removed after duration
     *  - Transparent: Clear logging of all cooldown actions
     *  - Fair: Same automated rules for all IPs
     *
     *  This is NOT a ban system. Cooldowns are temporary, reversible, and
     *  automatically expire. This prevents permanent blocking while protecting
     *  against spam attacks.
     *
     **/
    class AutoCooldownManager
    {
    private:
        /** IP address (IPv4) -> expiry timestamp (auto-expires) **/
        std::map<uint32_t, uint64_t> m_cooldowns;
        
        /** Mutex for thread-safe cooldown map access **/
        mutable std::mutex m_mutex;
        
        /** Private constructor for singleton pattern **/
        AutoCooldownManager() = default;
        
        /** Delete copy constructor and assignment operator **/
        AutoCooldownManager(const AutoCooldownManager&) = delete;
        AutoCooldownManager& operator=(const AutoCooldownManager&) = delete;

    public:
        /** Get
         *
         *  Get the global manager instance (singleton).
         *
         *  @return Reference to the singleton manager
         *
         **/
        static AutoCooldownManager& Get();

        /** AddCooldown
         *
         *  @brief Add IP to auto-expiring cooldown (AUTOMATED ONLY)
         * 
         *  This is NOT a ban. It's a temporary cooldown that:
         *  - Auto-expires after duration
         *  - Is cleared on node restart
         *  - Cannot be manually extended or made permanent
         * 
         *  @param[in] addr IP address to cooldown
         *  @param[in] nDurationSeconds Cooldown duration (auto-expires)
         *
         **/
        void AddCooldown(const BaseAddress& addr, uint32_t nDurationSeconds);

        /** IsInCooldown
         *
         *  @brief Check if IP is in cooldown
         *  
         *  Automatically removes expired cooldowns during check.
         *  
         *  @param[in] addr IP address to check
         *  @return true if in cooldown (should reject connection)
         *
         **/
        bool IsInCooldown(const BaseAddress& addr);

        /** CleanupExpired
         *
         *  @brief Remove all expired cooldowns
         *  
         *  This is called periodically to clean up the cooldown map.
         *  Cooldowns are also auto-removed during IsInCooldown checks.
         *
         **/
        void CleanupExpired();

        /** GetCooldownCount
         *
         *  @brief Get the current number of active cooldowns
         *  
         *  @return Number of IPs currently in cooldown
         *
         **/
        size_t GetCooldownCount() const;

        // SECURITY: NO manual removal method
        // SECURITY: NO permanent ban method
        // SECURITY: NO persistence to disk
    };

} // namespace LLP

#endif
