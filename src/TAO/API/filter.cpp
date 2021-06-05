/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/filter.h>
#include <TAO/API/include/evaluate.h>
#include <TAO/API/types/exception.h>

#include <TAO/Register/types/object.h>

#include <Util/include/string.h>

namespace TAO::API
{
    /* Recursive handle for any type of filter by passing in function and type to filter. */
    template<typename ObjectType>
    bool FilterStatement(const encoding::json& jStatement, const ObjectType& rCheck,
                         const std::function<bool (const encoding::json&, const ObjectType&)>& rFunc)
    {
        /* Check for final depth and call our object filter there. */
        if(jStatement.find("statement") == jStatement.end())
            return rFunc(jStatement, rCheck);

        /* Check for logical operator. */
        std::string strLogical = "NONE";
        if(jStatement.find("logical") != jStatement.end())
            strLogical = jStatement["logical"].get<std::string>();

        /* Check if we need to recurse another level. */
        if(jStatement.find("statement") != jStatement.end())
        {
            /* Loop through our statements recursively. */
            bool fResult = true;
            for(uint32_t n = 0; n < jStatement["statement"].size(); ++n)
            {
                /* Grab a reference of our clause. */
                const encoding::json& jClause = jStatement["statement"][n];

                /* If first iteration, set the result by evaluatino. */
                if(n == 0)
                {
                    fResult = FilterStatement(jClause, rCheck, rFunc);
                    continue;
                }

                /* Handle for an AND operator. */
                if(strLogical == "AND")
                {
                    fResult = fResult && FilterStatement(jClause, rCheck, rFunc);
                    continue;
                }

                /* Handle for an OR operator. */
                if(strLogical == "OR")
                {
                    fResult = fResult || FilterStatement(jClause, rCheck, rFunc);
                    continue;
                }
            }

            return fResult; //this is where a good request will exit the recursion
        }

        /* If we reach here, the generated JSON was malformed in some way. */
        throw APIException(-60, "Query Syntax Error, malformed where clause at ", jStatement.dump(4));
    }


    /* Determines if an object should be included in a list based on input parameters. */
    bool FilterObject(const encoding::json& jParams, const TAO::Register::Object& objCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        return FilterStatement<TAO::Register::Object>(jParams["where"], objCheck, EvaluateObject);
    }


    /* If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field */
    void FilterFieldname(const encoding::json& jParams, encoding::json &jResponse)
    {
        /* Check for fieldname filters. */
        if(jParams.find("fieldname") != jParams.end())
        {
            /* Grab our field string to rebuild response. */
            const std::string strField = jParams["fieldname"].get<std::string>();

            /* Copy over our new field. */
            const encoding::json jRet = { strField, jResponse[strField].get<std::string>() };
            jResponse = jRet;
        }
    }
}
