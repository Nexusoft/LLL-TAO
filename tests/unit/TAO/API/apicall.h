
/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <Util/include/json.h>

/** APICall
*
*  Makes an HTTP request to the API
*
*  @param[in] strMethod The API method (endpoint) to call
*  @param[in] strParams JSON paramters to pass to API call
*
*  @return JSON returned by the API
*
**/
json::json APICall(std::string strMethod, json::json jsonParams);


/* global session value used for all API calls */
extern std::string SESSION;