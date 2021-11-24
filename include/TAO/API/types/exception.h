/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Exception
     *
     *  Encapsulates an exception that can be converted into a valid JSON error object
     *
     **/
    class Exception : public encoding::detail::exception
    {
    public:

        /** Default Constructor **/
        Exception(const int32_t nCode, const std::string& strMessage)
        : encoding::detail::exception(nCode, strMessage.c_str())
        {
        }


        /** Variadic template constructor for building error strings. **/
        template<class... Args>
        Exception(const int32_t nCode, Args&&... args)
        : encoding::detail::exception(nCode, debug::safe_printstr(args...).c_str())
        {
        }


        /** ToJSON
         *
         *  Converts this exception into a json object conforming to the JSON-RPC specification.
         *
         *  @return the json object containing the exception code and message.
         *
         **/
        encoding::json ToJSON() const
        {
            /* Include our global errors if applicable. */
            std::string strMessage = std::string(what());
            if(!debug::strLastError.empty())
                strMessage += ". " + debug::GetLastError();

            return { {"code", id }, {"message", strMessage} }; //no need to make another copy, so emplace our return value
        }
    };
}
