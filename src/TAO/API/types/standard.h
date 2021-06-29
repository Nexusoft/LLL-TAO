/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/exception.h>

#include <TAO/Register/types/object.h>

#include <Util/include/version.h>

#include <functional>
#include <memory>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Standard
     *
     *  Base class for all standard objects per API commandset
     *  Encapsulates the function pointer to the method to process the API request
     *
     **/
    class Standard
    {
        /** The function pointer to be called. */
        std::function<bool(const TAO::Register::Object&)> tFunction;


        /** The activation timestamp. **/
        uint64_t nActivation;


        /** The deprecation version. **/
        uint32_t nMaxVersion;


    public:


        /** Default Constructor. **/
        Standard      ( )
        : tFunction   ( )
        , nActivation (0)
        , nMaxVersion (0)
        {
        }


        /** Constructor
         *
         *  Alternative constructor following value sequence of base constructor and no default parameters
         *
         *  @param[in] tFunctionIn The function to be executed by this class.
         *  @param[in] nActivationIn The activating timestamp if this method activates with hard fork.
         *  @param[in] nMaxVersionIn The maximum version this function can be called on.
         *
         **/
        Standard(const std::function<bool(const TAO::Register::Object&)> tFunctionIn,
                 const uint64_t nActivationIn = 0, const uint32_t nMaxVersionIn = 0)
        : tFunction   (tFunctionIn)
        , nActivation (nActivationIn)
        , nMaxVersion (nMaxVersionIn)
        {
        }


        /** Check
         *
         *  Evaluates the function against given standard object.
         *
         *  @param[in] jParams The json formatted parameters
         *
         *  @return The json formatted response.
         *
         **/
        __attribute__((pure)) bool Check(const TAO::Register::Object& objCheck) const
        {
            /* Check for activation status. */
            const uint64_t nTimestamp = runtime::unifiedtimestamp();
            if(nActivation != 0 && nTimestamp < nActivation)
                throw Exception(-5, "Object not available: activates in ", (nActivation - nTimestamp), " seconds");

            /* Check for deprecation status. */
            if(nMaxVersion != 0 && version::CLIENT_VERSION >= nMaxVersion)
                throw Exception(-5, "Object not available: deprecated at version ", version::version_string(nMaxVersion));

            return tFunction(objCheck);
        }
    };
}
