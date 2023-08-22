/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/filter.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/names.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Cancel an active order in market. */
    encoding::json Names::Lookup(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract our address for name lookup. */
        if(!CheckParameter(jParams, "address", "string"))
            throw Exception(-11, "Missing Parameter [address]");

        /* Build our response object. */
        const TAO::Register::Address hashAddress =
            TAO::Register::Address(jParams["address"].get<std::string>());

        /* Handle for NXS hardcoded token name. */
        uint256_t hashName;
        if(!LLD::Logical->ReadPTR(hashAddress, hashName))
            throw Exception(-22, "No PTR Record found: Address has no valid name index");

        /* Get the name object now. */
        TAO::Register::Object tName;
        if(!LLD::Register->ReadObject(hashName, tName))
            throw Exception(-12, "Object does not exist");

        /* Build our response object. */
        encoding::json jRet =
            RegisterToJSON(tName, hashName);

        /* Filter out our expected fieldnames if specified. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
