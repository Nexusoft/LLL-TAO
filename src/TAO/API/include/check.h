/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <Util/include/json.h>

#include <string.h>

/* Forward declarations */
namespace TAO::Operation { class Contract; }
namespace TAO::Register  { class Object;   }


/* Global TAO namespace. */
namespace TAO::API
{

    /** CheckAddress
     *
     *  Determines whether a Base58 encoded value is a valid register address by checking type.
     *
     *  @param[in] strValue The value to check
     *
     *  @return True if the value can be decoded into a valid Register::Address with a valid Type.
     **/
    bool CheckAddress(const std::string& strValue);


    /** CheckParameter
     *
     *  Determines if given parameter is what is correct expected type. Will throw exception if invalid.
     *
     *  @param[in] jParam The parameter to check
     *  @param[in] strKey The key of parameter we are checking.
     *  @param[in] strType The allowed types, seperated by any delimiter like a comma
     *
     *  @return true if the parameter is valid and ready for extracting.
     *
     **/
    bool CheckParameter(const encoding::json& jParam, const std::string& strKey, const std::string& strType = "");


    /** CheckMature
     *
     *  Utilty method that checks that the signature chain is mature and can therefore create new transactions.
     *  Throws an appropriate Exception if it is not mature
     *
     *  @param[in] hashGenesis The genesis hash of the signature chain to check
     *
     **/
    void CheckMature(const uint256_t& hashGenesis);


    /** CheckContract
     *
     *  Checks if a contract will execute correctly once built.
     *
     *  @param[in] rContract The contract to check and sanitize.
     *
     *  @return true if the contract executed correctly.
     *
     **/
    bool CheckContract(const TAO::Operation::Contract& rContract);


    /** CheckStandard
     *
     *  Checks if the designated object matches the explicet type specified in parameters.
     *  We have no return value since this command is meant to throw on errors for API calls.
     *
     *  @param[in] jParams The json parameters to check against.
     *  @param[in] hashCheck The register that we are checking against.
     *
     *  @return True if the object type is what was specified.
     *
     **/
    bool CheckStandard(const encoding::json& jParams, const uint256_t& hashCheck);


    /** CheckStandard
     *
     *  Checks if the designated object matches the explicet type specified in parameters.
     *  Doesn't do a register database lookup like prior overload does.
     *
     *  @param[in] jParams The json parameters to check against.
     *  @param[in] rObject The object that we are checking for.
     *
     *  @return True if the object type is what was specified.
     *
     **/
    bool CheckStandard(const encoding::json& jParams, const TAO::Register::Object& rObject);


}
