/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/json.h>
#include <TAO/API/include/get.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/types/address.h>
#include <Legacy/types/trustkey.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/format.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>
#include <TAO/Register/include/reserved.h>

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>
#include <Util/include/math.h>

#include <Util/encoding/include/utf-8.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /* Recursive helper function for QueryToJSON to recursively generate JSON statements for use with filters. */
    encoding::json StatementToJSON(std::vector<std::string> &vWhere, uint32_t &nIndex, encoding::json &jStatement)
    {
        /* Check if we have consumed all of our clauses. */
        if(nIndex >= vWhere.size())
            return jStatement;

        /* Grab a reference of our working string. */
        std::string& strClause = vWhere[nIndex];

        /* Check for logical statement. */
        if(strClause == "AND" || strClause == "OR")
        {
            /* Check for incorrect mixing of AND/OR. */
            if(jStatement.find("logical") == jStatement.end())
                throw Exception(-122, "Query Syntax Error: missing logical operator for group");

            /* Grab a copy of our current logical statement. */
            const std::string strLogical = jStatement["logical"].get<std::string>();
            if(strLogical != "NONE" && strLogical != strClause)
                throw Exception(-121, "Query Syntax Error, must use '(' and ')' to mix AND/OR statements");

            jStatement["logical"] = strClause;
            return StatementToJSON(vWhere, ++nIndex, jStatement);
        }

        /* Check if we have extra spaces in the statement. */
        while(nIndex + 1 < vWhere.size())
        {
            /* Grab a reference of our current statement in query. */
            const std::string& strCheck = vWhere[nIndex + 1];

            /* Check if we can exit now with complete statement. */
            if(strCheck == "AND" || strCheck == "OR")
                break;

            strClause += (std::string(" ") + strCheck);
            vWhere.erase(vWhere.begin() + nIndex + 1); //to clear up iterations for next statement, to ensure no re-use of data
        }

        /* Check if we are recursing up a level. */
        const auto nLeft = strClause.find("(");
        if(nLeft == 0)
        {
            /* Parse out substring removing paranthesis. */
            strClause = strClause.substr(nLeft + 1);

            /* Create a new group to recurse up a level. */
            encoding::json jGroup
            {
                { "logical", "NONE" },
                { "statement", encoding::json::array() },
            };

            /* We want to push this new group recursive field to current level. */
            jStatement["statement"].push_back(StatementToJSON(vWhere, nIndex, jGroup));

            /* Now we continue consuming our clauses to complete the statement. */
            return StatementToJSON(vWhere, ++nIndex, jStatement);
        }

        /* Check if we are recursing back down a level. */
        const auto nRight = strClause.rfind(")");
        if(nRight == strClause.length() - 1)
        {
            /* Parse out substring removing paranthesis. */
            strClause = strClause.substr(0, nRight);

            /* Check if we need to recurse another level still. */
            if(strClause.rfind(")") == strClause.length() - 1)
                return StatementToJSON(vWhere, nIndex, jStatement);

            /* Add our clause to end of statement. */
            jStatement["statement"].push_back(ClauseToJSON(strClause));

            return jStatement;
        }

        /* Regular statement adding clause. */
        jStatement["statement"].push_back(ClauseToJSON(strClause));

        /* Regular recursion to move to next statement. */
        return StatementToJSON(vWhere, ++nIndex, jStatement);
    }


    /* Turns a where query string in url encoding into a formatted JSON object. */
    encoding::json QueryToJSON(const std::string& strWhere)
    {
        /* Split up our query by spaces. */
        std::vector<std::string> vWhere;
        ParseString(strWhere, ' ', vWhere);

        /* Build our return object. */
        encoding::json jRet
        {
            { "logical", "NONE" },
            { "statement", encoding::json::array() },
        };

        /* Recursively process the query. */
        uint32_t n = 0;
        jRet = StatementToJSON(vWhere, n, jRet);

        /* Check for logical operator. */
        //if(jRet["logical"].get<std::string>() == "NONE" && jRet["statement"].size() > 1)
        //    throw Exception(-120, "Query Syntax Error: missing logical operator for base group, too many '()' maybe?");

        return jRet;
    }


    /* Turns a where clause string in url encoding into a formatted JSON object. */
    encoding::json ClauseToJSON(const std::string& strClause)
    {
        /* Check for a set to compare. */
        const auto nBegin = strClause.find_first_of("!=<>");
        if(nBegin == strClause.npos)
            throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters. ", strClause);

        /* Grab our current key. */
        const std::string strKey = strClause.substr(0, nBegin);

        /* Check for our incoming parameter. */
        const std::string::size_type nDot = strKey.find('.');
        if(nDot == strKey.npos)
            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strKey);

        /* Build our current json value. */
        encoding::json jClause =
        {
            { "class", strKey.substr(0, nDot)  },
            { "field", strKey.substr(nDot + 1) },
        };

        /* Check if its < or =. */
        const auto nEnd = strClause.find_first_of("!=<>", nBegin + 1);
        if(nEnd != strClause.npos)
        {
            jClause["operator"] = strClause[nBegin] + std::string("") + strClause[nEnd];
            jClause["value"]    = strClause.substr(nBegin + 2);
        }

        /* It's just < if we reach here. */
        else
        {
            jClause["operator"] = strClause[nBegin] + std::string("");
            jClause["value"]    = strClause.substr(nBegin + 1);
        }

        /* Check the clause for any variables. */
        const std::string strValue = jClause["value"].get<std::string>();
        if(strValue.find(';') != strValue.npos)
            jClause["value"] = VariableToJSON(strValue);

        /* Check for valid values and parameters. */
        if(jClause["value"].get<std::string>().empty())
            throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters.");

        return jClause;
    }


    /* Turns a query string in url encoding into a formatted JSON object. */
    encoding::json ParamsToJSON(const std::vector<std::string>& vParams)
    {
        /* Track our where clause. */
        std::string strWhere;

        /* Get the parameters. */
        encoding::json jRet;
        for(uint32_t n = 0; n < vParams.size(); ++n)
        {
            /* Set et as reference of our parameter. */
            const std::string& strParam = vParams[n];

            /* Find the key/value delimiter. */
            const auto nPos = strParam.find("=");
            if(nPos != strParam.npos || strParam == "WHERE")
            {
                /* Check for key/value pairs. */
                std::string strKey, strValue;
                if(nPos != strParam.npos)
                {
                    /* Grab the key and value substrings. */
                    strKey   = strParam.substr(0, nPos);
                    strValue = strParam.substr(nPos + 1);

                    /* Check for valid parameter. */
                    while(n < vParams.size() - 1 && vParams[n + 1].find("=") == vParams[n + 1].npos)
                        strValue += " " + vParams[++n];

                    /* Check for empty value, due to ' ' or bad input. */
                    if(strValue.empty())
                        throw Exception(-58, "Empty Parameter [", strKey, "]");
                }

                /* Check for where clauses. */
                if(strParam == "WHERE" || strKey == "where")
                {
                    /* Check if we have operated before. */
                    if(!strWhere.empty()) //check for double WHERE
                        throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                    /* If where as key/value, append the value we parsed out. */
                    if(strKey == "where")
                    {
                        /* Check that our where clause is a proper conditional statement. */
                        if(strValue.find_first_of("!=<>") == strValue.npos)
                            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                        strWhere += std::string(strValue);
                    }

                    /* Check if we have empty WHERE. */
                    if(n + 1 >= vParams.size() && strParam == "WHERE")
                        throw Exception(7, "Query Syntax Error: empty WHERE clause at [", strValue, "]");

                    /* Build a single where string for parsing. */
                    for(uint32_t i = n + 1; i < vParams.size(); ++i)
                    {
                        /* Check if we have operated before. */
                        if(vParams[i] == "WHERE" && strKey == "where") //check for double WHERE
                            throw Exception(-60, "Query Syntax Error: malformed where clause at ", strValue);

                        /* Append the string with remaining arguments. */
                        strWhere += vParams[i];

                        /* Add a space as delimiter. */
                        if(i < vParams.size() - 1)
                            strWhere += std::string(" ");
                    }

                    break;
                }

                /* Add to our return json. */
                else
                {
                    /* Parse if the parameter is a json object or array. */
                    if(encoding::json::accept(strValue))
                    {
                        jRet[strKey] = encoding::json::parse(strValue);
                        continue;
                    }

                    /* Otherwise just copy string argument over. */
                    jRet[strKey] = strValue;
                }
            }
            else
                throw Exception(-120, "Query Syntax Error: must use <key>=<value> with no extra characters.");
        }

        /* Build our query string now. */
        if(!strWhere.empty())
            jRet["where"] = strWhere;

        return jRet;
    }
}
