/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/templates.h>

//XXX: these static methods should be moved GetOutstanding, GetExpired, get_tokenized_debits, get_coinbases, etc
#include <TAO/API/users/types/users.h>

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
    encoding::json Templates::List(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Genesis ID. */
        const uint256_t hashGenesis = ExtractGenesis(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc", strColumn = "modified";
        ExtractList(jParams, strOrder, strColumn, nLimit, nOffset);

        /* Get the list of registers owned by this sig chain */
        std::vector<TAO::Register::Address> vAddresses;
        ListRegisters(hashGenesis, vAddresses);

        /* Get any registers that have been transferred to this user but not yet paid (claimed) */
        std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vUnclaimed;
        Users::get_unclaimed(hashGenesis, vUnclaimed);

        /* Add the unclaimed register addresses to the list */
        for(const auto& tUnclaimed : vUnclaimed)
            vAddresses.push_back(std::get<2>(tUnclaimed));

        /* Check for empty return. */
        if(vAddresses.size() == 0)
            throw Exception(-74, "No registers found");

        /* Build our object list and sort on insert. */
        std::set<encoding::json, CompareResults> setRegisters({}, CompareResults(strOrder, strColumn));

        /* Add the register data to the response */
        for(const auto& hashRegister : vAddresses)
        {
            /* Grab our object from disk. */
            TAO::Register::Object tObject;
            if(!LLD::Register->ReadObject(hashRegister, tObject))
                continue;

            /* Check our object standards. */
            if(!CheckStandard(jParams, tObject))
                continue;

            /* Populate the response */
            encoding::json jRegister =
                StandardToJSON(jParams, tObject, hashRegister);

            /* Check that we match our filters. */
            if(!FilterObject(jParams, jRegister, tObject))
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
