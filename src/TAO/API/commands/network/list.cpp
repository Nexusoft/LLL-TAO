/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/types/commands/network.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the current logged in sessions for -multiusername mode. */
    encoding::json Network::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc", strColumn = "lastseen";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setAddresses({}, CompareResults(strOrder, strColumn));

        /* Get addresses from manager. */
        std::vector<LLP::TrustAddress> vAddr;
        if(LLP::TRITIUM_SERVER->GetAddressManager())
            LLP::TRITIUM_SERVER->GetAddressManager()->GetAddresses(vAddr, CONNECT_FLAGS_ALL);

        /* Build our return JSON data. */
        for(const auto& rAddr : vAddr)
        {
            /* Check for invalid addresses. */
            if(rAddr.nLatency == std::numeric_limits<uint32_t>::max() || rAddr.nLastSeen == 0)
                continue;

            /* Build our JSON object. */
            encoding::json jAddr =
            {
                { "address",  rAddr.ToStringIP() },
                { "lastseen", rAddr.nLastSeen    },
                { "latency",  rAddr.nLatency     }
            };

            /* Check that we match our filters. */
            if(!FilterResults(jParams, jAddr))
                continue;

            /* Filter out our expected fieldnames if specified. */
            if(!FilterFieldname(jParams, jAddr))
                continue;

            setAddresses.insert(jAddr);
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jRegister : setAddresses)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jRegister);
        }

        return jRet;
    }
}
