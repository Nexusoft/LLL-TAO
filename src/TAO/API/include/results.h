/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /** ResultsToArray
     *
     *  Convert a given json results queue into a compiled array of only values.
     *
     *  @param[in] jParams The parameters that contain fieldname we are converting for.
     *  @param[in] jResponse The object we are building array from.
     *  @param[out] jArray The array we are building.
     *
     **/
    void ResultsToArray(const encoding::json& jParams, const encoding::json& jResponse, encoding::json &jArray);


    /** ResultsToArray
     *
     *  Convert a given json results queue into a compiled array of only values.
     *
     *  @param[in] strField The fieldname we are converting for.
     *  @param[in] jResponse The object we are building array from.
     *  @param[out] jArray The array we are building.
     *
     *  @return true if converted fields sucessfully.
     *
     **/
    bool ResultsToArray(const std::string& strField, const encoding::json& jResponse, encoding::json &jArray);


}
