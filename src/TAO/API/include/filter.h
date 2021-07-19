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
    /** FilterStatement
     *
     *  Recursive handle for any type of filter by passing in function and type to filter.
     *  Can process any depth of logical recursion required.
     *
     *  @param[in] jStatement The statement to filter by, in JSON encoding.
     *  @param[in] rCheck The object that we are checking on.
     *  @param[in] rFunct The function that executes the final clause filter.
     *
     *  @return true if the object passes filter checks, false if it shouldn't be included.
     *
     **/
    template<typename ObjectType>
    bool FilterStatement(const encoding::json& jStatement, ObjectType& rCheck,
                         const std::function<bool (const encoding::json&, ObjectType&)>& rFunct);


     /** FilterStatement
      *
      *  Recursive handle for any type of filter by passing in function and type to filter.
      *  Can process any depth of logical recursion required.
      *
      *  @param[in] jStatement The statement to filter by, in JSON encoding.
      *  @param[in] rCheck The object that we are checking on.
      *  @param[in] rFunct The function that executes the final clause filter.
      *
      *  @return true if the object passes filter checks, false if it shouldn't be included.
      *
      **/
     template<typename ObjectType>
     bool FilterStatement(const encoding::json& jStatement, encoding::json &jCheck, ObjectType& rCheck,
                          const std::function<bool (const encoding::json&, ObjectType&)>& xObject);


    /** FilterFieldname
     *
     *  If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field
     *
     *  @param[in] params The parameters passed into the request
     *  @param[out] response The reponse JSON to be filtered.
     *
     *  @return true if a field was found and filtered.
     *
     **/
    bool FilterFieldname(const encoding::json& jParams, encoding::json &jResponse);


    /** FilterObject
     *
     *  Determines if an object or it's results should be included in list.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[out] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterObject(const encoding::json& jParams, encoding::json &jCheck, TAO::Register::Object &rCheck);


    /** FilterObject
     *
     *  Determines if an object should be included in a list based on input parameters.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[out] objCheck The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterObject(const encoding::json& jParams, TAO::Register::Object &objCheck);


    /** FilterResults
     *
     *  Determines if an object should be included in results list if they match parameters.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[out] jCheck The json object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterResults(const encoding::json& jParams, encoding::json &jCheck);

}
