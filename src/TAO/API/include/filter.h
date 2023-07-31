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
     *  Helper filter, to handle recursion up and down levels of json. This handles only single entries at a time.
     *
     *  @param[in] strField The fieldname that we are filtering for.
     *  @param[in] jResponse The response data that we are filtering for.
     *  @param[out] jFiltered The final filtered data returned by reference.
     *
     *  @return true if a field was found and filtered.
     *
     **/
    bool FilterFieldname(const std::string& strField, const encoding::json& jResponse, encoding::json &jFiltered);


    /** FilterFieldname
     *
     *  Main filter interface, handles json levels recursively handling multiple fields to filter.
     *
     *  @param[in] jParams The parameters passed into the request
     *  @param[out] jResponse The response JSON to be filtered.
     *
     *  @return true if a field was found and filtered or if no filters present.
     *
     **/
    bool FilterFieldname(const encoding::json& jParams, encoding::json &jResponse);


    /** FilterObject
     *
     *  Determines if an object should be included in a list based on input parameters.
     *
     *  @param[in] jParams The input parameters for the command.
     *  @param[out] rObject The object we are checking for.
     *
     *  @return true if the object should be included in the results.
     *
     **/
    bool FilterObject(const encoding::json& jParams, TAO::Register::Object &rObject);


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
