/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/* [Option D] Tests for the stall-detection clock behind the mining lanes'
 * stall-aware fresh-template-push throttle. */

#include <TAO/Ledger/include/chainstate.h>

#include <unit/catch2/catch.hpp>


TEST_CASE("SecondsSinceTipChange resets when the tip hash changes", "[chainstate][stall]")
{
    /* Save and restore the global tip hash so other tests are unaffected. */
    const uint1024_t hashPrev = TAO::Ledger::ChainState::hashBestChain.load();

    /* Prime the clock on an arbitrary tip. */
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(0x1111));
    REQUIRE(TAO::Ledger::ChainState::SecondsSinceTipChange() == 0);

    /* An unchanged tip accumulates elapsed time (>= 0 immediately after). */
    REQUIRE(TAO::Ledger::ChainState::SecondsSinceTipChange() <= 1);

    /* A tip change resets the clock to 0. */
    TAO::Ledger::ChainState::hashBestChain.store(uint1024_t(0x2222));
    REQUIRE(TAO::Ledger::ChainState::SecondsSinceTipChange() == 0);

    /* Sanity of the throttle configuration: the stall threshold must be much
     * larger than the reduced-cadence push interval, or the throttle would be
     * pointless. */
    REQUIRE(TAO::Ledger::ChainState::TIP_STALL_THRESHOLD_SECONDS
        > TAO::Ledger::ChainState::STALL_TEMPLATE_PUSH_INTERVAL_SECONDS);

    TAO::Ledger::ChainState::hashBestChain.store(hashPrev);
}
