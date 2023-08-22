/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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
    encoding::json Market::User(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our type strings. */
        const std::set<std::string> setTypes =
            ExtractTypes(jParams);

        /* Get our base currency. */
        const uint256_t hashBase =
            ExtractToken(jParams);

        /* Check for invalid token. */
        if(hashBase == 0)
            throw Exception(-56, "Missing Parameter [token]");

        /* Get our user genesis. */
        const uint256_t hashGenesis =
            ExtractGenesis(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc", strColumn = "timestamp";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our return value. */
        encoding::json jRet =
            encoding::json::object();

        /* Handle for our orders key. */
        if(setTypes.find("order") != setTypes.end())
        {
            /* Get a list of our active orders. */
            std::vector<std::pair<uint512_t, uint32_t>> vOrders;
            if(LLD::Logical->ListOrders(hashGenesis, vOrders))
            {
                /* Build our object list and sort on insert. */
                std::set<encoding::json, CompareResults> setOrders({}, CompareResults(strOrder, strColumn));

                /* Build our list of orders now. */
                for(const auto& pairOrder : vOrders)
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
                        OrderToJSON(tContract, hashBase);

                    /* Check for valid base token. */
                    const std::pair<uint256_t, uint256_t> pairMarket = std::make_pair
                    (
                        TAO::Register::Address(jOrder["contract"]["token"].get<std::string>()),
                        TAO::Register::Address(jOrder["order"]["token"].get<std::string>())
                    );

                    /* Check that this the market we are looking for. */
                    if(hashBase != pairMarket.first && hashBase != pairMarket.second)
                        continue;

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
                encoding::json jOrders = encoding::json::array();

                /* Handle paging and offsets. */
                uint32_t nTotal = 0;
                for(const auto& jOrder : setOrders)
                {
                    /* Check the offset. */
                    if(++nTotal <= nOffset)
                        continue;

                    /* Check the limit */
                    if(jOrders.size() == nLimit)
                        break;

                    jOrders.push_back(jOrder);
                }

                /* Add to our return value. */
                jRet["orders"] = jOrders;
            }
            else
                jRet["orders"] = encoding::json::array();
        }

        /* Handle for our orders key. */
        if(setTypes.find("executed") != setTypes.end())
        {
            /* Get a list of our active orders. */
            std::vector<std::pair<uint512_t, uint32_t>> vOrders;
            if(LLD::Logical->ListExecuted(hashGenesis, vOrders))
            {
                /* Build our object list and sort on insert. */
                std::set<encoding::json, CompareResults> setOrders({}, CompareResults(strOrder, strColumn));

                /* Build our list of orders now. */
                for(const auto& pairOrder : vOrders)
                {
                    /* Get our contract now. */
                    const TAO::Operation::Contract tContract =
                        LLD::Ledger->ReadContract(pairOrder.first, pairOrder.second);

                    /* Unpack our register address. */
                    uint256_t hashRegister;
                    if(!TAO::Register::Unpack(tContract, hashRegister))
                        continue;

                    /* Get our order's json. */
                    encoding::json jOrder =
                        OrderToJSON(tContract, hashBase);

                    /* Check for valid base token. */
                    const std::pair<uint256_t, uint256_t> pairMarket = std::make_pair
                    (
                        TAO::Register::Address(jOrder["contract"]["token"].get<std::string>()),
                        TAO::Register::Address(jOrder["order"]["token"].get<std::string>())
                    );

                    /* Check that this the market we are looking for. */
                    if(hashBase != pairMarket.first && hashBase != pairMarket.second)
                        continue;

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
                encoding::json jOrders = encoding::json::array();

                /* Handle paging and offsets. */
                uint32_t nTotal = 0;
                for(const auto& jOrder : setOrders)
                {
                    /* Check the offset. */
                    if(++nTotal <= nOffset)
                        continue;

                    /* Check the limit */
                    if(jOrders.size() == nLimit)
                        break;

                    jOrders.push_back(jOrder);
                }

                /* Add to our return value. */
                jRet["executed"] = jOrders;
            }
            else
                jRet["executed"] = encoding::json::array();
        }

        return jRet;
    }
}
