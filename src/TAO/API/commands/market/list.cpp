/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>
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
        const std::pair<uint256_t, uint256_t> pairMarket = ExtractMarket(jParams);

        /* Get a list of our active orders. */
        std::vector<std::pair<uint512_t, uint32_t>> vOrders;
        if(!LLD::Logical->ListOrders(pairMarket, vOrders))
            throw Exception(-24, "Market pair not found");

        /* Build our list of orders now. */
        encoding::json jRet = encoding::json::array();
        for(const auto& pairOrder : vOrders)
        {
            /* Build our order's json. */
            const encoding::json jOrder =
            {
                { "txid",     pairOrder.first.ToString() },
                { "contract", pairOrder.second           }
            };

            jRet.push_back(jOrder);
        }

        return jRet;
    }
}
