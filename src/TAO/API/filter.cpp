/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/filter.h>
#include <TAO/API/include/get.h>

#include <TAO/Register/types/object.h>

namespace TAO::API
{
    /* Determines if an object should be included in a list based on input parameters. */
    bool FilterObject(const json::json& jParams, const TAO::Register::Object& objCheck)
    {
        /* First let's check some core register values. */
        if(jParams.find("owner") != jParams.end())
        {
            /* Grab copy of our owner from parameters. */
            const uint256_t hashOwner = uint256_t(jParams["owner"].get<std::string>());
            if(hashOwner != objCheck.hashOwner)
                return false;
        }

        /* Check for a core structure filter for version. */
        if(jParams.find("version") != jParams.end())
        {
            /* Grab a copy of our version from parameters. */
            const uint64_t nVersion = uint64_t(std::stoull(jParams["version"].get<std::string>()));
            if(nVersion != objCheck.nVersion)
                return false;
        }

        /* Loop through all of our type names. */
        const std::vector<std::string> vFieldNames = objCheck.ListFields();
        for(const auto& strName : vFieldNames)
        {
            /* Check if we have a key in our parameters. */
            if(jParams.find(strName) == jParams.end())
                continue;

            /* Cache a reference of our value. */
            const json::json jCheck = jParams[strName];
            if(!jCheck.is_primitive())
                continue;

            /* We want to catch and continue here, in case there are any issues with parameters. */
            try
            {
                /* Now let's check our type. */
                uint8_t nType = 0;
                objCheck.Type(strName, nType);

                /* Switch based on type. */
                switch(nType)
                {
                    /* Check for uint8_t type. */
                    case TAO::Register::TYPES::UINT8_T:
                    {
                        /* Set the return value from object register data. */
                        const uint8_t nValue = objCheck.get<uint8_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(nValue != std::stoull(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint16_t type. */
                    case TAO::Register::TYPES::UINT16_T:
                    {
                        /* Set the return value from object register data. */
                        const uint16_t nValue = objCheck.get<uint16_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(nValue != std::stoull(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint32_t type. */
                    case TAO::Register::TYPES::UINT32_T:
                    {
                        /* Set the return value from object register data. */
                        const uint32_t nValue = objCheck.get<uint32_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(nValue != std::stoull(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint64_t type. */
                    case TAO::Register::TYPES::UINT64_T:
                    {
                        /* Grab a copy of our object register value. */
                        const uint64_t nValue = objCheck.get<uint64_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Special rule for balances that need to cast in and out of double. */
                        if(strName == "balance" || strName == "stake" || strName == "supply")
                        {
                            /* Convert our input parameter into integer converted uint64_t. */
                            const uint64_t nCheck = std::stod(jCheck.get<std::string>()) * GetFigures(objCheck);
                            if(nValue != nCheck)
                                return false;
                        }

                        /* Check that our values match. */
                        else if(nValue != std::stoull(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint256_t type. */
                    case TAO::Register::TYPES::UINT256_T:
                    {
                        /* Grab a copy of our hash to check. */
                        const uint256_t hashValue = objCheck.get<uint256_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Special case where value is an address. */
                        if(strName == "token" || strName == "address")
                        {
                            /* Check our value against a register address. */
                            if(hashValue != TAO::Register::Address(jCheck.get<std::string>()))
                                return false;
                        }

                        /* Check that our values match. */
                        else if(hashValue != uint256_t(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint512_t type. */
                    case TAO::Register::TYPES::UINT512_T:
                    {
                        /* Set the return value from object register data. */
                        const uint512_t hashValue = objCheck.get<uint512_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(hashValue != uint512_t(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for uint1024_t type. */
                    case TAO::Register::TYPES::UINT1024_T:
                    {
                        /* Set the return value from object register data. */
                        const uint1024_t hashValue = objCheck.get<uint1024_t>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(hashValue != uint1024_t(jCheck.get<std::string>()))
                            return false;

                        break;
                    }

                    /* Check for string type. */
                    case TAO::Register::TYPES::STRING:
                    {
                        /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                        const std::string strValue = objCheck.get<std::string>(strName);
                        if(!jCheck.is_string())
                            continue;

                        /* Check that our values match. */
                        if(strValue != jCheck.get<std::string>())
                            return false;

                        break;
                    }
                }
            }

            /* We want to catch any string conversion erros and continue. */
            catch (std::exception & e) { continue; }
        }

        return true;
    }
}
