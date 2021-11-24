/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/exception.h>

#include <Util/include/json.h>
#include <Util/include/version.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Operator
     *
     *  Base class for all JSON based API methods
     *  Encapsulates the function pointer to the method to process the API request
     *
     **/
    class Operator
    {
        /** The function pointer to be called. */
        std::function<encoding::json(const encoding::json&, const encoding::json&)> xFunction;


        /** The deprecation version. **/
        uint32_t nMaxVersion;


        /** The deprecation message and info. **/
        std::string strMessage;


    public:


        /** Default Constructor. **/
        Operator()
        : xFunction   ( )
        , nMaxVersion (0)
        , strMessage  ( )
        {
        }


        /** Constructor
         *
         *  Base constructor with no deprecation data for base operator function.
         *
         *  @param[in] tFunctionIn The function to be executed by this class.
         *
         **/
        Operator(const std::function<encoding::json(const encoding::json&, const encoding::json&)> xFunctionIn)
        : xFunction   (xFunctionIn)
        , nMaxVersion (0)
        , strMessage  ( )
        {
        }


        /** Constructor
         *
         *  Alternative constructor following value sequence of base constructor and no default parameters
         *
         *  @param[in] tFunctionIn The function to be executed by this class.
         *  @param[in] nActivationIn The activating timestamp if this method activates with hard fork.
         *  @param[in] nMaxVersionIn The maximum version this function can be called on.
         *  @param[in] strMessageIn The deprecation message including info for re-routes or alternative methods.
         *
         **/
        Operator(const std::function<encoding::json(const encoding::json&, const encoding::json&)> xFunctionIn,
                 const uint32_t nMaxVersionIn, const std::string& strMessageIn)
        : xFunction   (xFunctionIn)
        , nMaxVersion (nMaxVersionIn)
        , strMessage  (strMessageIn)
        {
        }


        /** Execute
         *
         *  Executes the function pointer.
         *
         *  @param[in] jParams The json formatted parameters
         *  @param[in] jList The list of objects to operate on.
         *
         *  @return The json formatted response.
         *
         **/
        __attribute__((const)) encoding::json Execute(const encoding::json& jParams, const encoding::json& jList)
        {
            /* Check for deprecation status. */
            if(nMaxVersion != 0 && version::CLIENT_VERSION >= nMaxVersion)
                throw Exception(-3, "Operator was deprecated at version ", version::version_string(nMaxVersion), ": ", strMessage);

            return xFunction(jParams, jList);
        }
    };
}
