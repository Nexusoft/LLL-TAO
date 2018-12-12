/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

namespace TAO::API
{

    /** APIException class
    *
    *  Encapsulates an exception that can be converted into a valid JSON error object
    **/
    class APIException : public json::detail::exception
    {
    public:
        APIException(int nCode, const char* strMessage)
        : json::detail::exception(nCode, strMessage) {}

        APIException(int nCode, const std::string& strMessage)
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
            jsonError["message"] = std::string(what());
            return jsonError;
        }

    };
}