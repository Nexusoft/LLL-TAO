/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/types/base.h>
#include <TAO/API/types/exception.h>

#include <TAO/API/include/check.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Checks an object's standard if it has been standardized for this command-set. */
    bool Base::Standard(const std::string& strType, const uint8_t nStandard) const
    {
        /* Let's check against the types required now. */
        if(!mapStandards.count(strType))
            return false;

        /* Now let's check that the enum values match. */
        if(mapStandards.at(strType) != nStandard)
            return false;

        return true;
    }


    /* Handles the processing of the requested method. */
    encoding::json Base::Execute(std::string &strMethod, encoding::json &jParams, const bool fHelp)
    {
         /* If the incoming method is not in the function map then rewrite the URL to one that does */
        if(mapFunctions.find(strMethod) == mapFunctions.end())
            strMethod = RewriteURL(strMethod, jParams);

        /* Execute the function map if method is found. */
        if(mapFunctions.find(strMethod) != mapFunctions.end())
            return mapFunctions[strMethod].Execute(SanitizeParams(strMethod, jParams), fHelp);
        else
            throw APIException(-2, FUNCTION, "Method not found: ", strMethod);
    }


    /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not map directly to a function */
    std::string Base::RewriteURL(const std::string& strMethod, encoding::json &jParams)
    {
        /* Grab our components of the URL to rewrite. */
        std::vector<std::string> vMethods;
        ParseString(strMethod, '/', vMethods);

        /* Check that we have all of our values. */
        if(vMethods.empty()) //we are ignoring everything past first noun on rewrite
            throw APIException(-14, FUNCTION, "Malformed request URL: ", strMethod);

        /* Track if we've found an explicet type. */
        bool fStandard = false, fAddress = false, fField = false;;

        /* Grab the components of this URL. */
        const std::string& strVerb = vMethods[0];
        for(uint32_t n = 1; n < vMethods.size(); ++n)
        {
            /* Grab our current noun. */
            const std::string strNoun = ((vMethods[n].back() == 's' && strVerb == "list")
                ? vMethods[n].substr(0, vMethods[n].size() - 1)
                : vMethods[n]);  //we are taking out the last char if it happens to be an 's' as special for 'list' command

            /* Now lets do some rules for the different nouns. */
            if(!fStandard && mapStandards.count(strNoun))
            {
                jParams["request"]["type"] = strNoun;

                /* Set our explicet flag now. */
                fStandard = true;
                fAddress  = true; //we set this here so that first noun slot can't be either/or, it breaks our fieldname

                continue;
            }

            /* If we have reached here, we know we are an address or name. */
            else if(!fAddress)
            {
                /* Check if this value is an address. */
                if(CheckAddress(strNoun))
                    jParams["address"] = strNoun;

                /* If not address it must be a name. */
                else
                    jParams["name"] = strNoun;

                /* Set both address and noun to handle ommiting the noun if desired. */
                fAddress  = true;
                fStandard = true;

                continue;
            }

            /* If we have reached here, we know we are a fieldname. */
            else if(!fField)
            {
                jParams["fieldname"] = strNoun;

                fField = true;
                continue;
            }

            /* If we get here, we need to throw for malformed URL. */
            else
                throw APIException(-14, FUNCTION, "Malformed request URL: ", strMethod);
        }

        return strVerb;
    }



    /* Allows derived API's to check the values in the parameters array for the method being called. */
    encoding::json Base::SanitizeParams(const std::string& strMethod, const encoding::json& jParams)
    {
        /* Some 3rd party apps such as bubble do not handle dynamic values very well
         * and will insert the text null if not populated */
        if(config::GetBoolArg("-apiremovenullstring", false))
        {
            /* Make a copy of the params to parse */
            encoding::json jsonSanitizedParams = jParams;

            /* Iterate all parameters */
            for(auto param = jParams.begin(); param != jParams.end(); ++param)
            {
                if((*param).is_string() && (*param).get<std::string>() == "null")
                {
                    jsonSanitizedParams[param.key()] = nullptr;
                }
            }

            return jsonSanitizedParams;

        }
        else
            return jParams;
    }
}
