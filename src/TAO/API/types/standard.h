/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/include/json.h>
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
     *  Encapsulates the function pointer that verifies object's standard.
     *
     **/
    class Standard
    {
        /** The standard function to be called. */
        std::function<bool(const TAO::Register::Object&)> xFunction;


        /** The encoding function to be called. */
        std::function<encoding::json(const TAO::Register::Object&, const TAO::Register::Address&)> xEncoding;


        /** Register standard name for sequential reads. **/
        std::string strStandard;


        /** The activation timestamp. **/
        uint64_t nActivation;


        /** The deprecation version. **/
        uint32_t nMaxVersion;


    public:


        /** Default Constructor. **/
        Standard      ( )
        : xFunction   ( )
        , xEncoding   (std::bind(&RegisterToJSON, std::placeholders::_1, std::placeholders::_2))
        , strStandard ("")
        , nActivation (0)
        , nMaxVersion (0)
        {
        }


        /** Constructor
         *
         *  Base constructor that requires function but has activation and version as default disabled.
         *
         *  @param[in] tFunctionIn The function to be executed by this class.
         *  @param[in] strStandardIn The object standard to use for sequential reads
         *  @param[in] nActivationIn The activating timestamp if this method activates with hard fork.
         *  @param[in] nMaxVersionIn The maximum version this function can be called on.
         *
         **/
        Standard(const std::function<bool(const TAO::Register::Object&)> xFunctionIn, const std::string& strStandardIn = "object",
                 const uint64_t nActivationIn = 0, const uint32_t nMaxVersionIn = 0)
        : xFunction   (xFunctionIn)
        , xEncoding   (std::bind(&RegisterToJSON, std::placeholders::_1, std::placeholders::_2))
        , strStandard (strStandardIn)
        , nActivation (nActivationIn)
        , nMaxVersion (nMaxVersionIn)
        {
        }


        /** Constructor
         *
         *  Alternative constructor that requires function and encoding but has activation and version as default disabled.
         *
         *  @param[in] tFunctionIn The function to be executed by this class.
         *  @param[in] xEncodingIn The encoding function to use for given object.
         *  @param[in] strStandardIn The object standard to use for sequential reads
         *  @param[in] nActivationIn The activating timestamp if this method activates with hard fork.
         *  @param[in] nMaxVersionIn The maximum version this function can be called on.
         *
         **/
        Standard(const std::function<bool(const TAO::Register::Object&)> xFunctionIn,
                 const std::function<encoding::json(const TAO::Register::Object&, const TAO::Register::Address&)> xEncodingIn,
                 const std::string& strStandardIn = "object", const uint64_t nActivationIn = 0, const uint32_t nMaxVersionIn = 0)
        : xFunction   (xFunctionIn)
        , xEncoding   (xEncodingIn)
        , strStandard (strStandardIn)
        , nActivation (nActivationIn)
        , nMaxVersion (nMaxVersionIn)
        {
        }


        /** Type
         *
         *  Checks the type of given standard for sequential disk reads.
         *
         *  @return The string to use.
         *
         **/
        __attribute__((pure)) std::string Type() const
        {
            return strStandard;
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
        __attribute__((pure)) bool Check(const TAO::Register::Object& rObject) const
        {
            /* Check for activation status. */
            const uint64_t nTimestamp = runtime::unifiedtimestamp();
            if(nActivation != 0 && nTimestamp < nActivation)
                throw Exception(-5, "Object not available: activates in ", (nActivation - nTimestamp), " seconds");

            /* Check for deprecation status. */
            if(nMaxVersion != 0 && version::CLIENT_VERSION >= nMaxVersion)
                throw Exception(-5, "Object not available: deprecated at version ", version::version_string(nMaxVersion));

            return xFunction(rObject);
        }


        /** Encode
         *
         *  Encode this standard object into json using custom encoding function.
         *
         *  @param[in] object The object we are encoding for.
         *  @param[in] hashRegister The register's address we are encoding for.
         *
         *  @return the json encoded object
         *
         **/
        __attribute__((pure)) encoding::json Encode(const TAO::Register::Object& object, const uint256_t& hashRegister) const
        {
            return xEncoding(object, hashRegister);
        }
    };
}
