/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/local.h>
#include <TAO/API/include/check.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Check that a record exists in local database */
    encoding::json Local::Erase(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for our parameters. */
        if(!CheckParameter(jParams, "table", "string"))
            throw Exception(-81, "missing or invalid parameter [table=string]");

        /* Get our table to list items from. */
        const std::string& strTable =
            jParams["table"].get<std::string>();

        /* Check for our parameters. */
        if(!CheckParameter(jParams, "key", "string, number"))
            throw Exception(-81, "missing or invalid parameter [key=string]");

        /* Get our table to list items from. */
        const std::string& strKey =
            jParams["key"].is_number() ? debug::safe_printstr(jParams["key"].get<uint64_t>()) : jParams["key"].get<std::string>();

        /* Check local database for record. */
        return {{ "success", LLD::Local->EraseRecord(strTable, strKey) }};
    }
}
