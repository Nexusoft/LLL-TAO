/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/auto_cooldown_manager.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /** Get the singleton instance **/
    AutoCooldownManager& AutoCooldownManager::Get()
    {
        static AutoCooldownManager instance;
        return instance;
    }

    /** AddCooldown
     *
     *  Add IP to auto-expiring cooldown (AUTOMATED ONLY)
     *
     **/
    void AutoCooldownManager::AddCooldown(const BaseAddress& addr, uint32_t nDurationSeconds)
    {
        LOCK(m_mutex);
        
        /* Get IPv4 address bytes for storage */
        /* For IPv4: bytes are at [0][1][2][3] in standard order */
        uint32_t nIP = 0;
        if (addr.IsIPv4()) {
            nIP = (static_cast<uint32_t>(addr.GetByte(3)) << 24) |
                  (static_cast<uint32_t>(addr.GetByte(2)) << 16) |
                  (static_cast<uint32_t>(addr.GetByte(1)) << 8) |
                  static_cast<uint32_t>(addr.GetByte(0));
        } else {
            /* For IPv6, use a hash or just the lower 32 bits */
            /* For now, use the last 4 bytes */
            nIP = (static_cast<uint32_t>(addr.GetByte(3)) << 24) |
                  (static_cast<uint32_t>(addr.GetByte(2)) << 16) |
                  (static_cast<uint32_t>(addr.GetByte(1)) << 8) |
                  static_cast<uint32_t>(addr.GetByte(0));
        }
        
        /* Calculate expiry timestamp */
        uint64_t nExpiry = runtime::unifiedtimestamp() + nDurationSeconds;
        
        /* Store cooldown with expiry */
        m_cooldowns[nIP] = nExpiry;
        
        debug::log(0, FUNCTION, "AutoCooldown: Added ", addr.ToStringIP(), 
            " for ", nDurationSeconds, " seconds (expires at ", nExpiry, ")");
        debug::log(0, FUNCTION, "   This is AUTOMATED protection, not a ban");
        debug::log(0, FUNCTION, "   Cooldown will auto-expire - miner can reconnect after");
    }

    /** IsInCooldown
     *
     *  Check if IP is in cooldown (auto-removes if expired)
     *
     **/
    bool AutoCooldownManager::IsInCooldown(const BaseAddress& addr)
    {
        LOCK(m_mutex);
        
        /* Get IPv4 address bytes */
        uint32_t nIP = 0;
        if (addr.IsIPv4()) {
            nIP = (static_cast<uint32_t>(addr.GetByte(3)) << 24) |
                  (static_cast<uint32_t>(addr.GetByte(2)) << 16) |
                  (static_cast<uint32_t>(addr.GetByte(1)) << 8) |
                  static_cast<uint32_t>(addr.GetByte(0));
        } else {
            /* For IPv6, use the last 4 bytes */
            nIP = (static_cast<uint32_t>(addr.GetByte(3)) << 24) |
                  (static_cast<uint32_t>(addr.GetByte(2)) << 16) |
                  (static_cast<uint32_t>(addr.GetByte(1)) << 8) |
                  static_cast<uint32_t>(addr.GetByte(0));
        }
        
        /* Check if IP is in cooldown map */
        auto it = m_cooldowns.find(nIP);
        if (it == m_cooldowns.end()) {
            return false;  // Not in cooldown
        }
        
        /* Check if expired */
        uint64_t nNow = runtime::unifiedtimestamp();
        if (nNow >= it->second) {
            // Auto-expire: remove from map
            debug::log(1, FUNCTION, "AutoCooldown: Expired for ", addr.ToStringIP(), " - welcome back!");
            m_cooldowns.erase(it);
            return false;  // Cooldown expired, allow connection
        }
        
        /* Still in cooldown */
        uint64_t nRemaining = it->second - nNow;
        debug::log(1, FUNCTION, "AutoCooldown: ", addr.ToStringIP(), 
            " still in cooldown (", nRemaining, " seconds remaining)");
        return true;
    }

    /** CleanupExpired
     *
     *  Remove all expired cooldowns
     *
     **/
    void AutoCooldownManager::CleanupExpired()
    {
        LOCK(m_mutex);
        
        uint64_t nNow = runtime::unifiedtimestamp();
        size_t nRemoved = 0;
        
        /* Iterate through cooldowns and remove expired ones */
        for (auto it = m_cooldowns.begin(); it != m_cooldowns.end(); ) {
            if (nNow >= it->second) {
                debug::log(2, FUNCTION, "AutoCooldown: Cleanup - removed expired entry");
                it = m_cooldowns.erase(it);
                ++nRemoved;
            } else {
                ++it;
            }
        }
        
        if (nRemoved > 0) {
            debug::log(1, FUNCTION, "AutoCooldown: Cleanup complete - removed ", nRemoved, " expired entries");
        }
    }

    /** GetCooldownCount
     *
     *  Get the current number of active cooldowns
     *
     **/
    size_t AutoCooldownManager::GetCooldownCount() const
    {
        LOCK(m_mutex);
        return m_cooldowns.size();
    }

} // namespace LLP
