/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#pragma once

#include <TAO/API/types/exception.h>

#include <Util/include/allocators.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>

#include <set>

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


    /** ExtractAddress
     *
     *  Extract an address from a single string.
     *
     *  @param[in] jParams The parameters to find address in.
     *  @param[in] strAddress The parameters to find address in.
     *
     *  @return The register address if valid.
     *
     **/
    uint256_t ExtractAddress(const std::string& strAddress, const encoding::json& jParams);


    /** ExtractMarket
     *
     *  Extract a market identification from incoming parameters
     *
     *  @param[in] jParams The parameters to find address in.
     *
     *  @return The market trading pari
     *
     **/
    std::pair<uint256_t, uint256_t> ExtractMarket(const encoding::json& jParams);


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


    /** ExtractHash
     *
     *  Extracts a hash value for reading a txid.
     *
     *  @param[in] jParams
     *
     *  @return the extracted txid.
     *
     **/
    uint512_t ExtractHash(const encoding::json& jParams);


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
    uint64_t ExtractAmount(const encoding::json& jParams, const uint64_t nFigures, const std::string& strKey = "");


    /** ExtractFieldname
     *
     *  Extract the fieldname string value from input parameters as only string.
     *
     *  @param[in] jParams The input parameters to extract from.
     *
     *  @return a string list of types submitted to API
     *
     **/
    std::string ExtractFieldname(const encoding::json& jParams);


    /** ExtractFormat
     *
     *  Extract format specifier from input parameters.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strDefault The default format if no format specified.
     *  @param[in] strAllowed The allowed formates to extract for.
     *
     *  @return string with format specifier.
     *
     **/
    std::string ExtractFormat(const encoding::json& jParams, const std::string& strDefault, const std::string& strAllowed = "json");


    /** ExtractScheme
     *
     *  Extract key scheme specifier from input parameters.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strAllowed The allowed formates to extract for.
     *
     *  @return the scheme translated as binary represented enum.
     *
     **/
    uint8_t ExtractScheme(const encoding::json& jParams, const std::string& strAllowed = "falcon, brainpool");


    /** ExtractType
     *
     *  Extract the type string value from input parameters as only string.
     *
     *  @param[in] jParams The input parameters to extract from.
     *
     *  @return a string list of types submitted to API
     *
     **/
    std::string ExtractType(const encoding::json& jParams);


    /** ExtractPIN
     *
     *  Extract the pin number from input parameters.
     *
     *  @param[in] jParams The incoming parameters to check.
     *
     *  @return secure string representation of pin.
     *
     **/
    SecureString ExtractPIN(const encoding::json& jParams);


    /** ExtractTypes
     *
     *  Extract the type string value from input parameters as either array or string.
     *
     *  @param[in] jParams The input parameters to extract from.
     *
     *  @return a string set of types submitted to API
     *
     **/
    std::set<std::string> ExtractTypes(const encoding::json& jParams);



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


    /** ExtractBoolean
     *
     *  Extract a boolean value from input parameters.
     *
     *  @param[in] jParams The input parameters passed into request
     *  @param[in] strKey The parameter key we are extracting for.
     *  @param[in] fDefault Default parameter if none found.
     *
     *  @return true or false based on given value.
     *
     **/
    bool ExtractBoolean(const encoding::json& jParams, const std::string& strKey, const bool fDefault = false);


    /** ExtractHash
     *
     *  Extracts a hash value by template deduction.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strKey The key value we are extracting for.
     *
     *  @return the converted object from string constructors.
     *
     **/
    uint256_t ExtractHash(const encoding::json& jParams, const std::string& strKey, const uint256_t& hashDefault = ~uint256_t(0));


    /** ExtractString
     *
     *  Extract a string of given key from input parameters.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strKey The key we are extracting for.
     *
     *  @return the string extracted from parameters.
     *
     **/
    std::string ExtractString(const encoding::json& jParams, const std::string& strKey);


    /** ExtractHash
     *
     *  Extracts a hash value by template deduction.
     *
     *  @param[in] jParams The input parameters to extract from.
     *  @param[in] strKey The key value we are extracting for.
     *
     *  @return the converted object from string constructors.
     *
     **/
    template<typename Type>
    Type ExtractHash(const encoding::json& jParams, const std::string& strKey)
    {
        /* Check for missing parameter. */
        if(jParams.find(strKey) == jParams.end())
            throw Exception(-56, "Missing Parameter [", strKey, "]");

        /* Check for invalid type. */
        if(!jParams[strKey].is_string())
            throw Exception(-35, "Invalid parameter [", strKey, "], expecting [hex-string]");

        /* Check for hex encoding. */
        const std::string strHash = jParams[strKey].get<std::string>();
        if(!IsHex(strHash))
            throw Exception(-35, "Invalid parameter [", strKey, "], expecting [hex-string]");

        return Type(strHash);
    }


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
    Type ExtractInteger(const encoding::json& jParams, const std::string& strKey,
                        const Type nDefault = std::numeric_limits<Type>::max(),
                        const uint64_t nLimit = std::numeric_limits<Type>::max())
    {
        /* Check for missing parameter. */
        if(jParams.find(strKey) != jParams.end())
        {
            /* Catch parsing exceptions. */
            try
            {
                /* Build our return value. */
                uint64_t nRet = 0;

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

        /* Check for default parameter and throw if none supplied. */
        if(nDefault == std::numeric_limits<Type>::max())
            throw Exception(-56, "Missing Parameter [", strKey, "]");

        return nDefault;
    }
}
