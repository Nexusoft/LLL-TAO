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

#include <LLP/include/node_cache.h>

#include <Util/include/args.h>
#include <Util/include/config.h>

#include <cstdint>
#include <limits>
#include <string>

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

    /** Max send buffer for authenticated mining connections (5 MB).
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
     *  5 MB accommodates thousands of push notifications and prevents
     *  the DataThread from killing authenticated miners whose only crime is
     *  being slow to drain the socket during peak hash-rate periods.
     *
     *  Unauthenticated connections still use the default 3 MB limit to prevent
     *  resource abuse before Falcon authentication completes.
     *
     *  Overridable at runtime via -miningmaxsendbuffer=<bytes>.
     */
    constexpr uint64_t MINING_MAX_SEND_BUFFER = 5 * 1024 * 1024;

    //=========================================================================
    // CONNECTION HEALTH & KEEPALIVE
    //=========================================================================

    /** @deprecated  No longer used — Flush() now drains in a batch loop,
     *  so respond() calls a single Flush() instead of a retry loop.
     *  Kept for reference; safe to remove in a future cleanup pass. */
    constexpr int RESPOND_FLUSH_RETRY_COUNT    = 3;
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
    // POLL TIMEOUT
    //=========================================================================

    /** Default poll() timeout for mining DataThreads (milliseconds).
     *
     *  The generic DataThread uses a 100 ms poll() timeout which is fine for
     *  P2P traffic but adds up to 100 ms of latency for mining I/O — the
     *  single biggest source of mining read-side delay.
     *
     *  Mining DataThread instances (Miner, StatelessMinerConnection) use this
     *  much shorter timeout so that incoming share submissions and protocol
     *  messages are picked up with minimal delay.  The trade-off is slightly
     *  higher CPU utilisation on mining threads, but mining DataThreads
     *  typically serve far fewer connections than P2P threads and the extra
     *  wakeups are negligible.
     *
     *  Default: 10 ms.
     *  Overridable at runtime via -miningpolltimeout=<ms>.
     */
    constexpr uint32_t DEFAULT_MINING_POLL_TIMEOUT_MS = 10;

    //=========================================================================
    // CONNECTION TIMEOUTS
    //=========================================================================

    /** Default read-idle timeout for authenticated mining connections
     *  (milliseconds).
     *
     *  Authenticated miners use this long but finite timeout instead of the
     *  shorter server-default socket timeout.  It is intentionally aligned with
     *  the 24-hour keepalive/session liveness window so a healthy miner is not
     *  disconnected earlier than its session would expire, while still keeping
     *  transport cleanup finite — preventing
     *  the "shadow ban" scenario where PUSH works but all miner→node
     *  requests are silently dropped.
     *
     *  The shared MiningConstants liveness helpers clamp runtime config so the
     *  effective read timeout never falls below the 24-hour mining session
     *  liveness window.
     *
     *  Default: 86 400 000 ms (24 hours).
     *  Overridable at runtime via -miningreadtimeout=<ms>, subject to the
     *  shared policy floor.
     */
    constexpr uint32_t DEFAULT_MINING_READ_TIMEOUT_MS = 86400000;

    /** Shared 24-hour transport floor.
     *
     *  Mining read-idle timeout is intentionally aligned with the 24-hour
     *  keepalive/session liveness window so a healthy authenticated miner is
     *  not disconnected earlier than the node would expire its session.
     *  This is intentionally coupled to NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS
     *  so transport-idle policy and session-liveness policy cannot drift apart.
     */
    /** Session-liveness timeout as uint64 to prevent overflow during ms conversion. */
    constexpr uint64_t READ_TIMEOUT_FLOOR_SEC_U64 =
        static_cast<uint64_t>(NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS);

    /** 24-hour session-liveness floor expressed in milliseconds before narrowing. */
    constexpr uint64_t READ_TIMEOUT_FLOOR_MS_U64 =
        READ_TIMEOUT_FLOOR_SEC_U64 * 1000ULL;

    static_assert(READ_TIMEOUT_FLOOR_SEC_U64 <=
            std::numeric_limits<uint64_t>::max() / 1000ULL,
        "SESSION_LIVENESS_TIMEOUT_SECONDS must not overflow uint64 during ms conversion.");
    static_assert(READ_TIMEOUT_FLOOR_MS_U64 <=
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
        "SESSION_LIVENESS_TIMEOUT_SECONDS must fit in uint32 milliseconds.");
    constexpr uint32_t READ_TIMEOUT_FLOOR_MS =
        static_cast<uint32_t>(READ_TIMEOUT_FLOOR_MS_U64);
    static_assert(DEFAULT_MINING_READ_TIMEOUT_MS >= READ_TIMEOUT_FLOOR_MS,
        "DEFAULT_MINING_READ_TIMEOUT_MS must satisfy the shared mining read-timeout floor.");

    /** Shared session liveness timeout accessor used by legacy/stateless paths. */
    inline uint64_t GetSessionLivenessTimeoutSec(const std::string& strAddress = std::string())
    {
        return NodeCache::GetSessionLivenessTimeout(strAddress);
    }

    /** Shared accessor for authenticated mining read-idle timeout.
     *
     *  Runtime config remains supported, but values below the policy floor are
     *  clamped upward so operators cannot shorten the transport timeout below
     *  the 24-hour mining liveness contract.
     */
    inline uint32_t GetConfiguredReadTimeoutMs()
    {
        const int64_t nConfigured = config::GetArg(
            "-miningreadtimeout",
            static_cast<int64_t>(DEFAULT_MINING_READ_TIMEOUT_MS));

        const int64_t nClamped =
            (nConfigured < static_cast<int64_t>(READ_TIMEOUT_FLOOR_MS))
                ? static_cast<int64_t>(READ_TIMEOUT_FLOOR_MS)
                : nConfigured;

        if(nClamped >= static_cast<int64_t>(std::numeric_limits<uint32_t>::max()))
            return std::numeric_limits<uint32_t>::max();

        return static_cast<uint32_t>(nClamped);
    }

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
