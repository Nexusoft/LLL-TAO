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

#include <TAO/Register/types/object.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the current status of a given command. */
    std::string Base::Status(const std::string& strMethod) const
    {
        /* Check that the functions map contains status. */
        if(mapFunctions.find(strMethod) == mapFunctions.end())
            return debug::safe_printstr("Method not found: ", strMethod);

        return mapFunctions.at(strMethod).Status();
    }


    /* Checks an object's type if it has been standardized for this command-set. */
    bool Base::CheckObject(const std::string& strType, const TAO::Register::Object& objCheck) const
    {
        /* Let's check against the types required now. */
        if(!mapStandards.count(strType))
            return false;

        /* Now let's check that the enum values match. */
        return mapStandards.at(strType).Check(objCheck);
    }


    /* Handles the processing of the requested method. */
    encoding::json Base::Execute(std::string &strMethod, encoding::json &jParams, const bool fHelp)
    {
         /* If the incoming method is not in the function map then rewrite the URL to one that does */
        if(mapFunctions.find(strMethod) == mapFunctions.end())
            strMethod = RewriteURL(strMethod, jParams);

        /* Execute the function map if method is found. */
        if(mapFunctions.find(strMethod) != mapFunctions.end())
            return mapFunctions[strMethod].Execute(jParams, fHelp);
        else
            throw Exception(-2, "Method not found: ", strMethod);
    }


    /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not map directly to a function */
    std::string Base::RewriteURL(const std::string& strMethod, encoding::json &jParams)
    {
        /* Grab our components of the URL to rewrite. */
        std::vector<std::string> vMethods;
        ParseString(strMethod, '/', vMethods);

        /* Check that we have all of our values. */
        if(vMethods.empty()) //we are ignoring everything past first noun on rewrite
            throw Exception(-14, "Malformed request URL: ", strMethod);

        /* Detect whether fieldname or a resolved name. */
        const bool fAddress =
            (jParams.find("address") != jParams.end() || jParams.find("name") != jParams.end());

        /* Grab the components of this URL. */
        const std::string& strVerb = vMethods[0];
        for(uint32_t n = 1; n < vMethods.size(); ++n)
        {
            /* Grab our current noun. */
            const std::string strNoun = ((vMethods[n].back() == 's' && strVerb == "list")
                ? vMethods[n].substr(0, vMethods[n].size() - 1)
                : vMethods[n]);  //we are taking out the last char if it happens to be an 's' as special for 'list' command

            /* Now lets do some rules for the different nouns. */
            if(n == 1)
            {
                /* Check for unexpected types. */
                if(!mapStandards.count(strNoun))
                    throw Exception(-36, "Invalid type [", strNoun, "] for command");

                /* Add our type to request object. */
                jParams["request"]["type"] = strNoun;

                continue;
            }

            /* If we have reached here, we know we are an address or name. */
            if(n == 2 && !fAddress)
            {
                /* Check if this value is an address. */
                if(CheckAddress(strNoun))
                    jParams["address"] = strNoun;

                /* If not address it must be a name. */
                else
                    jParams["name"] = strNoun;

                continue;
            }

            /* If we have reached here, we know we are a fieldname. */
            if(n >= 2 && fAddress)
            {
                jParams["fieldname"] = strNoun;

                continue;
            }

            /* If we get here, we need to throw for malformed URL. */
            else
                throw Exception(-14, "Malformed request URL at: ", strNoun);
        }

        return strVerb;
    }
}
