/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/qtv_engine.h>

namespace
{
    int SuccessStatus(const int caseId)
    {
        return caseId == 1 ? LLC::QTV::HOOK_STATUS_OK : 9;
    }

    bool SuccessBool(const int caseId)
    {
        return caseId == 1;
    }
}

TEST_CASE("QTVJuliaBridge reports availability and status", "[qtv][hooks][bridge]")
{
    LLC::QTV::QTVJuliaBridge unavailable;
    REQUIRE_FALSE(unavailable.available());
    REQUIRE(unavailable.run_fixture(1) == LLC::QTV::HOOK_STATUS_UNAVAILABLE);
    REQUIRE(unavailable.compare_parity(1) == LLC::QTV::HOOK_STATUS_UNAVAILABLE);

    LLC::QTV::QTVJuliaBridge bridge(&SuccessStatus, &SuccessStatus);
    REQUIRE(bridge.available());
    REQUIRE(bridge.run_fixture(1) == LLC::QTV::HOOK_STATUS_OK);
    REQUIRE(bridge.compare_parity(0) != LLC::QTV::HOOK_STATUS_OK);
}

TEST_CASE("QTV engine backends isolate hook behavior", "[qtv][hooks][engine]")
{
    LLC::QTV::NullQTVEngine nullEngine;
    REQUIRE_FALSE(nullEngine.RunFixture(1));
    REQUIRE_FALSE(nullEngine.CompareParity(1));

    LLC::QTV::CppQTVEngine cppEngine(&SuccessBool, &SuccessBool);
    REQUIRE(cppEngine.Available());
    REQUIRE(cppEngine.RunFixture(1));
    REQUIRE_FALSE(cppEngine.CompareParity(0));

    LLC::QTV::JuliaQTVEngine juliaEngine(LLC::QTV::QTVJuliaBridge(&SuccessStatus, &SuccessStatus));
    REQUIRE(juliaEngine.Available());
    REQUIRE(juliaEngine.RunFixture(1));
    REQUIRE_FALSE(juliaEngine.CompareParity(0));
}
