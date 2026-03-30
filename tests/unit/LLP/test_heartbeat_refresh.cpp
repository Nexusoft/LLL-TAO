/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_constants.h>
#include <LLP/include/stateless_manager.h>
#include <Util/include/config.h>

#include <cstdint>
#include <string>

using namespace LLP;

/**
 * Template Age and SIM-LINK Tests
 *
 * Tests for template age constants and SIM-LINK session management.
 * Template refresh during dry spells is handled on the miner side.
 */

//=============================================================================
// Section 1: Constant value tests
//=============================================================================

TEST_CASE("FalconConstants: MAX_TEMPLATE_AGE_SECONDS", "[constants]")
{
    SECTION("Hard template age cutoff equals 600 seconds")
    {
        REQUIRE(FalconConstants::MAX_TEMPLATE_AGE_SECONDS == 600u);
    }
}


//=============================================================================
// Section 2: SIM-LINK deprecation flag tests
//=============================================================================

TEST_CASE("SIM-LINK: -deprecate-simlink-fallback config flag", "[deprecation]")
{
    SECTION("Flag defaults to false (fallback enabled by default)")
    {
        /* Without setting the flag, cross-lane resolution is still active */
        const bool fDeprecated = config::GetBoolArg("-deprecate-simlink-fallback", false);
        REQUIRE_FALSE(fDeprecated);
    }

    SECTION("FindSessionBlock returns nullptr for unknown session (unchanged behaviour)")
    {
        /* Even with the flag disabled, an unknown session still returns nullptr */
        const uint32_t nUnknownSession = 0xDEADC0DE;
        const uint512_t fakeMerkle(uint64_t(0xABCDEF));

        auto pBlock = StatelessMinerManager::Get().FindSessionBlock(
            nUnknownSession, fakeMerkle);

        REQUIRE(pBlock == nullptr);
    }

    SECTION("PruneSessionBlocks is safe for unknown session (no crash on deprecation path)")
    {
        const uint32_t nUnknownSession = 0xFEEDFACE;
        REQUIRE_NOTHROW(
            StatelessMinerManager::Get().PruneSessionBlocks(nUnknownSession));
    }
}
