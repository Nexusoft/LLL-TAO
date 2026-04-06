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
    //=========================================================================

    /** Minimum interval between GET_BLOCK requests.
     *
     *  1-second minimum interval between GET_BLOCK requests.
     *  Matches GET_BLOCK_COOLDOWN_SECONDS. Both mechanisms enforce the same
     *  1-second floor — no lockout, no doom loop.
     *  The 25/minute rolling cap is the primary firewall; the 1s floor is
     *  just burst smoothing.
     */
    constexpr uint32_t GET_BLOCK_MIN_INTERVAL_MS = 1000;

    /** Throttled interval for GET_BLOCK when rate limited (1 second) */
    constexpr uint32_t GET_BLOCK_THROTTLE_INTERVAL_MS = 1000;

    /** Number of rate limit violations before temporary ban (15 strikes) */
    constexpr uint32_t RATE_LIMIT_STRIKE_THRESHOLD = 15;

    /** Auto-cooldown duration in seconds (5 minutes) */
    constexpr uint32_t AUTOCOOLDOWN_DURATION_SECONDS = 300;
    
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

    /** Per-connection GET_BLOCK minimum interval (1 second).
     *
     *  Used by AutoCoolDown m_get_block_cooldown on each miner connection.
     *  This is a simple 1-second rate-limit floor — NOT a lockout window.
     *  The cooldown is NOT reset after serving a GET_BLOCK, so miners can
     *  retry every 1 second during recovery from Emergency/Degraded mode.
     *  The per-minute cap (MAX_GET_BLOCK_PER_MINUTE) provides the primary
     *  spam protection (the real firewall); this 1s floor is just burst
     *  smoothing that prevents rapid-fire polling abuse.
     *  MINER_READY resets it for an immediate first GET_BLOCK after
     *  re-subscription.
     */
    constexpr uint32_t GET_BLOCK_COOLDOWN_SECONDS = 1;

    //=========================================================================
    // MINING SEND BUFFER
    //=========================================================================

    /** Max send buffer for authenticated mining connections (15 MB).
     *
     *  Mining push notifications (channel notifications + BLOCK_DATA templates)
     *  are the primary — and often the only reliable — mechanism for delivering
     *  fresh work to miners.  Push is NEVER advisory; it is Priority #1.
     *
     *  The default MAX_SEND_BUFFER (3 MB) is appropriate for P2P connections
     *  but far too small for mining: a miner busy hashing may not read its
     *  socket for several seconds, causing the node's send buffer to fill and
     *  triggering DISCONNECT::BUFFER — a silent kill from the miner's
     *  perspective.
     *
     *  15 MB accommodates tens of thousands of push notifications and prevents
     *  the DataThread from killing authenticated miners whose only crime is
     *  being slow to drain the socket during peak hash-rate periods.
     *
     *  Unauthenticated connections still use the default 3 MB limit to prevent
     *  resource abuse before Falcon authentication completes.
     *
     *  Overridable at runtime via -miningmaxsendbuffer=<bytes>.
     */
    constexpr uint64_t MINING_MAX_SEND_BUFFER = 15 * 1024 * 1024;

    //=========================================================================
    // CONNECTION HEALTH & KEEPALIVE
    //=========================================================================

    /** Maximum flush-and-retry attempts in respond() before giving up.
     *
     *  When the send buffer is saturated (fBufferFull == true), respond() tries
     *  to Flush() the socket up to this many times (with a short sleep between
     *  attempts) before falling through to WritePacket(), which may drop the
     *  packet.  Mining responses are tiny (16–32 bytes) so even a partial
     *  flush usually frees enough space.
     */
    constexpr int RESPOND_FLUSH_RETRY_COUNT = 3;

    /** Sleep between flush retries in respond() (milliseconds). */
    constexpr int RESPOND_FLUSH_RETRY_SLEEP_MS = 10;

    /** Default node-side health probe interval (seconds).
     *
     *  The Meter thread checks all authenticated mining connections for
     *  receive-idle staleness at this cadence.  If a miner hasn't sent
     *  anything for 2× this value, the node flushes the outbound buffer
     *  to force an OS-level TCP error if the path is truly dead.
     *
     *  Overridable at runtime via -mininghealthprobeinterval=<seconds>.
     */
    constexpr int64_t NODE_HEALTH_PROBE_INTERVAL_SEC = 120;

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
