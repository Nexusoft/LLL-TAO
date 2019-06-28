
/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <Util/include/json.h>
#include <LLC/types/uint1024.h>

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
json::json APICall(const std::string& strMethod, const json::json& jsonParams);


/** InitializeUser
*
*  Creates, logs in, and unlocks the specified user
*
*  @param[in] strUsername The username
*  @param[in] strPassword The password
*  @param[in] strPin The ping
*  @param[out] hashGenesis The sig chain genesis hash
*  @param[out] strSession The session ID for this user's session
*
**/
void InitializeUser(const std::string& strUsername, const std::string& strPassword, const std::string& strPin,
                           uint256_t& hashGenesis, std::string& strSession);


/** LogoutUser
*
*  Logs out the specified session
*
*  @param[in/out] hashGenesis The sig chain genesis hash
*  @param[in/out] strSession The session ID for this user's session
*
**/
void LogoutUser(uint256_t& hashGenesis, std::string& strSession);


/* global session value used for all API calls */
extern std::string USERNAME1;
extern std::string USERNAME2;
extern uint256_t GENESIS1;
extern uint256_t GENESIS2;
extern std::string SESSION1;
extern std::string SESSION2;
extern std::string PASSWORD;
extern std::string PIN;