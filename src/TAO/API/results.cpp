/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/check.h>
#include <TAO/API/include/results.h>
#include <TAO/API/types/exception.h>

#include <Util/include/string.h>
#include <Util/types/precision.h>

namespace TAO::API
{
    /* Convert a given json results queue into a compiled array of only values. */
    void ResultsToArray(const encoding::json& jParams, const encoding::json& jResponse, encoding::json &jArray)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "fieldname", "string, array"))
            return;

        /* Handle if single string. */
        if(jParams["request"]["fieldname"].is_string())
        {
            /* Grab our field string to rebuild response. */
            const std::string strField = jParams["request"]["fieldname"].get<std::string>();

            /* Build our return value. */
            if(!ResultsToArray(strField, jResponse, jArray))
                throw Exception(-119, "[", strField, "] field does not exist for result");

            /* Set our return value. */
            return;
        }

        /* Handle if multiple fields. */
        if(jParams["request"]["fieldname"].is_array())
            throw Exception(-66, "Cannot use aggregate fieldnames with operations");
    }


    /* Convert a given json results queue into a compiled array of only values. */
    bool ResultsToArray(const std::string& strField, const encoding::json& jResponse, encoding::json &jArray)
    {
        /* Check for recursion. */
        const auto nFind = strField.find(".");
        if(nFind != strField.npos)
        {
            /* Check that our filter is valid. */
            const std::string strNext = strField.substr(0, nFind);
            if(jResponse.find(strNext) != jResponse.end())
            {
                /* Adjust our new fieldname for recursion. */
                const std::string strAdjusted = strField.substr(nFind + 1);

                /* Use tail recursion to traverse json depth. */
                return ResultsToArray(strAdjusted, jResponse[strNext], jArray);
            }
        }

        /* Check for array to iterate. */
        if(jResponse.is_array())
        {
            /* Loop through all entries and aggregate objects. */
            for(uint32_t n = 0; n < jResponse.size(); ++n)
                ResultsToArray(strField, jResponse[n], jArray);

            return true;
        }

        /* Check for object to evaluate. */
        if(jResponse.is_object())
        {
            /* Check for missing field. */
            if(jResponse.find(strField) == jResponse.end())
                return false;

            /* Grab a reference of our field. */
            const encoding::json& jField = jResponse[strField];

            /* Check for string. */
            if(jField.is_string())
            {
                /* Check if this string is a number in hiding. */
                const std::string strValue = jField.get<std::string>();
                if(IsAllDigit(strValue))
                {
                    /* If we are parse this and return. */
                    jArray.push_back(std::stoull(strValue));

                    return true;
                }

                /* Otherwise treat like regular string. */
                jArray.push_back(strValue);
            }

            /* Check for unsigned int. */
            else if(jField.is_number_unsigned())
                jArray.push_back(jField.get<uint64_t>());

            /* Check for signed int. */
            else if(jField.is_number_integer())
                jArray.push_back(jField.get<int64_t>());

            /* Check for floating point. */
            else if(jField.is_number_float())
                jArray.push_back(precision_t(jField.dump()).double_t());

            /* Check for invalid object. */
            else if(jField.is_object())
                throw Exception(-36, FUNCTION, "Unsupported type [object] for command");

            /* Check for invalid object. */
            else if(jField.is_array())
                throw Exception(-36, FUNCTION, "Unsupported type [array] for command");

            return true;
        }

        return false;
    }
}
