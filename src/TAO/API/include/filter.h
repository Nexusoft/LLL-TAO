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
    /** ObjectClause
     *
     *  Determines if an object should be included in a list based on given clause.
     *
     *  @param[in] jClause The clause to check filter for.
     *  @param[in] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool ObjectClause(const encoding::json& jClause, const TAO::Register::Object& objCheck);


    /** FilterObject
     *
     *  Determines if an object should be included in a list based on input parameters.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[in] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterObject(const encoding::json& jParams, const TAO::Register::Object& objCheck);


    /** FilterResponse
     *
     *  If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field
     *
     *  @param[in] params The parameters passed into the request
     *  @param[out] response The reponse JSON to be filtered.
     *
     **/
    void FilterResponse(const encoding::json& jParams, encoding::json &jResponse);

}
