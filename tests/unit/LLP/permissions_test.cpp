/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/permissions.h>
#include <LLP/include/port.h>

#include <Util/include/args.h>

/* NOTE: CheckPermissions() caches its notion of "standard" ports (and a snapshot of
 * config::mapIPFilters) in function-local statics that are only ever initialized on the
 * *first* call made anywhere in the process. This test file is the only caller of
 * CheckPermissions() in the unit-test binary, so all whitelist configuration below must be
 * put in place before the first invocation and every REQUIRE() in this file must therefore
 * live inside a single TEST_CASE (no SECTIONs) so it always executes exactly once, in order. */
TEST_CASE("CheckPermissions enforces the -llpallowip whitelist on non-standard ports on testnet", "[llp][permissions]")
{
    /* This suite runs under the global testnet configuration established by
     * tests/unit/main.cpp ("Arguments Tests" sets config::fTestNet = true). Confirm that
     * assumption rather than silently relying on ordering. */
    REQUIRE(config::fTestNet.load() == true);

    /* Pick a port that is never "standard" on either network (mining is not part of the
     * Tritium/time/lookup default-open set) and whitelist a single address on it before the
     * very first CheckPermissions() call freezes the filter snapshot. */
    const uint16_t nRestrictedPort = LLP::GetMiningPort();
    config::mapIPFilters[nRestrictedPort] = std::vector<std::string>{ "10.1.2.3" };

    /* Localhost is always permitted, regardless of port or whitelist state. */
    REQUIRE(CheckPermissions("127.0.0.1", nRestrictedPort) == true);
    REQUIRE(CheckPermissions("::1", nRestrictedPort) == true);

    /* Regression guard for the testnet whitelist bypass: a non-standard port (mining) with no
     * matching whitelist entry must be DENIED on testnet. Before the fix, CheckPermissions()
     * unconditionally set fStandardPort = true for every port whenever config::fTestNet was
     * true, which allowed any address on any port (mining, lookup, RPC, API, ...). */
    REQUIRE(CheckPermissions("8.8.8.8", nRestrictedPort) == false);

    /* An address matching the configured -llpallowip whitelist entry for that same restricted
     * port must be permitted. */
    REQUIRE(CheckPermissions("10.1.2.3", nRestrictedPort) == true);

    /* A non-matching address on the same restricted port must still be denied. */
    REQUIRE(CheckPermissions("10.1.2.4", nRestrictedPort) == false);

    /* The hard-coded testnet time-server port remains a standard, default-open port and needs
     * no whitelist entry. */
    REQUIRE(CheckPermissions("8.8.8.8", LLP::GetTimePort()) == true);

    /* The Tritium message port (and its SSL variant) remain default-open on testnet too, since
     * TRITIUM_PORT_CHECK/TRITIUM_SSL_PORT_CHECK are applied regardless of network. */
    REQUIRE(CheckPermissions("8.8.8.8", LLP::GetDefaultPort()) == true);

    /* The lookup port is intentionally always allowed (pre-existing, unrelated behavior) --
     * confirm this long-standing contract remains intact after the fix. */
    REQUIRE(CheckPermissions("8.8.8.8", LLP::GetLookupPort()) == true);
}
