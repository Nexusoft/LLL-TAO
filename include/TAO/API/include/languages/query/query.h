/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

/* Forward declarations. */
namespace TAO::Register  { class Object; }

/* Global TAO namespace. */
namespace TAO::API
{

    /** StatementToJSON
     *
     *  Recursive helper function for QueryToJSON to recursively generate JSON statements for use with filters.
     *
     *  @param[out] vWhere The statements to parse into query.
     *  @param[out] nIndex The current statement being processed.
     *  @param[out] jStatement The statement reference to pass through recursion.
     *
     *  @return The JSON statement generated with query.
     *
     **/
    __attribute__((pure)) encoding::json StatementToJSON(std::vector<std::string> &vWhere, uint32_t &nIndex, encoding::json &jStatement);


    /** QueryToJSON
     *
     *  Turns a where query string in url encoding into a formatted JSON object.
     *
     *  @param[in] strWhere The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json QueryToJSON(const std::string& strWhere);


    /** ClauseToJSON
     *
     *  Turns a where clause string in url encoding into a formatted JSON object.
     *
     *  @param[in] strClause The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json ClauseToJSON(const std::string& strClause);


    /** ParamsToJSON
     *
     *  Turns parameters from url encoding into a formatted JSON object.
     *
     *  @param[in] vParams The string to parse and generate into json.
     *
     *  @return The JSON object generated with query.
     *
     **/
    __attribute__((pure)) encoding::json ParamsToJSON(const std::vector<std::string>& vParams);
}
