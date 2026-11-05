/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

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
#include <TAO/API/include/list.h>
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

        /* Build our return value. */
        encoding::json jRet = encoding::json::array();
        for(const auto& strType : setTypes)
        {
            try
            {
                /* Get our standard type. */
                const std::string strStandard =
                    mapStandards[strType].Type();

                /* Special handle if address indexed. */
                if(config::fIndexAddress.load())
                {
                    /* Batch read up to 1000 at a time */
                    std::vector<std::pair<uint256_t, TAO::Register::Object>> vObjects;
                    if(LLD::Register->BatchRead(strStandard + "_address", vObjects, -1))
                    {
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

                            /* Insert into our return value. */
                            jRet.push_back(jRegister);
                        }
                    }
                }
                else
                {
                    /* Batch read up to 1000 at a time */
                    std::vector<TAO::Register::Object> vObjects;
                    if(LLD::Register->BatchRead(strStandard, vObjects, -1))
                    {
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

                            /* Insert into our return value. */
                            jRet.push_back(jRegister);
                        }
                    }
                }
            }
            catch(const std::exception& e){ debug::warning("Exception: ", e.what()); }
        }

        return jRet;
    }
}
