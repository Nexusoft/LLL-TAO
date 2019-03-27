/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/sigchain.h>

#include <unit/catch2/catch.hpp>

#include <Util/include/debug.h>

TEST_CASE( "Signature Chain Generation", "[sigchain]" )
{

    TAO::Ledger::SignatureChain user = TAO::Ledger::SignatureChain("user", "password");

    REQUIRE(user.Genesis() == uint256_t("0xb5254d24183a77625e2dbe0c63570194aca6fb7156cb84edf3e238f706b51019"));

    REQUIRE(user.Generate(0, "pin") == uint512_t("0x2986e8254e45ce87484feb0cbb9a961588dfe040bf109662f1235d97e57745fdcfae12ed46ba8a523bf490caf9461c9ef8dbc68bbbe8c62ea484ec0fc519f00c"));
}
