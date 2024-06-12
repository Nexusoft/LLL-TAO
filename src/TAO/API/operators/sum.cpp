/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/sum.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Sum(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our list from Array command. */
        const encoding::json jList =
            Operators::Array(jParams, jResult);

        /* Extract our fieldname. */
        const std::string strField =
            ExtractFieldname(jParams);

        /* Loop through to calculate sum. */
        encoding::json jRet = encoding::json::object();
        for(const auto& jItem : jList)
        {
            /* Handle for strings. */
            if(jItem.is_string())
            {
                /* Grab our current value. */
                std::string strCurrent = "";
                if(jRet.find(strField) != jRet.end())
                    strCurrent = jRet[strField].get<std::string>();

                /* Grab our values. */
                const std::string strValue = jItem.get<std::string>();

                /* Add to our output value. */
                jRet[strField] = (strCurrent + strValue);

                continue;
            }

            /* Handle for unsigned signed integers. */
            if(jItem.is_number_unsigned())
            {
                /* Grab our current value. */
                uint64_t nCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    nCurrent = jRet[strField].get<uint64_t>();

                /* Grab our values. */
                const uint64_t nValue = jItem.get<uint64_t>();

                /* Add to our output value. */
                jRet[strField] = (nCurrent + nValue);

                continue;
            }

            /* Handle for signed integers. */
            if(jItem.is_number_integer())
            {
                /* Grab our current value. */
                int64_t nCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    nCurrent = jRet[strField].get<int64_t>();

                /* Grab our values. */
                const int64_t nValue = jItem.get<int64_t>();

                /* Add to our output value. */
                jRet[strField] = (nCurrent + nValue);

                continue;
            }

            /* Handle for floats. */
            if(jItem.is_number_float())
            {
                /* Grab our current value. */
                double dCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    dCurrent = jRet[strField].get<double>();//precision_t(jRet[strField].dump());

                /* Grab our values. */
                const double dSum =
                    jItem.get<double>() + dCurrent;

                /* Add to our output value. */
                jRet[strField] = dSum;

                continue;
            }

            /* If we fall through all checks, we are an invalid type. */
            throw Exception(-123, "[", jItem.type_name(), "] unsupported for operator [sum]");
        }

        return jRet;
    }
}
