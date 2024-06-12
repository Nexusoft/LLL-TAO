/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/assets.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/compare.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/list.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic method to list object registers by sig chain*/
    encoding::json Assets::Partial(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Genesis ID. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc", strColumn = "modified";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Get the list of registers owned by this sig chain */
        std::map<uint256_t, std::pair<Accounts, uint256_t>> mapAssets;
        if(!ListPartial(hashGenesis, mapAssets))
            throw Exception(-74, "No registers found");

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setRegisters({}, CompareResults(strOrder, strColumn));

        /* Add the register data to the response */
        for(const auto& pairAsset : mapAssets)
        {
            /* Grab our asset from disk. */
            TAO::Register::Object oAsset;
            if(!LLD::Register->ReadObject(pairAsset.second.second, oAsset))
                continue;

            /* Read our token now. */
            TAO::Register::Object oToken;
            if(!LLD::Register->ReadObject(oAsset.hashOwner, oToken))
                continue;

            /* Cache our maximum balance for given token. */
            const uint64_t nBalance =
                pairAsset.second.first.MaxBalance();

            /* Calculate our partial ownership now. */
            const uint64_t nOwnership =
                (nBalance * 10000) / oToken.get<uint64_t>("supply"); //we use 10000 as 100 * 100 for 100.00 percent displayed

            /* Populate the response */
            encoding::json jRegister =
                RegisterToJSON(oAsset, pairAsset.second.second);

            /* Add our ownership value to register. */
            jRegister["ownership"] = double(nOwnership / 100.0);

            /* Check that we match our filters. */
            if(!FilterResults(jParams, jRegister))
                continue;

            /* Filter out our expected fieldnames if specified. */
            if(!FilterFieldname(jParams, jRegister))
                continue;

            /* Insert into set and automatically sort. */
            setRegisters.insert(jRegister);
        }

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

        return jRet;
    }
}
