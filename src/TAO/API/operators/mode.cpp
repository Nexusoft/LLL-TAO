/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/mode.h>

#include <set>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Mode(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our array object. */
        const encoding::json jArray =
            Operators::Array(jParams, jResult);

        /* Check for empty results. */
        if(jArray.empty())
            throw Exception(-123, "Operator [mode] cannot be used on empty result");

        /* Keep track of our highest occurance. */
        std::multiset<encoding::json> setOccurances;

        /* Loop through our entries to find our highest mode. */
        uint32_t nMode = 1;
        for(const auto& jItem : jArray)
        {
            /* Handle for unsigned signed integers. */
            if(!jItem.is_primitive())
                throw Exception(-123, "[", jItem.type_name(), "] unsupported for operator [mode]");

            /* Add to our occurances set. */
            setOccurances.insert(jItem);

            /* Check against highest count. */
            if(setOccurances.count(jItem) > nMode)
                nMode = setOccurances.count(jItem);
        }

        /* Check if no mode found. */
        if(nMode == 1)
            return { {"mode", encoding::json::array() } };

        /* Calculate our mode now. */
        std::pair<uint32_t, encoding::json> pairMode =
            std::make_pair(nMode, encoding::json::array());

        /* Check through our occurances to generate mode. */
        std::set<encoding::json> setResults;
        for(const auto& jItem : setOccurances)
        {
            /* Filter by count greater than 1. */
            if(setOccurances.count(jItem) == pairMode.first)
            {
                /* Check for duplicate occurances during iterating. */
                if(setResults.count(jItem))
                    continue;

                /* Add to our results array now. */
                pairMode.second.push_back(jItem);
                setResults.insert(jItem);
            }
        }

        return { {"mode", pairMode.second } };
    }
}
