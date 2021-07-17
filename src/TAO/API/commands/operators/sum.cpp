/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/operators.h>
#include <TAO/API/types/exception.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Sum(const encoding::json& jParams, const encoding::json& jList)
    {
        /* Check that list is an array. */
        if(!jList.is_array())
            throw Exception(-123, "[", jList.type_name(), "] unsupported for operator [op]");

        /* Check for fieldname. */
        if(jParams.find("fieldname") == jParams.end())
            throw Exception(-56, "Missing Parameter [fieldname]");

        /* Only operate on single fields for now. */
        if(!jParams["fieldname"].is_string())
            throw Exception(-121, "Aggregated type [fieldname] not allowed for [sum]");

        /* Grab the fieldname we are operating on. */
        const std::string& strField =
            jParams["fieldname"].get<std::string>();

        /* Loop through our list. */
        encoding::json jRet = encoding::json::object();
        for(const auto& jItem : jList)
        {
            /* Check that field exists. */
            if(jItem.find(strField) == jItem.end())
                throw Exception(-71, "Fieldname [", strField, "] doesn't exist");

            /* Handle for strings. */
            if(jItem[strField].is_string())
            {
                /* Grab our current value. */
                std::string strCurrent = "";
                if(jRet.find(strField) != jRet.end())
                    strCurrent = jRet[strField].get<std::string>();

                /* Grab our values. */
                const std::string strValue = jItem[strField].get<std::string>();

                /* Add to our output value. */
                jRet[strField] = (strCurrent + strValue);

                continue;
            }

            /* Handle for unsigned signed integers. */
            if(jItem[strField].is_number_unsigned())
            {
                /* Grab our current value. */
                uint64_t nCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    nCurrent = jRet[strField].get<uint64_t>();

                /* Grab our values. */
                const uint64_t nValue = jItem[strField].get<uint64_t>();

                /* Add to our output value. */
                jRet[strField] = (nCurrent + nValue);

                continue;
            }

            /* Handle for signed integers. */
            if(jItem[strField].is_number_integer())
            {
                /* Grab our current value. */
                int64_t nCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    nCurrent = jRet[strField].get<int64_t>();

                /* Grab our values. */
                const int64_t nValue = jItem[strField].get<int64_t>();

                /* Add to our output value. */
                jRet[strField] = (nCurrent + nValue);

                continue;
            }

            /* Handle for floats. */
            if(jItem[strField].is_number_float())
            {
                /* Grab our current value. */
                double dCurrent = 0;
                if(jRet.find(strField) != jRet.end())
                    dCurrent = jRet[strField].get<double>();

                /* Grab our values. */
                const double dValue = jItem[strField].get<double>();

                /* Add to our output value. */
                jRet[strField] = (dCurrent + dValue);

                continue;
            }

            /* If we fall through all checks, we are an invalid type. */
            throw Exception(-123, "[", jItem[strField].type_name(), "] unsupported for operator [sum]");
        }

        return jRet;
    }
}
