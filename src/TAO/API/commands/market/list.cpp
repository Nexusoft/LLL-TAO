/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/market.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Market::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Grab our market pair. */
        const std::pair<uint256_t, uint256_t> pairMarket  = ExtractMarket(jParams);
        const std::pair<uint256_t, uint256_t> pairReverse = std::make_pair(pairMarket.second, pairMarket.first);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc", strColumn = "timestamp";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Get a list of our active orders. */
        std::vector<std::pair<uint512_t, uint32_t>> vOrders;
        if(!LLD::Logical->ListOrders(pairMarket, vOrders) && !LLD::Logical->ListOrders(pairReverse, vOrders))
            throw Exception(-24, "Market pair not found");

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setOrders({}, CompareResults(strOrder, strColumn));

        /* Build our list of orders now. */
        for(const auto& pairOrder : vOrders)
        {
            /* Get our contract now. */
            const TAO::Operation::Contract tContract =
                LLD::Ledger->ReadContract(pairOrder.first, pairOrder.second);

            /* Get our order's json. */
            encoding::json jOrder =
                OrderToJSON(tContract, pairMarket);

            /* Check that we match our filters. */
            if(!FilterResults(jParams, jOrder))
                continue;

            /* Filter out our expected fieldnames if specified. */
            if(!FilterFieldname(jParams, jOrder))
                continue;

            /* Insert into set and automatically sort. */
            setOrders.insert(jOrder);
        }

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jOrder : setOrders)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jOrder);
        }

        return jRet;
    }
}
