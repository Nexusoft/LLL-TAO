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
    uint256_t ExtractAddress(const encoding::json& jParams, const std::string& strSuffix = "", const std::string& strDefault = "");


    /** ExtractToken
     *
     *  Extract a token address from incoming parameters to derive from name or address field.
     *
     *  @param[in] jParams The parameters to find address in.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractToken(const encoding::json& jParams);


    /** ExtractGenesis
     *
     *  Extract a genesis-id from input parameters which could be either username or genesis keys.
     *
     *  @param[in] jParams The parameters to find address in.
     *
     *  @return The genesis-id that was extracted.
     *
     **/
    uint256_t ExtractGenesis(const encoding::json& jParams);


    /** ExtractRecipient
     *
     *  Extract a recipient genesis-id from input parameters which could be either username or genesis keys.
     *
     *  @param[in] jParams The parameters to find address in.
     *
     *  @return The genesis-id that was extracted.
     *
     **/
    uint256_t ExtractRecipient(const encoding::json& jParams);


    /** ExtractAmount
     *
     *  Extract an amount value from either string or integer and convert to its final value.
     *
     *  @param[in] jParams The parameters to extract amount from.
     *  @param[in] nFigures The figures calculated from decimals.
     *  @param[in] strPrefix A string prefix for prepending to amount field.
     *
     *  @return The amount represented as whole integer value.
     *
     **/
    uint64_t ExtractAmount(const encoding::json& jParams, const uint64_t nFigures, const std::string& strPrefix = "");


    /** ExtractValue
     *
     *  Extract an integer value from input parameters in either string or integer format.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strName The name of argument to extract.
     *
     *  @return the integer representation of verbose argument.
     *
     **/
    uint64_t ExtractValue(const encoding::json& jParams, const std::string& strName);



    /** ExtractVerbose
     *
     *  Extract a verbose argument from input parameters in either string or integer format.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] nMinimum The minimum value that can be used.
     *
     *  @return the integer representation of verbose argument.
     *
     **/
    uint32_t ExtractVerbose(const encoding::json& jParams, const uint32_t nMinimum = 1);


    /** ExtractList
     *
     *  Extracts the paramers applicable to a List API call in order to apply a filter/offset/limit to the result
     *
     *  @param[in] jParams The parameters passed into the request
     *  @param[out] strOrder The sort order to apply
     *  @param[out] nLimit The number of results to return
     *  @param[out] nOffset The offset to apply to the results
     *
     **/
    void ExtractList(const encoding::json& jParams, std::string &strOrder, uint32_t &nLimit, uint32_t &nOffset);


    /** ExtractList
     *
     *  Extracts the paramers applicable to a List API call in order to apply a filter/offset/limit to the result
     *  This overload includes a sort field.
     *
     *  @param[in] jParams The parameters passed into the request
     *  @param[out] strOrder The sort order to apply
     *  @param[out] strSort The column to sort by.
     *  @param[out] nLimit The number of results to return
     *  @param[out] nOffset The offset to apply to the results
     *
     **/
    void ExtractList(const encoding::json& jParams, std::string &strOrder, std::string &strSort, uint32_t &nLimit, uint32_t &nOffset);


    /** ExtractInteger
     *
     *  Extracts an integer value from given input parameters.
     *
     *  @param[in] jParams The parameters that contain requesting value.
     *  @param[in] strKey The key that we are checking parameters for.
     *  @param[in] fRequired Determines if given parameter is required.
     *  @param[in] nLimit The numeric limits to bound this type to, that may be less than type's value.
     *
     *  @return The extracted integer value.
     *
     **/
    template<typename Type>
    Type ExtractInteger(const encoding::json& jParams, const char* strKey,
                        const bool fRequired = true, const Type nLimit = std::numeric_limits<Type>::max())
    {
        /* Build our return value. */
        Type nRet = 0;

        /* Check for missing parameter. */
        if(jParams.find(strKey) != jParams.end())
        {
            /* Catch parsing exceptions. */
            try
            {
                /* Convert to value if in string form. */
                if(jParams[strKey].is_string())
                    nRet = std::stoull(jParams[strKey].get<std::string>());

                /* Grab value regularly if it is integer. */
                else if(jParams[strKey].is_number_integer())
                    nRet = jParams[strKey].get<Type>();

                /* Otherwise we have an invalid parameter. */
                else
                    throw Exception(-57, "Invalid Parameter [", strKey, "]");

                /* Check our numeric limits now. */
                if(nRet > nLimit)
                    throw Exception(-60, "[", strKey, "] out of range [", nLimit, "]");

                return nRet;
            }
            catch(const encoding::detail::exception& e) { throw Exception(-57, "Invalid Parameter [", strKey, "]");           }
            catch(const std::invalid_argument& e)       { throw Exception(-57, "Invalid Parameter [", strKey, "]");           }
            catch(const std::out_of_range& e)           { throw Exception(-60, "[", strKey, "] out of range [", nLimit, "]"); }
        }

        /* Return value if we aren't throwing missing. */
        if(!fRequired)
            return nRet;

        throw Exception(-56, "Missing Parameter [", strKey, "]");
    }

}
