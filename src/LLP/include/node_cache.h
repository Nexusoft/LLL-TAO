/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NODE_CACHE_H
#define NEXUS_LLP_INCLUDE_NODE_CACHE_H

#include <cstdint>
#include <cstddef>
#include <string>

namespace LLP
{
namespace NodeCache
{
    /** Cache Configuration Constants
     *
     *  Hardcoded DDOS protection limits for miner cache management.
     *
     **/

    /** DEFAULT_MAX_CACHE_SIZE
     *
     *  Maximum number of cached miner entries for DDOS protection.
     *  Default: 1000 entries Across 2 Mining Servers Stateless + Legacy 
     *            
     **/
    static constexpr size_t DEFAULT_MAX_CACHE_SIZE = 1000;

    /** DEFAULT_KEEPALIVE_INTERVAL
     *
     *  Required keepalive interval for miners to remain cached.
     *  Miners must ping at least once within this interval.
     *  Default: 86400 seconds (24 hours)
     *
     **/
    static constexpr uint64_t DEFAULT_KEEPALIVE_INTERVAL = 86400;

    /** SESSION_LIVENESS_TIMEOUT_SECONDS
     *
     *  Session liveness timeout used for rolling keepalive/session expiry.
     *  This is intentionally distinct from the much longer cache purge timeout.
     *
     **/
    static constexpr uint64_t SESSION_LIVENESS_TIMEOUT_SECONDS = DEFAULT_KEEPALIVE_INTERVAL;

    /** DEFAULT_CACHE_PURGE_TIMEOUT
     *
     *  Timeout after which inactive miners are purged from cache.
     *  Default: 604800 seconds (7 days)
     *
     **/
    static constexpr uint64_t DEFAULT_CACHE_PURGE_TIMEOUT = 604800;

    /** LOCALHOST_CACHE_PURGE_TIMEOUT
     *
     *  Extended timeout for localhost miners (exception handling).
     *  Localhost miners get longer cache persistence.
     *  Default: 2592000 seconds (30 days)
     *
     **/
    static constexpr uint64_t LOCALHOST_CACHE_PURGE_TIMEOUT = 2592000;

    /** KEEPALIVE_GRACE_PERIOD_SEC
     *
     *  Grace period for keepalive check in smart timeout logic.
     *  Sessions with a keepalive exchange within this window are not
     *  considered idle.
     *
     *  Aligned with the miner's maximum degraded recovery window:
     *  DEGRADED_MODE_HARD_LIMIT_SECONDS (300s) + 2 keepalive intervals
     *  (2×60s = 120s).  A miner in DEGRADED MODE sends keepalives every
     *  ~60s but may stop sending new MINER_READY for up to 300s while
     *  running its escape ladder.  Evicting the session during that
     *  window would cause Stateless=0 on the next BroadcastChannel event.
     *
     *  Default: 420 seconds (7 minutes)
     *
     **/
    static constexpr uint64_t KEEPALIVE_GRACE_PERIOD_SEC = 420;

    /** LOCALHOST_ADDRESSES
     *
     *  Addresses that qualify for localhost exception handling.
     *
     **/
    const char* const LOCALHOST_IPV4 = "127.0.0.1";
    const char* const LOCALHOST_IPV6 = "::1";
    const char* const LOCALHOST_NAME = "localhost";

    /** IsLocalhost
     *
     *  Check if an address qualifies for localhost exception handling.
     *
     *  @param[in] strAddress The miner's network address
     *
     *  @return true if address is localhost
     *
     **/
    bool IsLocalhost(const std::string& strAddress);

    /** GetPurgeTimeout
     *
     *  Get the appropriate purge timeout for a given address.
     *  Returns extended timeout for localhost, standard timeout otherwise.
     *
     *  @param[in] strAddress The miner's network address
     *
     *  @return Purge timeout in seconds
     *
     **/
    uint64_t GetPurgeTimeout(const std::string& strAddress);

    /** GetSessionLivenessTimeout
     *
     *  Get the rolling session liveness timeout for a miner connection.
     *  This timeout is used for session expiry / keepalive logic and remains
     *  intentionally separate from cache retention / purge timeouts.
     *
     *  @param[in] strAddress The miner's network address
     *
     *  @return Session liveness timeout in seconds
     *
     **/
    uint64_t GetSessionLivenessTimeout(const std::string& strAddress);

} // namespace NodeCache
} // namespace LLP

#endif
