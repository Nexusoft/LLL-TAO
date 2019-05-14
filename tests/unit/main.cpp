/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <unit/catch2/catch.hpp>

#include <LLD/include/global.h>

#include <Util/include/filesystem.h>
#include <Util/include/args.h>

TEST_CASE("Arguments Tests", "[args]")
{
    config::fTestNet = true;
    config::mapArgs["-testnet"] = "92349234";

    REQUIRE(config::fTestNet == true);
    REQUIRE(config::GetArg("-testnet", 0) == 92349234);

    //get the data directory
    std::string strPath = config::GetDataDir();

    //test the filesystem remove and also clear from previous unit tests
    if(filesystem::exists(strPath))
    {
        REQUIRE(filesystem::remove(strPath));
        REQUIRE(!filesystem::exists(strPath));
    }

    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::locDB = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::legDB = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);


    /* Initialize the Legacy Database. */
    LLD::trustDB  = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::legacyDB = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
}
