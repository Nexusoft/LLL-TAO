/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

namespace TAO::Register { class Object; }

namespace TAO::API
{
    /** Evaluate Wildcard
     *
     *  Checks a given string against value to find wildcard pattern matches.
     *
     *  @param[in] strWildcard The wildcard statement we are checking against.
     *  @param[in] strValue The value we are checking wildcard against.
     *
     *  @return true if wildcard pattern was found in value.
     *
     **/
    bool EvaluateWildcard(const std::string& strWildcard, const std::string& strValue);


    /** EvaluateResults
     *
     *  Determines if an results JSON object should be included in a list based on given clause.
     *
     *  @param[in] jClause The clause to check filter for.
     *  @param[out] jCheck The JSON object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool EvaluateResults(const encoding::json& jClause, encoding::json &jCheck);


    /** EvaluateObject
     *
     *  Determines if an object should be included in a list based on given clause.
     *
     *  @param[in] jClause The clause to check filter for.
     *  @param[out] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool EvaluateObject(const encoding::json& jClause, TAO::Register::Object &objCheck);

}
