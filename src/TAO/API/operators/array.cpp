/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/operators/array.h>

#include <TAO/API/include/results.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Array(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Check if already in array form. */
        if(jResult.is_array() && jResult.back().is_primitive())
            return jResult;

        /* Build our array object. */
        encoding::json jRet = encoding::json::array();
        for(const auto& jItem : jResult)
            ResultsToArray(jParams, jItem, jRet);

        return jRet;
    }
}
