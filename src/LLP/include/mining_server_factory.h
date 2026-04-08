/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_MINING_SERVER_FACTORY_H
#define NEXUS_LLP_INCLUDE_MINING_SERVER_FACTORY_H

#include <LLP/include/config.h>
#include <LLP/include/port.h>

namespace LLP
{
    /** MiningServerFactory
     *
     *  Factory for building unified mining server configurations.
     *
     *  DESIGN OVERVIEW:
     *  ================
     *  This factory eliminates duplication in global.cpp by providing a single entry-point
     *  for creating mining server configurations. Both the Stateless (port 9323) and Legacy
     *  (port 8323) mining lanes share identical configuration except for:
     *  1. Port number (GetMiningPort() vs GetLegacyMiningPort())
     *  2. Default socket timeout (300s for stateless push, 120s for legacy polling)
     *  3. Per-lane timeout configuration (-miningstatelesstimeout / -mininglegacytimeout)
     *
     *  ARCHITECTURAL BENEFITS:
     *  =======================
     *  - Eliminates 20+ lines of duplicated Config setup code
     *  - Single source of truth for mining server parameters
     *  - Easy to add new configuration options (update once, applies to both lanes)
     *  - Clear separation between lane-specific (port, timeout) and shared config
     *
     *  USAGE:
     *  ======
     *  In global.cpp Initialize():
     *
     *    // Stateless lane
     *    LLP::Config CONFIG = MiningServerFactory::BuildConfig(
     *        MiningServerFactory::Lane::STATELESS);
     *    STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(CONFIG);
     *
     *    // Legacy lane
     *    LLP::Config LEGACY_CONFIG = MiningServerFactory::BuildConfig(
     *        MiningServerFactory::Lane::LEGACY);
     *    MINING_SERVER = new Server<Miner>(LEGACY_CONFIG);
     *
     *  USAGE (SSL-enabled):
     *  ====================
     *  When -miningssl=1 is set, BuildConfig() automatically wires PORT_SSL.
     *  BuildSSLConfig() can be used for explicit SSL-only server construction:
     *
     *    // Stateless SSL lane (port 9325)
     *    LLP::Config cfg = MiningServerFactory::BuildSSLConfig(
     *        MiningServerFactory::Lane::STATELESS);
     *    STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(cfg);
     *
     *    // Legacy SSL lane (port 8325) — guaranteed clean reconnect
     *    LLP::Config legacy_cfg = MiningServerFactory::BuildSSLConfig(
     *        MiningServerFactory::Lane::LEGACY);
     *    MINING_SERVER = new Server<Miner>(legacy_cfg);
     *
     *  LANE DIFFERENCES:
     *  =================
     *  Stateless Lane (Port 9323):
     *  - 16-bit stateless packet framing
     *  - Push notification protocol (event-driven)
     *  - 300-second socket timeout (Prime blocks take 2-5+ minutes)
     *  - Falcon authentication mandatory
     *
     *  Legacy Lane (Port 8323):
     *  - 8-bit legacy packet framing
     *  - GET_BLOCK polling protocol (request-driven)
     *  - 120-second socket timeout (polling frequency ~5-10s)
     *  - Falcon authentication optional (backward compatibility)
     *
     *  SHARED CONFIGURATION:
     *  =====================
     *  Both lanes share identical settings for:
     *  - ENABLE_LISTEN: true (accept incoming connections)
     *  - ENABLE_METERS: false (no bandwidth metering)
     *  - ENABLE_DDOS: configurable via -miningddos
     *  - ENABLE_MANAGER: false (no connection manager)
     *  - ENABLE_SSL: configurable via -miningssl
     *  - ENABLE_REMOTE: true (allow remote connections)
     *  - REQUIRE_SSL: configurable via -miningsslrequired
     *  - PORT_SSL: GetMiningSSLPort() for STATELESS (default 9325), GetLegacyMiningSSLPort() for LEGACY (default 8325)
     *              Set to 0 when -miningssl is not enabled (no port stolen).
     *  - MAX_INCOMING: 128 connections
     *  - MAX_CONNECTIONS: 128 total
     *  - MAX_THREADS: configurable via -miningthreads (default 4)
     *  - DDOS_CSCORE: configurable via -miningcscore (default 1)
     *  - DDOS_RSCORE: configurable via -miningrscore (default 50)
     *  - DDOS_TIMESPAN: configurable via -miningtimespan (default 60)
     *  - MANAGER_SLEEP: 0 (disabled)
     *
     *  EXTENSIBILITY:
     *  ==============
     *  Future enhancements can add lane-specific parameters by extending the BuildConfig
     *  method signature or adding new factory methods. For example:
     *  - Custom rate limiting per lane
     *  - Different DDOS thresholds per lane
     *  - Lane-specific logging levels
     *
     *  RELATED FILES:
     *  ==============
     *  - src/LLP/global.cpp: Uses BuildConfig() for both mining lanes
     *  - src/LLP/include/port.h: Provides GetMiningPort() and GetLegacyMiningPort()
     *  - src/LLP/include/config.h: Defines LLP::Config structure
     *
     *  @see LLP::Config for configuration structure details
     *  @see LLP::GetMiningPort() for stateless lane port resolution
     *  @see LLP::GetLegacyMiningPort() for legacy lane port resolution
     *
     **/
    class MiningServerFactory
    {
    public:
        /** Lane
         *
         *  Mining protocol lane identifier.
         *
         *  STATELESS: Modern 16-bit framing, push notifications, port 9323
         *  LEGACY:    Classic 8-bit framing, polling, port 8323
         *
         **/
        enum class Lane
        {
            STATELESS,  // Phase 2 stateless miner (port 9323, 300s timeout, push-based)
            LEGACY      // Legacy hybrid miner (port 8323, 120s timeout, polling-based)
        };


        /** BuildConfig
         *
         *  Build a unified mining server configuration for the specified lane.
         *
         *  This is the canonical factory method for creating mining server configurations.
         *  Both lanes use identical configuration except for port and timeout settings.
         *
         *  Lane-Specific Behavior:
         *  -----------------------
         *  STATELESS (port 9323):
         *  - Port: GetMiningPort() (default 9323)
         *  - Timeout: 300 seconds (accommodates long Prime block searches)
         *  - Protocol: 16-bit stateless framing with push notifications
         *  - Use case: Modern NexusMiner clients with event-driven mining
         *
         *  LEGACY (port 8323):
         *  - Port: GetLegacyMiningPort() (default 8323)
         *  - Timeout: 120 seconds (traditional polling interval)
         *  - Protocol: 8-bit legacy framing with GET_BLOCK polling
         *  - Use case: Backward compatibility with older miners
         *
         *  Configuration Parameters:
         *  -------------------------
         *  All configuration parameters respect command-line arguments:
         *  - -miningthreads: Worker thread count (default: 4)
         *  - -miningstatelesstimeout: Override stateless lane timeout (default: 300s)
         *  - -mininglegacytimeout: Override legacy lane timeout (default: 120s)
         *  - -miningtimeout: Override timeout for both lanes (deprecated, use lane-specific)
         *  - -miningddos: Enable DDOS protection (default: false)
         *  - -miningssl: Enable SSL (default: false)
         *  - -miningsslrequired: Require SSL for all connections
         *  - -miningstatelesssslport: Override stateless SSL port (default: 9325)
         *  - -mininglegacysslport: Override legacy SSL port (default: 8325)
         *  - -miningcscore: DDOS connection score threshold (default: 1)
         *  - -miningrscore: DDOS request score threshold (default: 50)
         *  - -miningtimespan: DDOS timespan in seconds (default: 60)
         *
         *  Thread Safety:
         *  --------------
         *  This method is read-only and safe to call concurrently from multiple threads.
         *  It queries global config::GetArg() and config::GetBoolArg() which are
         *  initialized once at startup and remain constant during runtime.
         *
         *  Memory Management:
         *  ------------------
         *  Returns LLP::Config by value (copy semantics). The caller owns the returned
         *  Config object and can pass it to Server<T> constructor.
         *
         *  @param[in] lane    The mining protocol lane (STATELESS or LEGACY)
         *
         *  @return LLP::Config object ready for Server<T> construction
         *
         *  Example Usage:
         *  --------------
         *  // Stateless mining server
         *  LLP::Config cfg = MiningServerFactory::BuildConfig(
         *      MiningServerFactory::Lane::STATELESS);
         *  STATELESS_MINER_SERVER = new Server<StatelessMinerConnection>(cfg);
         *
         *  // Legacy mining server
         *  LLP::Config legacy_cfg = MiningServerFactory::BuildConfig(
         *      MiningServerFactory::Lane::LEGACY);
         *  MINING_SERVER = new Server<Miner>(legacy_cfg);
         *
         **/
        static LLP::Config BuildConfig(Lane lane)
        {
            /* Determine lane-specific parameters */
            const uint16_t nPort = (lane == Lane::STATELESS) ?
                GetMiningPort() : GetLegacyMiningPort();

            /* Socket timeout for mining lanes.
             * Both Stateless and Legacy lanes use the same default timeout.
             * The only protocol difference is opcode size (1-byte vs 2-byte).
             *
             * With authenticated miners exempt from socket read-idle timeout
             * (via IsTimeoutExempt()), this value primarily affects
             * unauthenticated connections that haven't completed Falcon auth.
             * Authenticated miners are governed by the 24-hour session keepalive. */
            const uint32_t nDefaultTimeout = 300;

            /* Generate unified config object */
            LLP::Config CONFIG = LLP::Config(nPort);

            /* Shared configuration (identical for both lanes) */
            CONFIG.ENABLE_LISTEN   = true;
            CONFIG.ENABLE_METERS   = false;
            CONFIG.ENABLE_DDOS     = config::GetBoolArg(std::string("-miningddos"), false);
            CONFIG.ENABLE_MANAGER  = false;
            CONFIG.ENABLE_SSL      = config::GetBoolArg(std::string("-miningssl"), false);
            CONFIG.ENABLE_REMOTE   = true;
            CONFIG.REQUIRE_SSL     = config::GetBoolArg(std::string("-miningsslrequired"), false);
            CONFIG.PORT_SSL        = CONFIG.ENABLE_SSL ?
                ((lane == Lane::STATELESS) ? GetMiningSSLPort() : GetLegacyMiningSSLPort()) :
                0;
            CONFIG.MAX_INCOMING    = 128;
            CONFIG.MAX_CONNECTIONS = 128;
            CONFIG.MAX_THREADS     = config::GetArg(std::string("-miningthreads"), 4);
            CONFIG.DDOS_CSCORE     = config::GetArg(std::string("-miningcscore"), 1);
            CONFIG.DDOS_RSCORE     = config::GetArg(std::string("-miningrscore"), 500);
            CONFIG.DDOS_TIMESPAN   = config::GetArg(std::string("-miningtimespan"), 60);
            CONFIG.MANAGER_SLEEP   = 0;  // Connection manager disabled

            /* Lane-specific socket timeout with priority:
             * 1. Lane-specific argument (-miningstatelesstimeout or -mininglegacytimeout)
             * 2. Generic argument (-miningtimeout) for backward compatibility
             * 3. Lane-specific default (300s for stateless, 120s for legacy) */
            if (lane == Lane::STATELESS)
            {
                /* Check for stateless-specific timeout first */
                if (config::HasArg(std::string("-miningstatelesstimeout")))
                    CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningstatelesstimeout"), nDefaultTimeout);
                else
                    CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningtimeout"), nDefaultTimeout);
            }
            else
            {
                /* Check for legacy-specific timeout first */
                if (config::HasArg(std::string("-mininglegacytimeout")))
                    CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-mininglegacytimeout"), nDefaultTimeout);
                else
                    CONFIG.SOCKET_TIMEOUT = config::GetArg(std::string("-miningtimeout"), nDefaultTimeout);
            }

            return CONFIG;
        }


        /** BuildSSLConfig
         *
         *  Build a mining server configuration for the specified lane with SSL explicitly enabled.
         *
         *  This is a convenience wrapper around BuildConfig() that forces ENABLE_SSL=true
         *  and sets the appropriate SSL port, regardless of the -miningssl command-line flag.
         *  Use this when the caller has already determined that SSL should be active (e.g.,
         *  when -miningssl=1 is set and you want explicit, self-documenting call sites).
         *
         *  Port Assignment:
         *  ----------------
         *  STATELESS: PORT_SSL = GetMiningSSLPort()       (default 9325, override -miningstatelesssslport)
         *  LEGACY:    PORT_SSL = GetLegacyMiningSSLPort() (default 8325, override -mininglegacysslport)
         *
         *  REQUIRE_SSL behavior:
         *  ---------------------
         *  Still reads -miningsslrequired from config. Set REQUIRE_SSL=true in nexus.conf
         *  to suppress the plaintext listener entirely (remote-facing public pools).
         *
         *  @param[in] lane    The mining protocol lane (STATELESS or LEGACY)
         *
         *  @return LLP::Config with ENABLE_SSL=true and PORT_SSL wired to the lane-specific port.
         *
         **/
        static LLP::Config BuildSSLConfig(Lane lane)
        {
            LLP::Config cfg = BuildConfig(lane);

            /* Force SSL enabled regardless of -miningssl flag */
            cfg.ENABLE_SSL = true;

            /* Wire the correct SSL port for this lane */
            cfg.PORT_SSL = (lane == Lane::STATELESS) ?
                GetMiningSSLPort() : GetLegacyMiningSSLPort();

            return cfg;
        }
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_MINING_SERVER_FACTORY_H
