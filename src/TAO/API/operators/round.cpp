/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/round.h>

#include <TAO/API/types/exception.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Round(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our array object. */
        encoding::json jList =
            Operators::Array(jParams, jResult);

        /* Loop through our values to adjust to whole numbers. */
        for(auto& jItem : jList)
        {
            /* Handle for floats. */
            if(!jItem.is_number_float())
                throw Exception(-123, "[", jItem.type_name(), "] unsupported for operator [round]");

            /* Adjust our values by triming at the integer's whole value. */
            jItem = int64_t(jItem.get<double>());
        }

        return jList;
    }
}
