/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <Util/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** ExtractAddress
     *
     *  Extract an address from incoming parameters to derive from name or address field.
     *
     *  @param[in] jParams The parameters to find address in.
     *  @param[in] strSuffix The suffix to append to end of parameter we are extracting.
     *  @param[in] strDefault The default value to revert to if failed to find in parameters.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractAddress(const json::json& jParams, const std::string& strSuffix = "", const std::string& strDefault = "");


    /** ExtractToken
     *
     *  Extract a token address from incoming parameters to derive from name or address field.
     *
     *  @param[in] jParams The parameters to find address in.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractToken(const json::json& jParams);



    /** ExtractParams
     *
     *  Extracts the paramers applicable to a List API call in order to apply a filter/offset/limit to the result
     *
     *  @param[in] params The parameters passed into the request
     *  @param[out] strOrder The sort order to apply
     *  @param[out] nLimit The number of results to return
     *  @param[out] nOffset The offset to apply to the results
     *
     **/
    void ExtractParams(const json::json& params, std::string &strOrder, uint32_t &nLimit, uint32_t &nOffset);

}
