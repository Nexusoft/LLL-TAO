/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/create.h>

#include <unit/catch2/catch.hpp>

TEST_CASE("Trust register defaults remain stake-safe", "[stakepool][stake]")
{
    TAO::Register::Object trust = TAO::Register::CreateTrust();

    REQUIRE(trust.Parse());
    REQUIRE(trust.Standard() == TAO::Register::OBJECTS::TRUST);
    REQUIRE(trust.Base() == TAO::Register::OBJECTS::ACCOUNT);

    REQUIRE(trust.get<uint64_t>("balance") == 0);
    REQUIRE(trust.get<uint64_t>("trust") == 0);
    REQUIRE(trust.get<uint64_t>("stake") == 0);
    REQUIRE(trust.get<uint256_t>("token") == 0);
}

TEST_CASE("Trust register stake fields remain mutable", "[stakepool][stake]")
{
    TAO::Register::Object trust = TAO::Register::CreateTrust();
    REQUIRE(trust.Parse());

    trust.Write("balance", uint64_t(5000));
    trust.Write("trust", uint64_t(42));
    trust.Write("stake", uint64_t(21));
    trust.SetChecksum();

    REQUIRE(trust.get<uint64_t>("balance") == 5000);
    REQUIRE(trust.get<uint64_t>("trust") == 42);
    REQUIRE(trust.get<uint64_t>("stake") == 21);
}
