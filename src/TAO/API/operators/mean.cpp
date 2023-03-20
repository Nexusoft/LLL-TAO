/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>
#include <TAO/API/types/operators/mean.h>
#include <TAO/API/types/operators/sum.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Mean(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Extract our fieldname. */
        const std::string strField =
            ExtractFieldname(jParams);

        /* Loop through to calculate sum. */
        encoding::json jRet =
            Operators::Sum(jParams, jResult);

        /* Handle for unsigned signed integers. */
        if(jRet[strField].is_number_unsigned())
        {
            /* Grab our values. */
            const uint64_t nValue = jRet[strField].get<uint64_t>();

            /* Add to our output value. */
            jRet[strField] = (nValue / jResult.size());
            return jRet;
        }

        /* Handle for signed integers. */
        if(jRet[strField].is_number_integer())
        {
            /* Grab our values. */
            const int64_t nValue = jRet[strField].get<int64_t>();

            /* Add to our output value. */
            jRet[strField] = (nValue / jResult.size());
            return jRet;
        }

        /* Handle for floats. */
        if(jRet[strField].is_number_float())
        {
            /* Grab our values. */
            const precision_t dValue =
                precision_t(jRet[strField].dump());

            /* Add to our output value. */
            jRet[strField] = (dValue / jResult.size()).double_t();
            return jRet;
        }

        /* If we fall through all checks, we are an invalid type. */
        throw Exception(-123, "[", jRet[strField].type_name(), "] unsupported for operator [sum]");
    }
}
