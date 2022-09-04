/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/address.h>

#include <LLD/include/global.h>

#include <TAO/Register/types/address.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/names.h>

#include <TAO/Register/include/names.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Extract an address from incoming parameters to derive from name or address field. */
    TAO::Register::Address ExtractAddress(const encoding::json& jParams, const std::string& strSuffix, const std::string& strDefault)
    {
        /* Check for our raw suffix formats here i.e. to, from, proof. */
        if(CheckParameter(jParams, strSuffix, "string"))
            return ExtractAddress(jParams[strSuffix].get<std::string>(), jParams);

        /* Cache a couple keys we will be using. */
        const std::string strName = "name"    + (strSuffix.empty() ? ("") : ("_" + strSuffix));
        const std::string strAddr = "address" + (strSuffix.empty() ? ("") : ("_" + strSuffix));

        /* Check if we are resolving for a name or namespace. */
        if(CheckRequest(jParams, "type", "string, array"))
        {
            /* Get our type to compare to. */
            const std::set<std::string>& setTypes = ExtractTypes(jParams);

            /* Iterate through our types now. */
            for(const std::string& strType : setTypes)
            {
                /* Check for name or namespace resolution. */
                if(strType == "name" || strType == "namespace" || strType == "global" || strType == "local")
                {
                    /* Check for namespace to get specific parameters. */
                    if(strType == "namespace")
                        return Names::ResolveNamespace(jParams);

                    /* Grab our name from incoming parameters. */
                    const std::string& strLookup =
                        jParams[strName].get<std::string>();

                    /* Declare the return register address hash */
                    TAO::Register::Address hashRegister =
                        TAO::API::ADDRESS_NONE;;

                    /* Get the Name object by name */
                    const TAO::Register::Object tObject =
                        Names::GetName(jParams, strLookup, hashRegister, false);

                    /* Check that we found a name record. */
                    if(hashRegister == TAO::API::ADDRESS_NONE)
                        throw Exception(-101, "Unknown name: ", strLookup);

                    return hashRegister;
                }
            }
        }

        /* If name is provided then use this to deduce the register address, */
        if(CheckParameter(jParams, strName, "string"))
            return Names::ResolveAddress(jParams, jParams[strName].get<std::string>(), false);

        /* Otherwise let's check for the raw address format. */
        else if(CheckParameter(jParams, strAddr, "string"))
        {
            /* Declare our return value. */
            const TAO::Register::Address hashRet =
                TAO::Register::Address(jParams[strAddr].get<std::string>());

            /* Check that it is valid */
            if(hashRet.IsValid())
                return hashRet;

            throw Exception(-35, "Invalid address [", hashRet.ToString(), "]");
        }

        /* Check for our default values. */
        else if(!strDefault.empty())
            return ExtractAddress(strDefault, jParams);

        /* Check for any/all request types. */
        if(CheckRequest(jParams, "type", "string, array"))
        {
            /* Grab a copy of our request type. */
            const std::string strType =
                jParams["request"]["type"].get<std::string>();

            /* Check for the ALL name, that debits from all relevant accounts. */
            if(strType == "all")
                return TAO::API::ADDRESS_ALL;

            /* Check for the ANY name, that debits from any account of any token, mixed */
            if(strType == "any")
                return TAO::API::ADDRESS_ANY;
        }

        /* This exception is for name/address */
        if(!strSuffix.empty())
            throw Exception(-56, "Missing Parameter [", strSuffix, "]");

        throw Exception(-56, "Missing Parameter [", strName, "/", strAddr, "]");
    }


    /* Extract an address from incoming parameters to derive from name or address field. */
    TAO::Register::Address ExtractToken(const encoding::json& jParams)
    {
        /* If name is provided then use this to deduce the register address, */
        if(CheckParameter(jParams, "token", "string"))
            return ExtractAddress(jParams["token"].get<std::string>(), jParams);

        return uint256_t(0);
    }


    /* Extract an address from a single string. */
    TAO::Register::Address ExtractAddress(const std::string& strAddress, const encoding::json& jParams)
    {
        /* Declare our return value. */
        const TAO::Register::Address hashRet =
            TAO::Register::Address(strAddress);

        /* Check that it is valid */
        if(hashRet.IsValid())
            return hashRet;

        /* Allow address to be a name record as well. */
        return Names::ResolveAddress(jParams, strAddress, false);
    }


    /* Extract a legacy address from incoming parameters to derive from name or address field. */
    bool ExtractLegacy(const encoding::json& jParams, Legacy::NexusAddress &addrLegacy, const std::string& strSuffix)
    {
        /* Check for our raw suffix formats here i.e. to, from, proof. */
        if(CheckParameter(jParams, strSuffix, "string"))
            return ExtractLegacy(jParams[strSuffix].get<std::string>(), addrLegacy);

        /* Cache a couple keys we will be using. */
        const std::string strAddr =
            "address" + (strSuffix.empty() ? ("") : ("_" + strSuffix));

        /* Let's check for the raw address format. */
        if(CheckParameter(jParams, strAddr, "string"))
            return ExtractLegacy(jParams[strAddr].get<std::string>(), addrLegacy);

        return false;
    }


    /* Extract a legacy address from a single string. */
    bool ExtractLegacy(const std::string& strAddress, Legacy::NexusAddress &addrLegacy)
    {
        /* Build a legacy address for this check. */
        Legacy::NexusAddress addr =
            Legacy::NexusAddress(strAddress);

        /* Check for invalid address. */
        if(!addr.IsValid())
            return false;

        /* Set our internal legacy address hash. */
        addrLegacy = std::move(addr);

        return true;
    }


    /* Extract a market identification from incoming parameters */
    std::pair<uint256_t, uint256_t> ExtractMarket(const encoding::json& jParams)
    {
        /* Check that the market parameter exists. */
        if(!CheckParameter(jParams, "market", "string"))
            throw Exception(-56, "Missing Parameter [market]");

        /* Get our pair string. */
        const std::string& strMarket = jParams["market"].get<std::string>();

        /* Parse into components. */
        std::vector<std::string> vMarkets(2, "");

        /* Find our last value here. */
        const uint64_t nPos = strMarket.find_last_of("/");
        if(nPos == strMarket.npos)
            throw Exception(-35, "Invalid parameter [market], expecting [ABC/DEF]");

        /* Build our base pair now. */
        vMarkets[0] = strMarket.substr(0, nPos);
        vMarkets[1] = strMarket.substr(nPos + 1);

        /* Build our new pair and return. */
        return std::make_pair
        (
            ExtractAddress(vMarkets[0], jParams),
            ExtractAddress(vMarkets[1], jParams)
        );
    }


    /* Extract a genesis-id from input parameters which could be either username or genesis keys. */
    uint256_t ExtractGenesis(const encoding::json& jParams)
    {
        /* Check to see if specific genesis has been supplied */
        if(CheckParameter(jParams, "genesis", "string"))
            return uint256_t(jParams["genesis"].get<std::string>());

        /* Check if username has been supplied instead. */
        if(CheckParameter(jParams, "username", "string"))
            return TAO::Ledger::SignatureChain::Genesis(jParams["username"].get<std::string>().c_str());

        return Authentication::Caller(jParams);
    }


    /* Extract a recipient genesis-id from input parameters which could be either username or genesis keys. */
    uint256_t ExtractRecipient(const encoding::json& jParams)
    {
        /* Check to see if specific genesis has been supplied */
        if(CheckParameter(jParams, "recipient", "string"))
        {
            /* Check for hex encoding. */
            const std::string strRecipient = jParams["recipient"].get<std::string>();
            if(IsHex(strRecipient))
            {
                /* Copy our genesis-id over to make a check. */
                const uint256_t hashGenesis = uint256_t(jParams["recipient"].get<std::string>());

                /* Check that this is a valid genesis-id. */
                if(hashGenesis.GetType() == TAO::Ledger::GENESIS::UserType())
                    return hashGenesis;
            }

            return TAO::Ledger::SignatureChain::Genesis(jParams["recipient"].get<std::string>().c_str());
        }

        throw Exception(-56, "Missing Parameter [recipient]");
    }


    /* Extracts a hash value for reading a txid. */
    uint512_t ExtractHash(const encoding::json& jParams)
    {
        /* Check for our hash parameter. */
        if(CheckParameter(jParams, "txid", "string"))
        {
            /* Check for hex encoding. */
            const std::string strHash = jParams["txid"].get<std::string>();
            if(!IsHex(strHash))
                throw Exception(-35, "Invalid parameter [txid], expecting [hex-string]");

            return uint512_t(strHash);
        }

        /* Check for our hash parameter. */
        if(CheckParameter(jParams, "hash", "string"))
        {
            /* Check for hex encoding. */
            const std::string strHash = jParams["hash"].get<std::string>();
            if(!IsHex(strHash))
                throw Exception(-35, "Invalid parameter [hash], expecting [hex-string]");

            return uint512_t(strHash);
        }

        throw Exception(-57, "Invalid Parameter [hash or txid]");
    }


    /* Extract an amount value from either string or integer and convert to its final value. */
    uint64_t ExtractAmount(const encoding::json& jParams, const uint64_t nFigures, const std::string& strKey)
    {
        /* Cache our name with prefix calculated. */
        const std::string strAmount =
            (strKey.empty() ? ("amount") : strKey);

        /* Check for missing parameter. */
        if(CheckParameter(jParams, strAmount, "string, number"))
        {
            /* Watch our numeric limits. */
            const uint64_t nLimit = std::numeric_limits<uint64_t>::max();

            /* Catch parsing exceptions. */
            try
            {
                /* Initialize our return value. */
                double dValue = 0;

                /* Convert to value if in string form. */
                if(jParams[strAmount].is_string())
                    dValue = std::stod(jParams[strAmount].get<std::string>());

                /* Grab value regularly if it is integer. */
                else if(jParams[strAmount].is_number_unsigned())
                    dValue = double(jParams[strAmount].get<uint64_t>());

                /* Check for a floating point value. */
                else if(jParams[strAmount].is_number_float())
                    dValue = jParams[strAmount].get<double>();

                /* Otherwise we have an invalid parameter. */
                else
                    throw Exception(-57, "Invalid Parameter [", strAmount, "]");

                /* Check our minimum range. */
                if(dValue <= 0)
                    throw Exception(-68, "[", strAmount, "] too small [", dValue, "]");

                /* Check our limits and ranges now. */
                if(uint64_t(dValue) > (nLimit / nFigures))
                    throw Exception(-60, "[", strAmount, "] out of range [", nLimit, "]");

                /* Final compute of our figures. */
                return uint64_t(dValue * nFigures);
            }
            catch(const encoding::detail::exception& e) { throw Exception(-57, "Invalid Parameter [", strAmount, "]");           }
            catch(const std::invalid_argument& e)       { throw Exception(-57, "Invalid Parameter [", strAmount, "]");           }
            catch(const std::out_of_range& e)           { throw Exception(-60, "[", strAmount, "] out of range [", nLimit, "]"); }
        }

        throw Exception(-56, "Missing Parameter [", strAmount, "]");
    }


    /* Extract the fieldname string value from input parameters as only string. */
    std::string ExtractFieldname(const encoding::json& jParams)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "fieldname", "string"))
            throw Exception(-28, "Missing parameter [request::fieldname] for command");

        /* Grab a reference of our string. */
        const std::string& strField =
            jParams["request"]["fieldname"].get<std::string>();

        /* Find last delimiter. */
        const auto nFind = strField.rfind(".");
        if(nFind == strField.npos)
            return strField;

        /* Otherwise return substring. */
        return strField.substr(nFind + 1);
    }


    /* Extract format specifier from input parameters. */
    std::string ExtractFormat(const encoding::json& jParams, const std::string& strDefault, const std::string& strAllowed)
    {
        /* Check for formatting parameter. */
        if(CheckParameter(jParams, "format", "string"))
        {
            /* Grab our format string. */
            const std::string strFormat =
                ToLower(jParams["format"].get<std::string>());

            /* Check for allowed formats. */
            const auto nFind = strAllowed.find(strFormat);
            if(nFind == strAllowed.npos)
                throw Exception(-35, "Invalid parameter [format], expecting [", strAllowed, "]");

            return strFormat;
        }

        /* Check for empty default parameter. */
        if(strDefault.empty() || strDefault == "required")
            throw Exception(-28, "Missing parameter [format] for command");

        return strDefault;
    }


    /* Extract key scheme specifier from input parameters. */
    uint8_t ExtractScheme(const encoding::json& jParams, const std::string& strAllowed)
    {
        /* Check for formatting parameter. */
        if(!CheckParameter(jParams, "scheme", "string"))
            return TAO::Ledger::SIGNATURE::BRAINPOOL;

        /* Grab our format string. */
        const std::string strScheme =
            ToLower(jParams["scheme"].get<std::string>());

        /* Check for allowed formats. */
        const auto nFind = strAllowed.find(strScheme);
        if(nFind == strAllowed.npos)
            throw Exception(-35, "Invalid parameter [scheme], expecting [", strAllowed, "]");

        /* Grab our type from string. */
        const uint8_t nRet =
            TAO::Ledger::SIGNATURE::TYPE(strScheme);

        /* Double check our mapping worked. */
        if(nRet == TAO::Ledger::SIGNATURE::RESERVED)
            throw Exception(-35, "Invalid parameter [scheme], expecting [", strAllowed, "]");

        return nRet;
    }


    /* Extract the type string value from input parameters as only string. */
    std::string ExtractType(const encoding::json& jParams)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "type", "string"))
            throw Exception(-28, "Missing parameter [request::type] for command");

        return jParams["request"]["type"].get<std::string>();
    }


    /* Extract the type string value from input parameters as either array or string. */
    std::set<std::string> ExtractTypes(const encoding::json& jParams)
    {
        /* Check for our request parameters first, since this method can be called without */
        if(!CheckRequest(jParams, "type", "string, array"))
            throw Exception(-28, "Missing parameter [request::type] for command");

        /* Check for array to iterate. */
        const encoding::json jTypes = jParams["request"]["type"];
        if(jTypes.is_array())
        {
            /* Loop through standards. */
            std::set<std::string> setRet;
            for(const auto& jCheck : jTypes)
                setRet.insert(jCheck.get<std::string>());

            return setRet;
        }

        return { jTypes.get<std::string>() };
    }


    /* Extract the pin number from input parameters. */
    SecureString ExtractPIN(const encoding::json& jParams, const std::string& strPrefix)
    {
        /* Get the parameter name we are searching. */
        const std::string strName =
            (strPrefix.empty() ? "pin" : strPrefix + "_pin");

        /* Check for correct parameter types. */
        if(!CheckParameter(jParams, strName, "string, number"))
            throw Exception(-28, "Missing parameter [", strName, "] for command");

        /* Handle for unsigned integer. */
        if(jParams[strName].is_number_unsigned())
        {
            /* Grab our secure string pin. */
            const SecureString strPIN =
                SecureString(debug::safe_printstr(jParams[strName].get<uint64_t>()).c_str());

            /* Check for empty values. */
            if(strPIN.empty())
                throw Exception(-57, "Invalid Parameter [", strName, ".empty()]");

            return strPIN;
        }

        /* Handle for string representation. */
        if(jParams[strName].is_string())
        {
            /* Grab our secure string pin. */
            const SecureString strPIN =
                SecureString(jParams[strName].get<std::string>().c_str());

            /* Check for empty values. */
            if(strPIN.empty())
                throw Exception(-57, "Invalid Parameter [", strName, ".empty()]");

            return strPIN;
        }

        throw Exception(-35, "Invalid parameter [", strName, "=", jParams[strName].type_name(), "], expecting [", strName, "=string, number]");
    }


    /* Extract a verbose argument from input parameters in either string or integer format. */
    uint32_t ExtractVerbose(const encoding::json& jParams, const uint32_t nMinimum)
    {
        /* If verbose not supplied, use default parameter. */
        if(jParams.find("verbose") == jParams.end())
            return nMinimum;

        /* Extract paramter if in integer form. */
        if(jParams["verbose"].is_number_unsigned())
            return std::max(nMinimum, jParams["verbose"].get<uint32_t>());

        /* Extract parameter if in string form. */
        std::string strVerbose = "default";
        if(jParams["verbose"].is_string())
        {
            /* Grab a copy of the string now. */
            strVerbose = jParams["verbose"].get<std::string>();

            /* Check if it is an integer. */
            if(IsAllDigit(strVerbose))
                return std::max(nMinimum, uint32_t(std::stoull(strVerbose)));
        }

        /* Otherwise check our base verbose levels. */
        if(strVerbose == "default")
            return std::max(nMinimum, uint32_t(1));

        /* Summary maps to verbose level 2. */
        if(strVerbose == "summary")
            return std::max(nMinimum, uint32_t(2));

        /* Detail maps to verbose level 3. */
        else if(strVerbose == "detail")
            return std::max(nMinimum, uint32_t(3));

        throw Exception(-57, "Invalid Parameter [verbose]");
    }


    /* Extracts the paramers applicable to a List API call in order to apply a filter/offset/limit to the result */
    void ExtractList(const encoding::json& jParams, std::string &strOrder, uint32_t &nLimit, uint32_t &nOffset)
    {
        /* Check for page parameter. */
        uint32_t nPage = 0;
        if(CheckParameter(jParams, "page", "string, number"))
        {
            /* Check for string. */
            if(jParams["page"].is_string())
                nPage = std::stoul(jParams["page"].get<std::string>());

            /* Check for number. */
            else if(jParams["page"].is_number_integer())
                nPage = jParams["page"].get<uint32_t>();

            /* Otherwise we have an invalid parameter. */
            else
                throw Exception(-57, "Invalid Parameter [page]");
        }

        /* Check for offset parameter. */
        nOffset = 0;
        if(CheckParameter(jParams, "offset", "string, number"))
        {
            /* Check for string. */
            if(jParams["offset"].is_string())
                nOffset = std::stoul(jParams["offset"].get<std::string>());

            /* Check for number. */
            else if(jParams["offset"].is_number_integer())
                nOffset = jParams["offset"].get<uint32_t>();

            /* Otherwise we have an invalid parameter. */
            else
                throw Exception(-57, "Invalid Parameter [offset=", jParams["offset"].type_name(), "]");
        }

        /* Check for limit and offset parameter. */
        nLimit = 100;
        if(CheckParameter(jParams, "limit", "string, number"))
        {
            /* Check for string. */
            if(jParams["limit"].is_string())
            {
                /* Grab our limit from parameters. */
                const std::string strLimit = jParams["limit"].get<std::string>();

                /* Check to see whether the limit includes an offset comma separated */
                if(IsAllDigit(strLimit))
                {
                    /* No offset included in the limit */
                    nLimit = std::stoul(strLimit);
                }
                else if(EqualsNoCase(strLimit, "none"))
                {
                    /* No limit includes as many values as possible. */
                    nLimit = std::numeric_limits<uint32_t>::max();
                }
                else if(strLimit.find(","))
                {
                    /* Parse the limit and offset */
                    std::vector<std::string> vParts;
                    ParseString(strLimit, ',', vParts);

                    /* Check for expected sizes. */
                    if(vParts.size() < 2)
                        throw Exception(-57, "Invalid Parameter [limit.size() < 2]");

                    /* Get the limit */
                    nLimit = std::stoul(trim(vParts[0]));

                    /* Get the offset */
                    nOffset = std::stoul(trim(vParts[1]));
                }
                else
                    throw Exception(-57, "Invalid Parameter [limit=", jParams["limit"].type_name(), "]");
            }

            /* Check for number. */
            else if(jParams["limit"].is_number_integer())
                nLimit = jParams["limit"].get<uint32_t>();

            /* Otherwise we have an invalid parameter. */
            else
                throw Exception(-57, "Invalid Parameter [limit=", jParams["limit"].type_name(), "]");
        }

        /* If no offset explicitly included calculate it from the limit + page */
        if(nOffset == 0 && nPage > 0)
            nOffset = nLimit * nPage;

        /* Get sort order*/
        if(CheckParameter(jParams, "order", "string"))
        {
            /* Grab a copy of the string to check against valid types. */
            const std::string strCheck = jParams["order"].get<std::string>();
            if(strCheck != "asc" && strCheck != "desc")
                throw Exception(-57, "Invalid Parameter [sort=", strCheck, "]");

            /* Now assign to proper orders. */
            strOrder = strCheck;
        }
    }


    /* Extracts the paramers applicable to a List API call in order to apply a filter. This overload includes a sort field. */
    void ExtractList(const encoding::json& jParams, std::string &strOrder, std::string &strSort, uint32_t &nLimit, uint32_t &nOffset)
    {
        /* Extract the previous parameters. */
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* Check for sort ordering */
        if(CheckParameter(jParams, "sort", "string"))
            strSort = jParams["sort"].get<std::string>();
    }


    /** ExtractBoolean
     *
     *  Extract a boolean value from input parameters.
     *
     *  @param[in] jParams The input parameters passed into request
     *  @param[in] strKey The parameter key we are extracting for.
     *
     *  @return true or false based on given value.
     *
     **/
    bool ExtractBoolean(const encoding::json& jParams, const std::string& strKey, const bool fDefault)
    {
        /* Check for correct parameter types. */
        if(!CheckParameter(jParams, strKey, "string, number, boolean"))
            return fDefault;

        /* Handle if boolean. */
        if(jParams[strKey].is_boolean())
            return jParams[strKey].get<bool>();

        /* Handle if number. */
        if(jParams[strKey].is_number_unsigned())
        {
            /* Extract the parameter. */
            const uint64_t nValue =
                jParams[strKey].get<uint64_t>();

            /* Handle for true. */
            if(nValue == 1)
                return true;

            /* Handle for false. */
            if(nValue == 0)
                return false;

            throw Exception(-60, "[", strKey, "] out of range [1]");
        }

        /* Handle if string. */
        if(jParams[strKey].is_string())
        {
            /* Extract the string. */
            const std::string strValue =
                jParams[strKey].get<std::string>();

            /* Handle for true. */
            if(strValue == "1" || strValue == "true")
                return true;

            /* Handle for false. */
            if(strValue == "0" || strValue == "false")
                return false;

            throw Exception(-57, "Invalid Parameter [", strKey, "=", strValue, "]");
        }

        throw Exception(-57, "Invalid Parameter [", strKey, "=", jParams[strKey].type_name(), "]");
    }


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
    uint256_t ExtractHash(const encoding::json& jParams, const std::string& strKey, const uint256_t& hashDefault)
    {
        /* Check for default parameter. */
        const bool fDefault =
            (hashDefault < ~uint256_t(0));

        /* Check for missing parameter. */
        if(jParams.find(strKey) == jParams.end())
        {
            /* Allow extracting by default value if enabled. */
            if(fDefault)
                return hashDefault;

            throw Exception(-56, "Missing Parameter [", strKey, "]");
        }

        /* Check for invalid type. */
        if(!jParams[strKey].is_string())
            throw Exception(-35, "Invalid parameter [", strKey, "], expecting [hex-string]");

        /* Check for hex encoding. */
        const std::string strHash = jParams[strKey].get<std::string>();
        if(IsHex(strHash))
        {
            /* Check that we don't conflict our default parameters. */
            const uint256_t hashRet = uint256_t(strHash);
            if(hashRet == ~uint256_t(0))
                throw Exception(-35, "Invalid parameter [", strKey, "], value too large");

            return hashRet;
        }

        /* Otherwise assume Base58 encoding */
        const TAO::Register::Address hashRegister =
            TAO::Register::Address(strHash);

        /* Check value doesn't exceed our allowed range. */
        if(hashRegister == ~uint256_t(0))
            throw Exception(-35, "Invalid parameter [", strKey, "], value too large");

        /* Check that this is a valid base58 string. */
        if(!hashRegister.IsValid())
            throw Exception(-35, "Invalid parameter [", strKey, "], expecting [base58-string]");

        return hashRegister;
    }


    /* Extract a string of given key from input parameters. */
    std::string ExtractString(const encoding::json& jParams, const std::string& strKey)
    {
        /* Check for missing parameters. */
        if(!CheckParameter(jParams, strKey, "string"))
            throw Exception(-56, "Missing Parameter [", strKey, "]");

        return jParams[strKey].get<std::string>();
    }


    /*Extracts an floating point value from given input parameters.*/
    double ExtractDouble(const encoding::json& jParams, const std::string& strKey,
                         const double dDefault, const double dLimit)
    {
        /* Check for missing parameter. */
        if(jParams.find(strKey) != jParams.end())
        {
            /* Catch parsing exceptions. */
            try
            {
                /* Build our return value. */
                double dRet = 0;

                /* Convert to value if in string form. */
                if(jParams[strKey].is_string())
                    dRet = std::stod(jParams[strKey].get<std::string>());

                /* Grab value regularly if it is integer. */
                else if(jParams[strKey].is_number_integer())
                    dRet = static_cast<double>(jParams[strKey].get<int64_t>());

                /* Grab value regularly if it is unsigned integer. */
                else if(jParams[strKey].is_number_unsigned())
                    dRet = static_cast<double>(jParams[strKey].get<uint64_t>());

                /* Check if value is a double. */
                else if(jParams[strKey].is_number_float())
                    dRet = jParams[strKey].get<double>();

                /* Otherwise we have an invalid parameter. */
                else
                    throw Exception(-57, "Invalid Parameter [", strKey, "]");

                /* Check our numeric limits now. */
                if(dRet > dLimit)
                    throw Exception(-60, "[", strKey, "] out of range [", dLimit, "]");

                return dRet;
            }
            catch(const encoding::detail::exception& e) { throw Exception(-57, "Invalid Parameter [", strKey, "]");           }
            catch(const std::invalid_argument& e)       { throw Exception(-57, "Invalid Parameter [", strKey, "]");           }
            catch(const std::out_of_range& e)           { throw Exception(-60, "[", strKey, "] out of range [", dLimit, "]"); }
        }

        /* Check for default parameter and throw if none supplied. */
        if(dDefault == std::numeric_limits<double>::max())
            throw Exception(-56, "Missing Parameter [", strKey, "]");

        return dDefault;
    }

} // End TAO namespace
