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
    bool FilterStatement(const encoding::json& jStatement, ObjectType& rCheck,
                         const std::function<bool (const encoding::json&, ObjectType&)>& rFunc)
    {
        /* Check for final depth and call our object filter there. */
        if(jStatement.find("statement") == jStatement.end())
            return rFunc(jStatement, rCheck); //execute evaluation function when reached final level of recursion

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

                /* If first iteration, set the result by evalute. */
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
        throw Exception(-60, "Query Syntax Error, malformed where clause at ", jStatement.dump(4));
    }


    /* Recursive handle for any type of filter by passing in function and type to filter. */
    template<typename ObjectType>
    bool FilterStatement(const encoding::json& jStatement, encoding::json &jCheck, ObjectType& rCheck,
                         const std::function<bool (const encoding::json&, ObjectType&)>& xEvaluate)
    {
        /* Check for final depth and call our object filter there. */
        if(jStatement.find("statement") == jStatement.end())
        {
            /* Check for missing class. */
            if(jStatement.find("class") == jStatement.end())
                throw Exception(-60, "Query Syntax Error, malformed where clause at ", jStatement.dump(4));

            /* Check that we are operating on a json object and use that function. */
            if(jStatement["class"].get<std::string>() == "results")
                return EvaluateResults(jStatement, jCheck);

            return xEvaluate(jStatement, rCheck); //execute evaluation function when reached final level of recursion
        }

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

                /* If first iteration, set the result by evalute. */
                if(n == 0)
                {
                    fResult = FilterStatement(jClause, jCheck, rCheck, xEvaluate);
                    continue;
                }

                /* Handle for an AND operator. */
                if(strLogical == "AND")
                {
                    fResult = fResult && FilterStatement(jClause, jCheck, rCheck, xEvaluate);
                    continue;
                }

                /* Handle for an OR operator. */
                if(strLogical == "OR")
                {
                    fResult = fResult || FilterStatement(jClause, jCheck, rCheck, xEvaluate);
                    continue;
                }
            }

            return fResult; //this is where a good request will exit the recursion
        }

        /* If we reach here, the generated JSON was malformed in some way. */
        throw Exception(-60, "Query Syntax Error, malformed where clause at ", jStatement.dump(4));
    }


    /* Helper filter, to handle recursion up and down levels of json. This handles only single entries at a time. */
    bool FilterFieldname(const std::string& strField, const encoding::json& jResponse, encoding::json &jFiltered)
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
                return FilterFieldname(strAdjusted, jResponse[strNext], jFiltered[strNext]);
            }
        }

        /* Check for array to iterate. */
        if(jResponse.is_array())
        {
            /* Check for same size array to merge. */
            if(jFiltered.size() == jResponse.size())
            {
                /* Loop through all entries and aggregate objects. */
                for(uint32_t n = 0; n < jFiltered.size(); ++n)
                    FilterFieldname(strField, jResponse[n], jFiltered[n]);
            }

            /* Create new items if filtering new data-set. */
            else
            {
                /* Loop through all entries. */
                for(const auto& jField : jResponse)
                {
                    /* Add to our return array if passed filters. */
                    encoding::json jItem;
                    if(FilterFieldname(strField, jField, jItem))
                        jFiltered.push_back(jItem);

                }
            }

            /* Check that we found some fieldnames. */
            if(jFiltered.empty())
                return false;

            return true;
        }

        /* Check for object to evaluate. */
        if(jResponse.is_object())
        {
            /* Check for missing field. */
            if(jResponse.find(strField) == jResponse.end())
                return false;

            /* Build our single entry return value. */
            jFiltered[strField] = jResponse[strField];

            return true;
        }

        return false;
    }


    /* Main filter interface, handles json levels recursively handling multiple fields to filter. */
    bool FilterFieldname(const encoding::json& jParams, encoding::json &jResponse)
    {
        /* Check for fieldname filters. */
        if(jParams.find("fieldname") == jParams.end())
            return true;

        /* Handle if single string. */
        if(jParams["fieldname"].is_string())
        {
            /* Grab our field string to rebuild response. */
            const std::string strField = jParams["fieldname"].get<std::string>();

            /* Build our return value. */
            encoding::json jRet;
            if(!FilterFieldname(strField, jResponse, jRet))
                throw Exception(-119, "[", strField, "] field(s) does not exist for object");

            /* Set our return value. */
            jResponse = jRet;
            return true;
        }

        /* Handle if multiple fields. */
        if(jParams["fieldname"].is_array())
        {
            /* Track our aggregates. */
            std::vector<bool> vfActive(jParams["fieldname"].size(), false);

            /* Loop through our fields. */
            encoding::json jRet;
            for(uint32_t n = 0; n < jParams["fieldname"].size(); ++n)
            {
                /* Grab our field string to rebuild response. */
                const std::string strField = jParams["fieldname"][n].get<std::string>();

                /* Build our filtered statements. */
                encoding::json jFinal;
                vfActive[n] = FilterFieldname(strField, jResponse, jRet);

                /* Skip if no fields are added. */
                if(!vfActive[n])
                    continue;
            }

            /* Check that we found some fieldnames. */
            if(jRet.empty())
            {
                /* Grab a reference of our fields. */
                const encoding::json& jFields = jParams["fieldname"];

                /* Aggregate fields. */
                std::string strAggregate;
                for(uint32_t n = 0; n < jFields.size(); ++n)
                {
                    /* Check for failed fields for reporting. */
                    if(!vfActive[n])
                    {
                        /* Add our aggregate values. */
                        strAggregate += jFields[n].get<std::string>();

                        /* Add comma if not last. */
                        if(n < jFields.size() - 1)
                            strAggregate += ",";
                    }
                }

                throw Exception(-119, "[", strAggregate, "] field(s) does not exist for object");
            }


            /* Build our single entry return value. */
            jResponse = jRet;
            return true;
        }

        throw Exception(-36, "Invalid type [fieldname=", jParams["fieldname"].type_name(), "] for command");
    }


    /* Determines if an object or it's results should be included in list. */
    bool FilterObject(const encoding::json& jParams, encoding::json &jCheck, TAO::Register::Object &rCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        return FilterStatement<TAO::Register::Object>(jParams["where"], jCheck, rCheck, EvaluateObject);
    }


    /* Determines if an object should be included in a list based on input parameters. */
    bool FilterObject(const encoding::json& jParams, TAO::Register::Object &objCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        return FilterStatement<TAO::Register::Object>(jParams["where"], objCheck, EvaluateObject);
    }


    /* Determines if an object should be included in results list if they match parameters. */
    bool FilterResults(const encoding::json& jParams, encoding::json &jCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        return FilterStatement<encoding::json>(jParams["where"], jCheck, EvaluateResults);
    }
}
