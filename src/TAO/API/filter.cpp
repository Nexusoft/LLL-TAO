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
    bool FilterObject(const encoding::json& jParams, const TAO::Register::Object& objCheck)
    {
        /* Check for a where clause. */
        if(jParams.find("where") == jParams.end())
            return true; //no filters

        /* Loop through our where clauses. */
        for(const auto& jClause : jParams["where"])
        {
            /* Check that we are operating object. */
            if(jClause["class"].get<std::string>() != "object")
                continue;

            /* Grab a copy of our field to check. */
            const std::string strName = jClause["field"].get<std::string>();

            /* First let's check some core register values. */
            if(strName == "owner")
            {
                /* Grab copy of our owner from parameters. */
                const uint256_t hashOwner = uint256_t(jClause["value"].get<std::string>());
                if(hashOwner != objCheck.hashOwner)
                    return false;
            }

            /* Check for a core structure filter for version. */
            if(strName == "version")
            {
                /* Grab a copy of our version from parameters. */
                const uint64_t nVersion = uint64_t(std::stoull(jClause["value"].get<std::string>()));
                if(nVersion != objCheck.nVersion)
                    return false;
            }

            /* Check for the available type. */
            if(!objCheck.Check(strName))
                continue;

            /* We want to catch and continue here, in case there are any issues with parameters. */
            try
            {
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
                            continue;

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

                        /* Check that our values match. */
                        bool fEvaluate = false;
                        if(strOP.find("=") != strOP.npos && nValue == nCheck)
                            fEvaluate = true;

                        /* Check our less than operator. */
                        if(strOP.find("<") != strOP.npos && nValue < nCheck)
                            fEvaluate = true;

                        /* Check our greater than operator. */
                        if(strOP.find(">") != strOP.npos && nValue > nCheck)
                            fEvaluate = true;

                        /* Break out if object failed filters. */
                        if(!fEvaluate)
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
                        uint256_t hashCheck = uint256_t(jCheck.get<std::string>());
                        if(strName == "token" || strName == "address")
                            hashCheck = TAO::Register::Address(jCheck.get<std::string>());

                        /* Check that our values match. */
                        bool fEvaluate = false;
                        if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                            fEvaluate = true;

                        /* Check our less than operator. */
                        if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                            fEvaluate = true;

                        /* Check our greater than operator. */
                        if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                            fEvaluate = true;

                        /* Break out if object failed filters. */
                        if(!fEvaluate)
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

                        /* Our hash to check against. */
                        const uint512_t hashCheck =
                            uint512_t(jCheck.get<std::string>());

                        /* Check that our values match. */
                        bool fEvaluate = false;
                        if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                            fEvaluate = true;

                        /* Check our less than operator. */
                        if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                            fEvaluate = true;

                        /* Check our greater than operator. */
                        if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                            fEvaluate = true;

                        /* Break out if object failed filters. */
                        if(!fEvaluate)
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

                        /* Our hash to check against. */
                        const uint1024_t hashCheck =
                            uint1024_t(jCheck.get<std::string>());

                        /* Check that our values match. */
                        bool fEvaluate = false;
                        if(strOP.find("=") != strOP.npos && hashValue == hashCheck)
                            fEvaluate = true;

                        /* Check our less than operator. */
                        if(strOP.find("<") != strOP.npos && hashValue < hashCheck)
                            fEvaluate = true;

                        /* Check our greater than operator. */
                        if(strOP.find(">") != strOP.npos && hashValue > hashCheck)
                            fEvaluate = true;

                        /* Break out if object failed filters. */
                        if(!fEvaluate)
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

                        /* Our hash to check against. */
                        const std::string strCheck = jCheck.get<std::string>();

                        /* Check that our values match. */
                        bool fEvaluate = false;
                        if(strOP.find("=") != strOP.npos && strValue == strCheck)
                            fEvaluate = true;

                        /* Check our less than operator. */
                        if(strOP.find("<") != strOP.npos && strValue < strCheck)
                            fEvaluate = true;

                        /* Check our greater than operator. */
                        if(strOP.find(">") != strOP.npos && strValue > strCheck)
                            fEvaluate = true;

                        /* Break out if object failed filters. */
                        if(!fEvaluate)
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


    /* If the caller has requested a fieldname to filter on then this filters the response JSON to only include that field */
    void FilterResponse(const encoding::json& jParams, encoding::json &jResponse)
    {
        /* Check for fieldname filters. */
        if(jParams.find("fieldname") != jParams.end())
        {
            /* Grab our field string to rebuild response. */
            const std::string strField = jParams["fieldname"].get<std::string>();

            /* Copy over our new field. */
            const encoding::json jRet = { strField, jResponse[strField].get<std::string>() };
            jResponse      = jRet;
        }
    }
}
