/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_TIMERS_H
#define NEXUS_LLP_INCLUDE_MINING_TIMERS_H

#include <cstdint>

namespace LLP
{
namespace MiningTimers
{
    //=========================================================================
    // SESSION LIVENESS — 24-HOUR KEEPALIVE WINDOW
    //=========================================================================

    /** The application-level session liveness window.
     *
     *  Miners must send at least one SESSION_KEEPALIVE (opcode 0xD4) within
     *  this interval to prevent session expiration.  The keepalive handler
     *  refreshes NodeSessionRegistry::nLastActivity — the sole clock used by
     *  SweepExpired() — so continuous keepalive traffic keeps the session
     *  alive indefinitely.
     *
     *  This is the ONLY liveness mechanism for mining connections.  TCP
     *  keepalive (SO_KEEPALIVE) is intentionally disabled on mining ports
     *  because it operates below the application layer and cannot refresh the
     *  session identity tracked by NodeSessionRegistry.
     *
     *  Default: 86 400 s (24 hours).
     *  Runtime override: -sessionlivenesstimeout=<seconds>
     */
    constexpr uint64_t SESSION_LIVENESS_TIMEOUT_SEC = 86400;

    //=========================================================================
    // CLEANUP SWEEP CADENCE
    //=========================================================================

    /** Cadence of the periodic session/cooldown cleanup pass in the Meter
     *  thread.  This is NOT the expiry threshold — each individual pass
     *  compares entries against SESSION_LIVENESS_TIMEOUT_SEC (or the cache
     *  purge timeouts).  A shorter sweep cadence simply bounds how long a
     *  stale entry can linger before being reaped.
     *
     *  Default: 600 s (10 minutes).
     *  Runtime override: -cleanupinterval=<seconds>
     */
    constexpr int64_t CLEANUP_SWEEP_INTERVAL_SEC = 600;

    //=========================================================================
    // HEALTH PROBE CADENCE
    //=========================================================================

    /** Interval at which the Meter thread checks all authenticated mining
     *  connections for receive-idle staleness.  If no data has been received
     *  for 2× this value (default 240 s), the node flushes the outbound
     *  buffer to trigger an OS-level TCP error on a truly dead socket.
     *
     *  Default: 120 s.
     *  Runtime override: -mininghealthprobeinterval=<seconds>
     */
    constexpr int64_t HEALTH_PROBE_INTERVAL_SEC = 120;

    //=========================================================================
    // KEEPALIVE GRACE PERIOD
    //=========================================================================

    /** Grace window added to the session liveness check for miners in
     *  DEGRADED MODE recovery.  Miners that have exchanged at least one
     *  keepalive within this window are NOT evicted by CleanupInactive(),
     *  even if the main activity timeout has been exceeded.
     *
     *  420 s = DEGRADED_MODE_HARD_LIMIT (300 s) + 2 keepalive intervals
     *          (2 × 60 s).
     */
    constexpr uint64_t KEEPALIVE_GRACE_PERIOD_SEC = 420;

    //=========================================================================
    // CACHE PURGE TIMEOUTS
    //=========================================================================

    /** Default purge timeout for remote miner cache entries.
     *  After this much inactivity, the entry is removed entirely from the
     *  StatelessMinerManager cache (not just the session registry).
     *
     *  Default: 604 800 s (7 days).
     */
    constexpr uint64_t CACHE_PURGE_TIMEOUT_SEC = 604800;

    /** Extended purge timeout for localhost miners.
     *  Localhost entries persist longer to accommodate development/test setups
     *  where the miner may be restarted infrequently.
     *
     *  Default: 2 592 000 s (30 days).
     */
    constexpr uint64_t LOCALHOST_CACHE_PURGE_TIMEOUT_SEC = 2592000;

    //=========================================================================
    // METER STATISTICS OUTPUT
    //=========================================================================

    /** Minimum elapsed time before the Meter thread emits RPS/CPS/DPS/PPS
     *  statistics.  Only applies when -meters is enabled.
     *
     *  Default: 30 s.
     */
    constexpr int64_t METER_STATS_INTERVAL_SEC = 30;

} // namespace MiningTimers
} // namespace LLP

#endif
