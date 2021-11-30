/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/register.h>

#include <TAO/API/include/compare.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>
#include <TAO/API/types/exception.h>

#include <TAO/Ledger/include/stake.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Retrieves the transaction for the given hash. */
    encoding::json Register::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Grab our type to run some checks against. */
        const std::set<std::string> setTypes = ExtractTypes(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the params to apply to the response. */
        std::string strOrder = "desc", strColumn = "modified";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setRegisters({}, CompareResults(strOrder, strColumn));

        /* Loop through our types. */
        for(const auto& strType : setTypes)
        {
            /* Special handle if address indexed. */
            if(config::GetBoolArg("-indexaddress"))
            {
                /* Batch read up to 1000 at a time */
                std::vector<std::pair<uint256_t, TAO::Register::Object>> vObjects;
                if(LLD::Register->BatchRead(strType, vObjects, -1))
                {
                    debug::warning("found ", vObjects.size(), " records");

                    /* Add the register data to the response */
                    for(auto& rObject : vObjects)
                    {
                        /* Parse our object now. */
                        if(rObject.second.nType == TAO::Register::REGISTER::OBJECT && !rObject.second.Parse())
                            continue;

                        /* Check our object standards. */
                        if(!CheckStandard(jParams, rObject.second))
                            continue;

                        /* Populate the response */
                        encoding::json jRegister =
                            StandardToJSON(jParams, rObject.second, rObject.first);

                        /* Check that we match our filters. */
                        if(!FilterObject(jParams, jRegister, rObject.second))
                            continue;

                        /* Filter out our expected fieldnames if specified. */
                        if(!FilterFieldname(jParams, jRegister))
                            continue;

                        /* Insert into set and automatically sort. */
                        setRegisters.insert(jRegister);
                    }
                }
            }
            else
            {
                /* Batch read up to 1000 at a time */
                std::vector<TAO::Register::Object> vObjects;
                if(LLD::Register->BatchRead(strType, vObjects, -1))
                {
                    debug::warning("found ", vObjects.size(), " records");

                    /* Add the register data to the response */
                    for(auto& rObject : vObjects)
                    {
                        /* Parse our object now. */
                        if(rObject.nType == TAO::Register::REGISTER::OBJECT && !rObject.Parse())
                            continue;

                        /* Check our object standards. */
                        if(!CheckStandard(jParams, rObject))
                            continue;

                        /* Populate the response */
                        encoding::json jRegister =
                            StandardToJSON(jParams, rObject);

                        /* Check that we match our filters. */
                        if(!FilterObject(jParams, jRegister, rObject))
                            continue;

                        /* Filter out our expected fieldnames if specified. */
                        if(!FilterFieldname(jParams, jRegister))
                            continue;

                        /* Insert into set and automatically sort. */
                        setRegisters.insert(jRegister);
                    }
                }
            }
        }

        /* Check that we have results. */
        if(setRegisters.empty())
            throw Exception(-74, "No registers found");

        /* Check that our offset is in range. */
        if(nOffset > setRegisters.size())
            throw Exception(-75, "Value [offset=", nOffset, "] exceeds dataset size [", setRegisters.size(), "]");

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();

        /* Handle paging and offsets. */
        uint32_t nTotal = 0;
        for(const auto& jRegister : setRegisters)
        {
            /* Check the offset. */
            if(++nTotal <= nOffset)
                continue;

            /* Check the limit */
            if(jRet.size() == nLimit)
                break;

            jRet.push_back(jRegister);
        }

        /* Check for over paging. */
        if(jRet.empty())
            throw Exception(-75, "Value [offset=", nOffset, "] + [limit=", nLimit, "] exceeds dataset size [", setRegisters.size(), "]");

        return jRet;
    }
}
