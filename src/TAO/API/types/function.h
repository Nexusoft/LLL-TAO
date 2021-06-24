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


        /** The activation version. **/
        uint32_t nMinVersion;


        /** The deprecation version. **/
        uint32_t nMaxVersion;


        /** The deprecation message and info. **/
        std::string strMessage;


    public:


        /** Default Constructor. **/
        Function()
        : tFunction   ( )
        , nMinVersion (0)
        , nMaxVersion (0)
        , strMessage  ( )
        {
        }


        /** Base Constructor **/
        Function(const std::function<encoding::json(const encoding::json&, bool)> tFunctionIn)
        : tFunction(tFunctionIn)
        , nMinVersion (version::get_version(3, 0)) //default: active since tritium launch
        , nMaxVersion (version::CLIENT_VERSION)
        , strMessage  ( )
        {
        }


        /** Constructor for Deprecation. **/
        Function(const std::function<encoding::json(const encoding::json&, bool)> tFunctionIn,
                 const uint32_t nMinVersionIn, const uint32_t nMaxVersionIn, const std::string& strMessageIn)
        : tFunction   (tFunctionIn)
        , nMinVersion (nMinVersionIn)
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
            /* Check for deprecation status. */
            if(version::CLIENT_VERSION < nMinVersion || version::CLIENT_VERSION >= nMaxVersion)
                throw APIException(-1, "Method not available: ", strMessage);

            return tFunction(jParams, fHelp);
        }


        /** Status
         *
         *  Get status message for current function.
         *
         **/
        std::string Status() const
        {
            /* Check for deprecation status messages ahead by one minor version increment. */
            if(version::CLIENT_VERSION + 100 >= nMaxVersion) //+100 is one minor version increment
                return debug::safe_printstr("WARNING: deprecated at version ", nMaxVersion, " (", strMessage, ")");

            return "active";
        }
    };
}
