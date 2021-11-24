/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/names.h>

#include <Util/include/debug.h>
#include <Util/include/json.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Converts a query variable into a string. */
    std::string VariableToJSON(const std::string& strValue)
    {
        /* Find where parameters start. */
        const auto nBegin = strValue.find('(');
        if(nBegin == strValue.npos)
            throw Exception(-120, "Query Syntax Error: variable format must be variable(`params`);", strValue);

        /* Parse out our variable name. */
        const std::string strVariable =
            strValue.substr(0, nBegin);

        /* Find our ending iterator. */
        const auto nEnd = strValue.find(')');
        if(nEnd == strValue.npos)
            throw Exception(-120, "Query Syntax Error: variable format must be variable(`params`);", strValue);

        /* Get our parameter values. */
        const std::string strParams =
            strValue.substr(nBegin + 1, nEnd - nBegin - 1);

        /* Date variable requires a string argument. */
        const auto nOpen = strParams.find('`');
        if(nOpen == strParams.npos)
            throw Exception(-120, "Query Syntax Error: variable format requires string ", strVariable, "(`params`); | ", strValue);

        /* Check for closing string. */
        const auto nClose = strParams.rfind('`');
        if(nClose == strParams.npos)
            throw Exception(-120, "Query Syntax Error: variable format requires string ", strVariable, "(`params`); | ", strValue);

        /* Check we have both open and close. */
        if(nOpen == nClose)
            throw Exception(-120, "Query Syntax Error: variable string must close ", strVariable, "(`params`); | ", strValue);

        /* Now get our parameter values. */
        const std::string strParam =
            strParams.substr(nOpen + 1, nClose - 1);

        /* Handle for date variable types. */
        if(strVariable == "date")
        {
            /* Build our time struct from strptime. */
            struct tm tm;
            if(!runtime::strptime(strParam.c_str(), tm))
                throw Exception(-121, "Query Syntax Error: date format must include year ex. date(`2021`);");

            /* Build a simple return string. */
            return debug::safe_printstr(std::mktime(&tm));
        }

        /* Handle for the name variable types. */
        if(strVariable == "name")
        {
            /* Build our address from base58. */
            const uint256_t hashAddress =
                TAO::Register::Address(strParam);

            /* Check for a valid reverse lookup entry. */
            std::string strName;
            if(!Names::ReverseLookup(hashAddress, strName))
                throw Exception(-121, "Query Syntax Error: name reverse lookup entry not found");

            return strName;
        }

        /* Handle for address resolver. */
        if(strVariable == "resolve")
        {
            /* Temporary value to pass. */
            encoding::json jParams;

            /* Build our address from name record. */
            const uint256_t hashAddress =
                Names::ResolveAddress(jParams, strParam, true);

            return TAO::Register::Address(hashAddress).ToString();
        }

        return strValue;
    }
}
