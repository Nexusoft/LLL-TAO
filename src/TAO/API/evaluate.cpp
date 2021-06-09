/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/evaluate.h>
#include <TAO/API/include/get.h>
#include <TAO/API/types/exception.h>

#include <TAO/Register/types/object.h>

#include <Util/include/string.h>

namespace TAO::API
{

    /* Checks a given string against value to find wildcard pattern matches. */
    bool EvaluateWildcard(const std::string& strWildcard, const std::string& strValue)
    {
        /* Check for multiple asterisk which will break our splitting logic. */
        if(strWildcard.find("**") != strWildcard.npos)
            throw APIException(-56, "Query Syntax Error: duplicate wildcard not allowed ", strWildcard);

        /* Check for single asterisk. */
        if(strWildcard == "*")
            return true;

        /* Split by delimiter. */
        std::vector<std::string> vWildcard;
        ParseString(strWildcard, '*', vWildcard);

        /* Build an object to store our strings between wildcards. */
        std::string::size_type nPos = 0;
        for(uint32_t n = 0; n < vWildcard.size(); ++n)
        {
            /* Grab reference to check. */
            const std::string& strWildcard = vWildcard[n];

            /* If non empty, push to comparisons vector. */
            if(!strWildcard.empty())
            {
                /* Check if we have pattern 'check*' */
                const std::string::size_type nFind = strValue.find(strWildcard, nPos);
                if(nFind == strValue.npos)
                    return false;

                /* Check for wildcard postfix i.e. (check*) */
                if(n == 0 && nFind != 0)
                    return false;

                /* Check for wildcard prefix i.e. (*check) */
                if(n == vWildcard.size() - 1 && nFind != (strValue.length() - strWildcard.length()))
                    return false;

                /* Iterate our position past current string. */
                nPos = (nFind + strWildcard.length());
            }
        }

        return true;
    }


    /* Determines if an JSON object should be included in a list based on given clause. */
    bool EvaluateResults(const encoding::json& jClause, const encoding::json& jCheck)
    {
        /* Check that we are operating on a json object. */
        if(jClause["class"].get<std::string>() != "results")
            return true;

        /* Grab a copy of our current name we are processing. */
        const std::string strName = jClause["field"].get<std::string>();

        /* Check that we have any nested parameters. */
        const auto nFind = strName.find("."); //we use . to move up levels of recursion
        if(nFind != strName.npos)
        {
            /* Grab a copy of our substring. */
            const std::string strKey   = strName.substr(0, nFind);

            /* Check that we have nested values. */
            if(jCheck.find(strKey) == jCheck.end())
                return false;

            /* Grab a copy to pass back recursively. */
            encoding::json jAdjusted = jClause;
            jAdjusted["field"] = strName.substr(nFind + 1);

            return EvaluateResults(jAdjusted, jCheck[strKey]);
        }

        /* Check that field exists in object. */
        if(jCheck.find(strName) == jCheck.end())
            return false;

        /* Grab our OP code now. */
        const std::string strOP = jClause["operator"].get<std::string>();

        /* Check our types to compare now. */
        if(jCheck[strName].is_string())
        {
            /* Check syntax to omit < and > operators for string comparisons. */
            if(strOP.find_first_of("<>") != strOP.npos)
                throw APIException(-57, "Query Syntax Error: only '=' and '!=' operator allowed for type [string]");

            /* Grab our string to check. */
            const std::string strCheck = jCheck[strName].get<std::string>();

            /* Grab a reference of value to check. */
            const std::string strClause = jClause["value"].get<std::string>();
            if(strClause.find("*") != strClause.npos) //handle for all characters wildcard
            {
                /* Find if we have a wildcard match. */
                const bool fWildcard = EvaluateWildcard(strClause, strCheck);

                /* Check for standard equals operator. */
                if(strOP == "=" && fWildcard)
                    return true;

                /* Check for not operator. */
                if(strOP == "!=" && !fWildcard)
                    return true;
            }

            /* Check for not operator. */
            if(strOP == "!=" && strClause != strCheck)
                return true;

            /* Check the rest of our combinations. */
            if(strOP == "=" && strClause == strCheck)
                return true;
        }

        /* Check now for floating points. */
        if(jCheck[strName].is_number_float())
        {
            /* Grab a copy of our doubles here: XXX: we may want to convert and compare as ints. */
            const double dValue  = jCheck[strName].get<double>();
            const double dCheck  = std::stod(jClause["value"].get<std::string>());

            /* Check our not operator. */
            if(strOP == "!=" && dValue != dCheck)
                return true;
            else
            {
                /* Check that our values match. */
                if(strOP.find("=") != strOP.npos && dValue == dCheck)
                    return true;

                /* Check our less than operator. */
                if(strOP.find("<") != strOP.npos && dValue < dCheck)
                    return true;

                /* Check our greater than operator. */
                if(strOP.find(">") != strOP.npos && dValue > dCheck)
                    return true;
            }
        }

        /* Check now for integers. */
        if(jCheck[strName].is_number_integer())
        {
            /* Handle for unsigned integers. */
            if(jCheck[strName].is_number_unsigned())
            {
                /* Grab a copy of our doubles here: XXX: we may want to convert and compare as ints. */
                const uint64_t nValue  = jCheck[strName].get<uint64_t>();
                const uint64_t nCheck  = std::stoull(jClause["value"].get<std::string>());

                /* Check our not operator. */
                if(strOP == "!=" && nValue != nCheck)
                    return true;
                else
                {
                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && nValue == nCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && nValue < nCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && nValue > nCheck)
                        return true;
                }
            }
            else
            {
                /* Grab a copy of our doubles here: XXX: we may want to convert and compare as ints. */
                const int64_t nValue  = jCheck[strName].get<int64_t>();
                const int64_t nCheck  = std::stoll(jClause["value"].get<std::string>());

                /* Check our not operator. */
                if(strOP == "!=" && nValue != nCheck)
                    return true;
                else
                {
                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && nValue == nCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && nValue < nCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && nValue > nCheck)
                        return true;
                }
            }
        }


        /* Check now for floating points. */
        if(jCheck[strName].is_boolean())
        {
            /* Check syntax to omit < and > operators for string comparisons. */
            if(strOP.find_first_of("<>") != strOP.npos)
                throw APIException(-57, "Query Syntax Error: only '=' and '!=' operator allowed for type [bool]");

            /* Grab a copy of our doubles here: XXX: we may want to convert and compare as ints. */
            const bool fValue  = jCheck[strName].get<bool>();
            const bool fCheck  = (ToLower(jClause["value"].get<std::string>()) == "true") ? true : false;

            /* Check our not operator. */
            if(strOP == "!=" && fValue != fCheck)
                return true;

            /* Check that our values match. */
            if(strOP == "=" && fValue == fCheck)
                return true;
        }

        return false;
    }


    /* Determines if an object should be included in a list based on given clause. */
    bool EvaluateObject(const encoding::json& jClause, const TAO::Register::Object& objCheck)
    {
        /* Check that we are operating object. */
        if(jClause["class"].get<std::string>() != "object")
            return true;

        /* Grab a copy of our field to check. */
        const std::string strName = jClause["field"].get<std::string>();

        /* Check for the available type. */
        if(!objCheck.Check(strName))
            return false;

        /* Grab a reference of value to check. */
        const encoding::json& jCheck = jClause["value"];

        /* Now let's check our type. */
        uint8_t nType = 0;
        objCheck.Type(strName, nType);

        /* Grab our OP code now. */
        const std::string strOP = jClause["operator"].get<std::string>();

        /* Switch based on type. */
        switch(nType)
        {
            /* Check for uint8_t type. */
            case TAO::Register::TYPES::UINT8_T:
            case TAO::Register::TYPES::UINT16_T:
            case TAO::Register::TYPES::UINT32_T:
            case TAO::Register::TYPES::UINT64_T:
            {
                /* Grab a copy of our object register value. */
                uint64_t nValue = 0; //we will store all in 64 bits
                if(!jCheck.is_string())
                    break;

                /* We need to switch to decode the correct type out of the object register. */
                switch(nType)
                {
                    /* Case for 8-bit unsigned int, or unsigned char. */
                    case TAO::Register::TYPES::UINT8_T:
                        nValue = objCheck.get<uint8_t>(strName);
                        break;

                    /* Case for 16-bit unsigned int, or unsigned short. */
                    case TAO::Register::TYPES::UINT16_T:
                        nValue = objCheck.get<uint16_t>(strName);
                        break;

                    /* Case for 32-bit unsigned int, or unsigned int. */
                    case TAO::Register::TYPES::UINT32_T:
                        nValue = objCheck.get<uint32_t>(strName);
                        break;

                    /* Case for 64-bit unsigned int, or unsigned long. */
                    case TAO::Register::TYPES::UINT64_T:
                        nValue = objCheck.get<uint64_t>(strName);
                        break;
                }

                /* Special rule for balances that need to cast in and out of double. */
                uint64_t nCheck = std::stoull(jCheck.get<std::string>());
                if(strName == "balance" || strName == "stake" || strName == "supply")
                    nCheck = std::stod(jCheck.get<std::string>()) * GetFigures(objCheck);

                /* Check our not operator. */
                if(strOP == "!=" && nValue != nCheck)
                    return true;
                else
                {
                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && nValue == nCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && nValue < nCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && nValue > nCheck)
                        return true;
                }

                break;
            }

            /* Check for uint256_t type. */
            case TAO::Register::TYPES::UINT256_T:
            {
                /* Grab a copy of our hash to check. */
                const uint256_t hashValue = objCheck.get<uint256_t>(strName);
                if(!jCheck.is_string())
                    break;

                /* Special case where value is an address. */
                uint256_t hashCheck = uint256_t(jCheck.get<std::string>());
                if(strName == "token" || strName == "address")
                    hashCheck = TAO::Register::Address(jCheck.get<std::string>());

                /* Check our not operator. */
                if(strOP == "!=" && hashValue != hashCheck)
                    return true;
                else
                {
                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        return true;
                }

                break;
            }

            /* Check for uint512_t type. */
            case TAO::Register::TYPES::UINT512_T:
            {
                /* Set the return value from object register data. */
                const uint512_t hashValue = objCheck.get<uint512_t>(strName);
                if(!jCheck.is_string())
                    break;

                /* Our hash to check against. */
                const uint512_t hashCheck =
                    uint512_t(jCheck.get<std::string>());

                /* Check our not operator. */
                if(strOP == "!=" && hashValue != hashCheck)
                    return true;
                else
                {
                    /* Check that our values match. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        return true;
                }

                break;
            }

            /* Check for uint1024_t type. */
            case TAO::Register::TYPES::UINT1024_T:
            {
                /* Set the return value from object register data. */
                const uint1024_t hashValue = objCheck.get<uint1024_t>(strName);
                if(!jCheck.is_string())
                    break;

                /* Our hash to check against. */
                const uint1024_t hashCheck =
                    uint1024_t(jCheck.get<std::string>());

                /* Check our not operator. */
                if(strOP == "!=" && hashValue != hashCheck)
                    return true;
                else
                {
                    /* Check our regular operators. */
                    if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                        return true;

                    /* Check our less than operator. */
                    if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                        return true;

                    /* Check our greater than operator. */
                    if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                        return true;
                }

                break;
            }

            /* Check for string type. */
            case TAO::Register::TYPES::STRING:
            {
                /* Check syntax to omit < and > operators for string comparisons. */
                if(strOP.find_first_of("<>") != strOP.npos)
                    throw APIException(-57, "Query Syntax Error: only '=' and '!=' operator allowed for type [string]");

                /* Grab our value from object */
                const std::string strValue = objCheck.get<std::string>(strName);
                if(!jCheck.is_string())
                    break;

                /* Our string to check against. */
                const std::string strCheck = jCheck.get<std::string>();
                if(strCheck.find("*") != strCheck.npos) //handle for all characters wildcard
                {
                    /* Find if we have a wildcard match. */
                    const bool fWildcard = EvaluateWildcard(strCheck, strValue);

                    /* Check for standard equals operator. */
                    if(strOP == "=" && fWildcard)
                        return true;

                    /* Check for not operator. */
                    if(strOP == "!=" && !fWildcard)
                        return true;
                }

                /* Check for not operator. */
                if(strOP == "!=" && strValue != strCheck)
                    return true;

                /* Check the rest of our combinations. */
                if(strOP == "=" && strValue == strCheck)
                    return true;
            }
        }

        return false;
    }
}
