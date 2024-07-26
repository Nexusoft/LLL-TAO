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
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* List records from local database. */
    encoding::json Local::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Sort order to apply */
        std::string strOrder = "desc";

        /* Get the jParams to apply to the response. */
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Check for our parameters. */
        if(!CheckParameter(jParams, "table", "string"))
            throw Exception(-81, "missing or invalid parameter [table=string]");

        /* Get our table to list items from. */
        const std::string& strTable =
            jParams["table"].get<std::string>();

        /* Get a copy of our list vector. */
        std::vector<std::pair<std::string, std::string>> vRecords;
        LLD::Local->ListRecords(strTable, vRecords);

        /* Build our return value. */
        encoding::json jRet = encoding::json::object();

        /* Grab our list of records. */
        uint32_t nTotal = 0;
        for(const auto& tRecord : vRecords)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            /* Add as a JSON object. */
            jRet[tRecord.first] = tRecord.second;
        }

        /* Check that we match our filters. */
        FilterResults(jParams, jRet);

        /* Filter out our expected fieldnames if specified. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
