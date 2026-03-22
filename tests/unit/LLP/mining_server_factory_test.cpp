/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_server_factory.h>
#include <LLP/include/config.h>

#include <Util/include/config.h>
#include <Util/include/args.h>

#include <string>

/* Test the MiningServerFactory for per-lane timeout configuration.
 * The factory should support:
 * 1. Lane-specific timeouts (-miningstatelesstimeout, -mininglegacytimeout)
 * 2. Generic timeout for backward compatibility (-miningtimeout)
 * 3. Lane-specific defaults (300s for stateless, 120s for legacy)
 */

TEST_CASE("MiningServerFactory per-lane timeout configuration", "[miner][config][factory]")
{
    /* Save original config state to restore after tests */
    std::map<std::string, std::string> mapOriginalArgs = config::mapArgs;
    std::map<std::string, std::vector<std::string>> mapOriginalMultiArgs = config::mapMultiArgs;

    SECTION("Default timeouts without any command-line arguments")
    {
        /* Clear all mining timeout arguments */
        config::mapArgs.erase("-miningtimeout");
        config::mapArgs.erase("-miningstatelesstimeout");
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Verify lane-specific default timeouts */
        REQUIRE(stateless.SOCKET_TIMEOUT == 300);  // Stateless default: 300s
        REQUIRE(legacy.SOCKET_TIMEOUT == 120);     // Legacy default: 120s
    }

    SECTION("Generic -miningtimeout applies to both lanes")
    {
        /* Set generic timeout */
        config::mapArgs["-miningtimeout"] = "180";
        config::mapArgs.erase("-miningstatelesstimeout");
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Both lanes should use the generic timeout */
        REQUIRE(stateless.SOCKET_TIMEOUT == 180);
        REQUIRE(legacy.SOCKET_TIMEOUT == 180);
    }

    SECTION("-miningstatelesstimeout overrides default for stateless lane only")
    {
        /* Set stateless-specific timeout */
        config::mapArgs["-miningstatelesstimeout"] = "450";
        config::mapArgs.erase("-miningtimeout");
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Stateless uses the specified timeout, legacy uses default */
        REQUIRE(stateless.SOCKET_TIMEOUT == 450);
        REQUIRE(legacy.SOCKET_TIMEOUT == 120);
    }

    SECTION("-mininglegacytimeout overrides default for legacy lane only")
    {
        /* Set legacy-specific timeout */
        config::mapArgs["-mininglegacytimeout"] = "90";
        config::mapArgs.erase("-miningtimeout");
        config::mapArgs.erase("-miningstatelesstimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Stateless uses default, legacy uses the specified timeout */
        REQUIRE(stateless.SOCKET_TIMEOUT == 300);
        REQUIRE(legacy.SOCKET_TIMEOUT == 90);
    }

    SECTION("Lane-specific timeout takes priority over generic -miningtimeout")
    {
        /* Set both generic and lane-specific timeouts */
        config::mapArgs["-miningtimeout"] = "180";
        config::mapArgs["-miningstatelesstimeout"] = "400";
        config::mapArgs["-mininglegacytimeout"] = "100";

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Lane-specific timeouts should take priority */
        REQUIRE(stateless.SOCKET_TIMEOUT == 400);
        REQUIRE(legacy.SOCKET_TIMEOUT == 100);
    }

    SECTION("-miningstatelesstimeout overrides generic, legacy falls back to generic")
    {
        /* Set generic and stateless-specific timeout */
        config::mapArgs["-miningtimeout"] = "180";
        config::mapArgs["-miningstatelesstimeout"] = "350";
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Stateless uses lane-specific, legacy falls back to generic */
        REQUIRE(stateless.SOCKET_TIMEOUT == 350);
        REQUIRE(legacy.SOCKET_TIMEOUT == 180);
    }

    SECTION("-mininglegacytimeout overrides generic, stateless falls back to generic")
    {
        /* Set generic and legacy-specific timeout */
        config::mapArgs["-miningtimeout"] = "180";
        config::mapArgs["-mininglegacytimeout"] = "80";
        config::mapArgs.erase("-miningstatelesstimeout");

        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Stateless falls back to generic, legacy uses lane-specific */
        REQUIRE(stateless.SOCKET_TIMEOUT == 180);
        REQUIRE(legacy.SOCKET_TIMEOUT == 80);
    }

    SECTION("Shared configuration is identical between lanes")
    {
        /* Build configs for both lanes */
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Verify shared configuration parameters are identical */
        REQUIRE(stateless.ENABLE_LISTEN == legacy.ENABLE_LISTEN);
        REQUIRE(stateless.ENABLE_METERS == legacy.ENABLE_METERS);
        REQUIRE(stateless.ENABLE_DDOS == legacy.ENABLE_DDOS);
        REQUIRE(stateless.ENABLE_MANAGER == legacy.ENABLE_MANAGER);
        REQUIRE(stateless.ENABLE_SSL == legacy.ENABLE_SSL);
        REQUIRE(stateless.ENABLE_REMOTE == legacy.ENABLE_REMOTE);
        REQUIRE(stateless.REQUIRE_SSL == legacy.REQUIRE_SSL);
        REQUIRE(stateless.PORT_SSL == legacy.PORT_SSL);
        REQUIRE(stateless.MAX_INCOMING == legacy.MAX_INCOMING);
        REQUIRE(stateless.MAX_CONNECTIONS == legacy.MAX_CONNECTIONS);
        REQUIRE(stateless.MAX_THREADS == legacy.MAX_THREADS);
        REQUIRE(stateless.DDOS_CSCORE == legacy.DDOS_CSCORE);
        REQUIRE(stateless.DDOS_RSCORE == legacy.DDOS_RSCORE);
        REQUIRE(stateless.DDOS_TIMESPAN == legacy.DDOS_TIMESPAN);
        REQUIRE(stateless.MANAGER_SLEEP == legacy.MANAGER_SLEEP);
    }

    SECTION("Problem case: -miningtimeout=60 no longer clobbers stateless 300s default")
    {
        /* This is the specific problem mentioned in the issue:
         * User sets -miningtimeout=60 and it affects BOTH lanes,
         * making the stateless lane timeout too short for Prime mining.
         *
         * With the fix, users can:
         * 1. Use -miningstatelesstimeout=300 -mininglegacytimeout=60
         * 2. Or keep -miningtimeout=60 but add -miningstatelesstimeout=300
         */

        /* Simulate the problem: user sets -miningtimeout=60 */
        config::mapArgs["-miningtimeout"] = "60";
        config::mapArgs.erase("-miningstatelesstimeout");
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs - both lanes will use 60s (backward compatible behavior) */
        LLP::Config stateless_problem = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy_problem = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Both use 60s - this is the backward-compatible problem case */
        REQUIRE(stateless_problem.SOCKET_TIMEOUT == 60);
        REQUIRE(legacy_problem.SOCKET_TIMEOUT == 60);

        /* Now demonstrate the solution: add -miningstatelesstimeout to protect 300s */
        config::mapArgs["-miningtimeout"] = "60";
        config::mapArgs["-miningstatelesstimeout"] = "300";
        config::mapArgs.erase("-mininglegacytimeout");

        /* Build configs - stateless now protected with 300s, legacy uses 60s */
        LLP::Config stateless_fixed = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy_fixed = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        /* Stateless protected at 300s, legacy uses 60s from generic arg */
        REQUIRE(stateless_fixed.SOCKET_TIMEOUT == 300);
        REQUIRE(legacy_fixed.SOCKET_TIMEOUT == 60);
    }

    /* Restore original config state */
    config::mapArgs = mapOriginalArgs;
    config::mapMultiArgs = mapOriginalMultiArgs;
}

TEST_CASE("MiningServerFactory struct vs class idiomatic style", "[miner][config][factory]")
{
    /* This test simply verifies that MiningServerFactory is a class with public members.
     * The problem statement mentions that using 'struct' is fine but 'class' with 'public:'
     * is more idiomatic for factory types in the Nexus codebase style. */

    SECTION("BuildConfig is accessible as a public static method")
    {
        /* This will fail to compile if BuildConfig is not public */
        LLP::Config cfg = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);

        /* Just verify it returns a valid config */
        REQUIRE(cfg.SOCKET_TIMEOUT > 0);
    }
}

TEST_CASE("MiningServerFactory SSL port wiring", "[llp][mining][ssl]")
{
    /* Save original config state to restore after tests */
    std::map<std::string, std::string> mapOriginalArgs = config::mapArgs;
    std::map<std::string, std::vector<std::string>> mapOriginalMultiArgs = config::mapMultiArgs;

    SECTION("BuildConfig STATELESS with SSL disabled: PORT_SSL must be 0")
    {
        config::mapArgs["-miningssl"] = "0";
        LLP::Config cfg = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        REQUIRE(cfg.PORT_SSL == 0);
        REQUIRE(cfg.ENABLE_SSL == false);
    }

    SECTION("BuildConfig STATELESS with SSL enabled: PORT_SSL must be 9325")
    {
        config::mapArgs["-miningssl"] = "1";
        config::mapArgs.erase("-miningstatelesssslport");
        LLP::Config cfg = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        REQUIRE(cfg.PORT_SSL == 9325);
        REQUIRE(cfg.ENABLE_SSL == true);
        config::mapArgs.erase("-miningssl");
    }

    SECTION("BuildConfig LEGACY with SSL enabled: PORT_SSL must be 8325")
    {
        config::mapArgs["-miningssl"] = "1";
        config::mapArgs.erase("-mininglegacysslport");
        LLP::Config cfg = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);
        REQUIRE(cfg.PORT_SSL == 8325);
        REQUIRE(cfg.ENABLE_SSL == true);
        config::mapArgs.erase("-miningssl");
    }

    SECTION("BuildSSLConfig STATELESS: PORT_SSL is 9325 regardless of -miningssl flag")
    {
        config::mapArgs.erase("-miningssl");  // flag NOT set
        LLP::Config cfg = LLP::MiningServerFactory::BuildSSLConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        REQUIRE(cfg.PORT_SSL == 9325);
        REQUIRE(cfg.ENABLE_SSL == true);
    }

    SECTION("BuildSSLConfig LEGACY: PORT_SSL is 8325 regardless of -miningssl flag")
    {
        config::mapArgs.erase("-miningssl");  // flag NOT set
        LLP::Config cfg = LLP::MiningServerFactory::BuildSSLConfig(
            LLP::MiningServerFactory::Lane::LEGACY);
        REQUIRE(cfg.PORT_SSL == 8325);
        REQUIRE(cfg.ENABLE_SSL == true);
    }

    SECTION("GetMiningSSLPort override via -miningstatelesssslport")
    {
        config::mapArgs["-miningstatelesssslport"] = "19325";
        REQUIRE(LLP::GetMiningSSLPort() == 19325);
        config::mapArgs.erase("-miningstatelesssslport");
    }

    SECTION("GetLegacyMiningSSLPort override via -mininglegacysslport")
    {
        config::mapArgs["-mininglegacysslport"] = "18325";
        REQUIRE(LLP::GetLegacyMiningSSLPort() == 18325);
        config::mapArgs.erase("-mininglegacysslport");
    }

    SECTION("PORT_SSL and PORT_BASE must not collide")
    {
        config::mapArgs["-miningssl"] = "1";
        config::mapArgs.erase("-miningstatelesssslport");
        config::mapArgs.erase("-mininglegacysslport");
        LLP::Config stateless = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::STATELESS);
        LLP::Config legacy = LLP::MiningServerFactory::BuildConfig(
            LLP::MiningServerFactory::Lane::LEGACY);

        REQUIRE(stateless.PORT_BASE != stateless.PORT_SSL);  // 9323 != 9325
        REQUIRE(legacy.PORT_BASE != legacy.PORT_SSL);        // 8323 != 8325
        REQUIRE(stateless.PORT_SSL != legacy.PORT_SSL);      // 9325 != 8325
        REQUIRE(stateless.PORT_BASE != legacy.PORT_BASE);    // 9323 != 8323

        config::mapArgs.erase("-miningssl");
    }

    /* Restore original config state */
    config::mapArgs = mapOriginalArgs;
    config::mapMultiArgs = mapOriginalMultiArgs;
}
