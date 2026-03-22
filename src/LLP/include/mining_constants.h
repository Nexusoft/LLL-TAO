/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_CONSTANTS_H
#define NEXUS_LLP_INCLUDE_MINING_CONSTANTS_H

#include <cstdint>

namespace LLP
{
namespace MiningConstants
{
    //=========================================================================
    // RATE LIMITING CONFIGURATION
    // Different values for DEBUG vs PRODUCTION builds
    //=========================================================================
    
    #ifdef ENABLE_DEBUG

        /** Minimum interval between GET_BLOCK requests.
         *
         *  2-second minimum interval between GET_BLOCK requests.
         *  Matches GET_BLOCK_COOLDOWN_SECONDS. Both mechanisms enforce the same
         *  2-second floor — no lockout, no doom loop.
         */
        constexpr uint32_t GET_BLOCK_MIN_INTERVAL_MS = 2000;

        /** Throttled interval for GET_BLOCK when rate limited (2 seconds) */
        constexpr uint32_t GET_BLOCK_THROTTLE_INTERVAL_MS = 2000;

        /** Number of rate limit violations before temporary ban (15 strikes) */
        constexpr uint32_t RATE_LIMIT_STRIKE_THRESHOLD = 15;

        /** Auto-cooldown duration in seconds (5 minute) */
        constexpr uint32_t AUTOCOOLDOWN_DURATION_SECONDS = 300;

        /** Disable rate limiting for localhost connections in debug mode */
        constexpr bool DISABLE_LOCALHOST_RATE_LIMITING = false;

    #else

        /* Production Build - Strict rate limits for network security */
        
        /** Minimum interval between GET_BLOCK requests.
         *
         *  2-second minimum interval between GET_BLOCK requests.
         *  Matches GET_BLOCK_COOLDOWN_SECONDS. Both mechanisms enforce the same
         *  2-second floor — no lockout, no doom loop.
         */
        constexpr uint32_t GET_BLOCK_MIN_INTERVAL_MS = 2000;
        
        /** Throttled interval for GET_BLOCK when rate limited (2 seconds) */
        constexpr uint32_t GET_BLOCK_THROTTLE_INTERVAL_MS = 2000;
        
        /** Number of rate limit violations before temporary ban (15 strikes) */
        constexpr uint32_t RATE_LIMIT_STRIKE_THRESHOLD = 15;
        
        /** Auto-cooldown duration in seconds (5 minutes) */
        constexpr uint32_t AUTOCOOLDOWN_DURATION_SECONDS = 300;
        
        /** Disable rate limiting for localhost connections (false in production) */
        constexpr bool DISABLE_LOCALHOST_RATE_LIMITING = false;
        
    #endif
    
    //=========================================================================
    // OPCODE VALIDATION RANGES
    //=========================================================================
    
    /** Start of valid mining opcodes (BLOCK_ACCEPTED = 200)
     *  This covers both legacy responses (200-206) and stateless-specific opcodes (207-218)
     */
    constexpr uint8_t MINING_OPCODE_MIN = 200;
    
    /** End of valid mining opcodes (HASH_BLOCK_AVAILABLE = 218)
     *  This is the highest opcode in the mining protocol range
     */
    constexpr uint8_t MINING_OPCODE_MAX = 218;
    
    /** Start of stateless-specific opcodes (MINER_AUTH_INIT = 207)
     *  These opcodes should NOT be mirrored as they are already stateless-protocol-specific
     *  Legacy responses (200-206) are mirrored, stateless-specific (207-218) are not
     */
    constexpr uint8_t STATELESS_MINING_OPCODE_MIN = 207;
    
    /** End of stateless-specific opcodes (HASH_BLOCK_AVAILABLE = 218)
     *  Same as MINING_OPCODE_MAX - all opcodes from 207-218 are stateless-specific
     */
    constexpr uint8_t STATELESS_MINING_OPCODE_MAX = 218;
    
    //=========================================================================
    // PUSH THROTTLE AND COOLDOWN CONFIGURATION
    //=========================================================================

    /** Minimum interval between consecutive unsolicited template pushes (1 second).
     *
     *  Guards SendStatelessTemplate() and SendChannelNotification() against
     *  flooding miners during a fork-resolution burst (multiple SetBest() events
     *  firing in < 100 ms).  1 s is still well above any fork-resolution burst
     *  window (~100 ms) and below any real block-time floor, so miners always get
     *  a fresh template within 1 s of tip stabilisation.
     *
     *  Reduced from 2 000 ms to 1 000 ms to close the doom-loop window: when a
     *  miner re-subscribes (STATELESS_MINER_READY) and simultaneously has its
     *  GET_BLOCK rate-limited, the node's push throttle was the last gate
     *  preventing fresh work delivery.  Re-subscription responses bypass this
     *  throttle entirely via the m_force_next_push flag.
     */
    constexpr int64_t TEMPLATE_PUSH_MIN_INTERVAL_MS = 1000;

    /** Per-connection GET_BLOCK minimum interval (2 seconds).
     *
     *  Defined outside the #ifdef ENABLE_DEBUG block so it applies to
     *  BOTH debug and production builds.
     *
     *  Used by AutoCoolDown m_get_block_cooldown on each miner connection.
     *  This is a simple 2-second rate-limit floor — NOT a lockout window.
     *  The cooldown is NOT reset after serving a GET_BLOCK, so miners can
     *  retry every 2 seconds during recovery from Emergency/Degraded mode.
     *  The per-minute cap (MAX_GET_BLOCK_PER_MINUTE) provides the primary
     *  spam protection; this floor just prevents rapid-fire polling abuse.
     *  MINER_READY resets it for an immediate first GET_BLOCK after
     *  re-subscription.
     */
    constexpr uint32_t GET_BLOCK_COOLDOWN_SECONDS = 2;

    /** Localhost connections skip AutoCoolDown in production.
     *
     *  A local miner cannot be a DDOS vector; the per-minute cap
     *  (MAX_GET_BLOCK_PER_MINUTE) provides sufficient rate control.
     *  Remote miners are still protected by the 2-second rate-limit floor.
     */
    constexpr bool DISABLE_LOCALHOST_AUTOCOOLDOWN = true;

    //=========================================================================
    // DIFFICULTY CACHING
    //=========================================================================
    
    /** Difficulty cache time-to-live in milliseconds (1 second)
     *  Reduces expensive GetNextTargetRequired() calls during high mining activity
     */
    constexpr uint32_t DIFFICULTY_CACHE_TTL_MS = 1000;
    
    /** Difficulty cache TTL in seconds (for runtime comparison)
     *  Precalculated to avoid division in hot path
     *  NOTE: Requires DIFFICULTY_CACHE_TTL_MS >= 1000 for correct operation
     */
    constexpr uint64_t DIFFICULTY_CACHE_TTL_SECONDS = DIFFICULTY_CACHE_TTL_MS / 1000;
    
    static_assert(DIFFICULTY_CACHE_TTL_MS >= 1000, 
                  "DIFFICULTY_CACHE_TTL_MS must be >= 1000ms (1 second) to avoid truncation");
    
} // namespace MiningConstants
} // namespace LLP

#endif
