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
#include <memory>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Function
     *
     *  Base class for all JSON based API methods
     *  Encapsulates the function pointer to the method to process the API request
     *
     **/
    class Function
    {
        /** The function pointer to be called. */
        std::function<encoding::json(const encoding::json&, bool)> tFunction;


        /** The activation timestamp. **/
        uint64_t nActivation;


        /** The deprecation version. **/
        uint32_t nMaxVersion;


        /** The deprecation message and info. **/
        std::string strMessage;


    public:


        /** Default Constructor. **/
        Function()
        : tFunction   ( )
        , nActivation (0)
        , nMaxVersion (0)
        , strMessage  ( )
        {
        }


        /** Base Constructor **/
        Function(const std::function<encoding::json(const encoding::json&, bool)> tFunctionIn, const uint64_t nActivationIn = 0)
        : tFunction   (tFunctionIn)
        , nActivation (nActivationIn) //default: zero denotes there is no activation switch
        , nMaxVersion (0)
        , strMessage  ( )
        {
        }


        /** Constructor for Deprecation. **/
        Function(const std::function<encoding::json(const encoding::json&, bool)> tFunctionIn,
                 const uint32_t nMaxVersionIn, const std::string& strMessageIn, const uint64_t nActivationIn = 0)
        : tFunction   (tFunctionIn)
        , nActivation (nActivationIn)
        , nMaxVersion (nMaxVersionIn)
        , strMessage  (strMessageIn)
        {
        }


        /** Execute
         *
         *  Executes the function pointer.
         *
         *  @param[in] jParams The json formatted parameters
         *  @param[in] fHelp Flag if help is invoked
         *
         *  @return The json formatted response.
         *
         **/
        encoding::json Execute(const encoding::json& jParams, const bool fHelp)
        {
            /* Check for activation status. */
            const uint64_t nTimestamp = runtime::unifiedtimestamp();
            if(nActivation > 0 && nTimestamp < nActivation)
                throw APIException(-1, "Method not available: activates in ", (nActivation - nTimestamp), " seconds");

            /* Check for deprecation status. */
            if(version::CLIENT_VERSION >= nMaxVersion)
                throw APIException(-1, "Method not available: ", strMessage);

            return tFunction(jParams, fHelp);
        }


        /** Status
         *
         *  Get status message for current function.
         *
         **/
        __attribute__((pure)) std::string Status() const
        {
            /* Check for activation status. */
            if(nActivation > 0 && runtime::unifiedtimestamp() < nActivation)
                return "inactive";

            /* Check for deprecation status messages ahead by one minor version increment. */
            if(nMaxVersion > 0)
            {
                /* Give deprecation message if in deprecation period. */
                if(version::CLIENT_VERSION < nMaxVersion)
                    return debug::safe_printstr("Deprecated at version ", version::version_string(nMaxVersion), ": ", strMessage);

                return "deprecated";
            }

            return "active";
        }
    };
}
