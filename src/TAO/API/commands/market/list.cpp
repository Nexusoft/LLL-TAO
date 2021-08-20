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

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Market::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our type strings. */
        const std::set<std::string> setTypes =
            ExtractTypes(jParams);

        /* Track if this is a catch-all. */
        const bool fAll =
            (setTypes.find("order") != setTypes.end());

        /* Grab our market pair. */
        const std::pair<uint256_t, uint256_t> pairMarket  = ExtractMarket(jParams);
        const std::pair<uint256_t, uint256_t> pairReverse = std::make_pair(pairMarket.second, pairMarket.first);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc", strColumn = "price";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our return value. */
        encoding::json jRet =
            encoding::json::object();

        /* Check for our bids type. */
        if(setTypes.find("bid") != setTypes.end() || fAll)
        {
            /* Get a list of our active orders. */
            std::vector<std::pair<uint512_t, uint32_t>> vBids;
            if(LLD::Logical->ListOrders(pairMarket, vBids))
            {
                /* Build our object list and sort on insert. */
                std::set<encoding::json, CompareResults> setBids({}, CompareResults(strOrder, strColumn));

                /* Build our list of orders now. */
                for(const auto& pairOrder : vBids)
                {
                    /* Get our contract now. */
                    const TAO::Operation::Contract tContract =
                        LLD::Ledger->ReadContract(pairOrder.first, pairOrder.second);

                    /* Unpack our register address. */
                    uint256_t hashRegister;
                    if(!TAO::Register::Unpack(tContract, hashRegister))
                        continue;

                    /* Check for a spent proof already. */
                    if(LLD::Ledger->HasProof(hashRegister, pairOrder.first, pairOrder.second))
                        continue;

                    /* Get our order's json. */
                    encoding::json jOrder =
                        OrderToJSON(tContract, pairMarket);

                    /* Check that we match our filters. */
                    if(!FilterResults(jParams, jOrder))
                        continue;

                    /* Insert into set and automatically sort. */
                    setBids.insert(jOrder);
                }

                /* Build our return value. */
                encoding::json jBids = encoding::json::array();

                /* Handle paging and offsets. */
                uint32_t nTotal = 0;
                for(const auto& jOrder : setBids)
                {
                    /* Check the offset. */
                    if(++nTotal <= nOffset)
                        continue;

                    /* Check the limit */
                    if(jBids.size() == nLimit)
                        break;

                    jBids.push_back(jOrder);
                }

                /* Add to our return value. */
                jRet["bids"] = jBids;
            }
            else
                jRet["bids"] = encoding::json::array();
        }

        /* Check for our bids type. */
        if(setTypes.find("ask") != setTypes.end() || fAll)
        {
            /* Get a list of our active orders. */
            std::vector<std::pair<uint512_t, uint32_t>> vAsks;
            if(LLD::Logical->ListOrders(pairReverse, vAsks))
            {
                /* Build our object list and sort on insert. */
                std::set<encoding::json, CompareResults> setAsks({}, CompareResults(strOrder, strColumn));

                /* Build our list of orders now. */
                for(const auto& pairOrder : vAsks)
                {
                    /* Get our contract now. */
                    const TAO::Operation::Contract tContract =
                        LLD::Ledger->ReadContract(pairOrder.first, pairOrder.second);

                    /* Unpack our register address. */
                    uint256_t hashRegister;
                    if(!TAO::Register::Unpack(tContract, hashRegister))
                        continue;

                    /* Check for a spent proof already. */
                    if(LLD::Ledger->HasProof(hashRegister, pairOrder.first, pairOrder.second))
                        continue;

                    /* Get our order's json. */
                    encoding::json jOrder =
                        OrderToJSON(tContract, pairMarket);

                    /* Check that we match our filters. */
                    if(!FilterResults(jParams, jOrder))
                        continue;

                    /* Insert into set and automatically sort. */
                    setAsks.insert(jOrder);
                }

                /* Build our return value. */
                encoding::json jAsks = encoding::json::array();

                /* Handle paging and offsets. */
                uint32_t nTotal = 0;
                for(const auto& jOrder : setAsks)
                {
                    /* Check the offset. */
                    if(++nTotal <= nOffset)
                        continue;

                    /* Check the limit */
                    if(jAsks.size() == nLimit)
                        break;

                    jAsks.push_back(jOrder);
                }

                /* Add to our return value. */
                jRet["asks"] = jAsks;
            }
            else
                jRet["asks"] = encoding::json::array();
        }

        /* Check for completed order summaries. */
        if(setTypes.find("executed") != setTypes.end())
        {
            /* Get a list of our active orders. */
            std::vector<std::pair<uint512_t, uint32_t>> vExecuted;
            if(LLD::Logical->ListExecuted(pairReverse, vExecuted))
            {
                /* Build our object list and sort on insert. */
                std::set<encoding::json, CompareResults> setExecuted({}, CompareResults(strOrder, strColumn));

                /* Build our list of orders now. */
                for(const auto& pairOrder : vExecuted)
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

                    /* Insert into set and automatically sort. */
                    setExecuted.insert(jOrder);
                }

                /* Build our return value. */
                encoding::json jExecuted = encoding::json::array();

                /* Handle paging and offsets. */
                uint32_t nTotal = 0;
                for(const auto& jOrder : setExecuted)
                {
                    /* Check the offset. */
                    if(++nTotal <= nOffset)
                        continue;

                    /* Check the limit */
                    if(jExecuted.size() == nLimit)
                        break;

                    jExecuted.push_back(jOrder);
                }

                /* Add to our return value. */
                jRet["executed"] = jExecuted;
            }
            else
                jRet["executed"] = encoding::json::array();
        }

        /* Filter out our expected fieldnames if specified. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
