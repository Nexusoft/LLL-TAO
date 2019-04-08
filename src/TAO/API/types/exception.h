/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** APIException
         *
         *  Encapsulates an exception that can be converted into a valid JSON error object
         *
         **/
        class APIException : public json::detail::exception
        {
        public:

            /** Default Constructor **/
            APIException(int32_t nCode, const char* strMessage)
            : json::detail::exception(nCode, strMessage) {}


            /** Default Constructor **/
            APIException(int32_t nCode, const std::string& strMessage)
            : json::detail::exception(nCode, strMessage.c_str()) {}


            /** ToJSON
             *
             *  Converts this exception into a json object conforming to the JSON-RPC specification.
             *
             *  @return the json object containing the exception code and message.
             *
             **/
            json::json ToJSON()
            {
                json::json jsonError;
                jsonError["code"] = id;

                std::string strMessage = std::string(what()); 

                /* If a global error message has been logged via debug::error then include this in the JSON*/
                if( !debug::strLastError.empty())
                    strMessage += ". " + debug::strLastError;

                jsonError["message"] = strMessage; 

                return jsonError;
            }

        };
    }
}
