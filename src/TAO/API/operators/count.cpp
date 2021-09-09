/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/count.h>

#include <TAO/API/include/results.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Count(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our array object. */
        const encoding::json jRet =
            Operators::Array(jParams, jResult);

        return { {"count", jRet.size() }};
    }
}
