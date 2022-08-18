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
    /* Destructor. */
    Base::~Base()
    {
        mapFunctions.clear();
        mapStandards.clear();
        mapOperators.clear();
    }

    /* Get the current status of a given command. */
    std::string Base::Status(const std::string& strMethod) const
    {
        /* Check that the functions map contains status. */
        if(mapFunctions.find(strMethod) == mapFunctions.end())
            return debug::safe_printstr("Method not found: ", strMethod);

        return mapFunctions.at(strMethod).Status();
    }


    /* Checks an object's type if it has been standardized for this command-set. */
    bool Base::CheckObject(const std::string& strType, const TAO::Register::Object& tObject) const
    {
        /* Let's check against the types required now. */
        if(!mapStandards.count(strType))
            return false;

        /* Now let's check that the enum values match. */
        return mapStandards.at(strType).Check(tObject);
    }


    /* Encode a standard object into json using custom encoding function. */
    encoding::json Base::EncodeObject(const std::string& strType, const TAO::Register::Object& tObject, const uint256_t& hashRegister) const
    {
        /* Let's check against the types required now. */
        if(!mapStandards.count(strType))
            throw Exception(-12, "Malformed or missing object encoding for [", strType, "]");

        /* Now let's encoding by our encoding function. */
        return mapStandards.at(strType).Encode(tObject, hashRegister);
    }


    /* Handles the processing of the requested method. */
    encoding::json Base::Execute(std::string &strMethod, encoding::json &jParams, const bool fHelp)
    {
         /* If the incoming method is not in the function map then rewrite the URL to one that does */
        if(mapFunctions.find(strMethod) == mapFunctions.end())
            strMethod = RewriteURL(strMethod, jParams);

        /* Execute the function map if method is found. */
        if(mapFunctions.find(strMethod) != mapFunctions.end())
        {
            /* Get the result of command. */
            const encoding::json& jResults =
                mapFunctions[strMethod].Execute(jParams, fHelp);

            /* Check for operator. */
            if(CheckRequest(jParams, "operator", "string, array"))
            {
                /* Check for single string operator. */
                if(jParams["request"]["operator"].is_string())
                {
                    /* Grab our current operator. */
                    const std::string& strOperator =
                        jParams["request"]["operator"].get<std::string>();

                    /* Compute and return from our operator function. */
                    return mapOperators[strOperator].Execute(jParams, jResults);
                }

                /* Check if we are mapping multiple types. */
                if(jParams["request"]["operator"].is_array())
                {
                    /* Grab our current operator. */
                    const encoding::json& jOperators =
                        jParams["request"]["operator"];

                    /* Loop through our nouns now. */
                    encoding::json jResult = jResults; //interim results to chain through operators
                    for(auto& jOperator : jOperators)
                    {
                        /* Grab our current operator. */
                        const std::string& strOperator =
                            jOperator.get<std::string>();

                        jResult = mapOperators[strOperator].Execute(jParams, jResult);
                    }

                    return jResult;
                }
            }

            return jResults;
        }

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
            throw Exception(-14, "Malformed request URI: ", strMethod);

        /* Grab the components of this URL. */
        std::string strVerb = vMethods[URI::VERB];
        for(uint32_t n = URI::NOUN; n < vMethods.size(); ++n)
        {
            /* Handle different URL components. */
            switch(n)
            {
                /* Noun URI component. */
                case URI::NOUN:
                {
                    /* Check for function noun for fieldname filters on top of literal function maps. */
                    const std::string& strFunction = strVerb + "/" + vMethods[n];
                    if(mapFunctions.count(strFunction))
                    {
                        /* Set our returned verb. */
                        strVerb = strFunction;

                        continue; //go to next fieldname
                    }

                    /* Check that we have this method available. */
                    if(!mapFunctions.count(strVerb))
                        throw Exception(-2, "Method not found: ", strVerb);

                    /* Check if we are mapping multiple types. */
                    if(vMethods[n].find(",") != vMethods[n].npos)
                    {
                        /* Grab our components of the URL to rewrite. */
                        std::vector<std::string> vComponents;
                        ParseString(vMethods[n], ',', vComponents);

                        /* Build our request type as an array. */
                        jParams["request"]["type"] = encoding::json::array();

                        /* Loop through our nouns now. */
                        for(auto& strCheck : vComponents)
                        {
                            /* Grab our current noun. */
                            const std::string strNoun = ((strCheck.back() == 's' && (strVerb == "list" || strVerb == "user"))
                                ? strCheck.substr(0, strCheck.size() - 1)
                                : strCheck);  //we are taking out the last char if it happens to be an 's' as special for 'list' command

                            /* Check for unexpected types. */
                            if(!mapStandards.count(strNoun))
                                throw Exception(-36, "Unsupported type [", strNoun, "] for command");

                            /* Add our type to request object. */
                            jParams["request"]["type"].push_back(strNoun);
                        }

                        continue;
                    }

                    /* Grab our current noun. */
                    const std::string strNoun = ((vMethods[n].back() == 's' && strVerb == "list")
                        ? vMethods[n].substr(0, vMethods[n].size() - 1)
                        : vMethods[n]);  //we are taking out the last char if it happens to be an 's' as special for 'list' command

                    /* Handle our supported override. */
                    if(!mapFunctions[strVerb].Supported(strNoun))
                        throw Exception(-36, "Unsupported type [", strNoun, "] for command");

                    /* Check for unexpected types. */
                    if(!mapStandards.count(strNoun) && !mapFunctions[strVerb].Supported(strNoun))
                        throw Exception(-36, "Unsupported type [", strNoun, "] for command");

                    /* Add our type to request object. */
                    jParams["request"]["type"] = strNoun;

                    continue;
                }

                /* Field URI component. */
                case URI::FIELD:
                {
                    /* Check if we are mapping multiple types. */
                    if(vMethods[n].find(",") != vMethods[n].npos)
                    {
                        /* Grab our components of the URL to rewrite. */
                        std::vector<std::string> vFields;
                        ParseString(vMethods[n], ',', vFields);

                        /* Build our request type as an array. */
                        jParams["request"]["fieldname"] = encoding::json::array();

                        /* Loop through our nouns now. */
                        for(auto& strField : vFields)
                            jParams["request"]["fieldname"].push_back(strField);

                        continue;
                    }

                    /* Set our fieldname string. */
                    jParams["request"]["fieldname"] = vMethods[n];

                    continue;
                }

                /* Operator URI component. */
                case URI::OPERATOR:
                {
                    /* Grab our operator. */
                    const std::string& strOperators = vMethods[n];

                    /* Check if we are mapping multiple types. */
                    if(strOperators.find(",") != strOperators.npos)
                    {
                        /* Grab our components of the operator. */
                        std::vector<std::string> vOperators;
                        ParseString(strOperators, ',', vOperators);

                        /* Loop through compound operators. */
                        encoding::json jOperators = encoding::json::array();
                        for(auto& strOperator : vOperators)
                        {
                            /* Check if the operator is available. */
                            if(!mapOperators.count(strOperator))
                                throw Exception(-118, "[", strOperator, "] operator not supported for this command-set");

                            /* Add to a results array. */
                            jOperators.push_back(strOperator);
                        }

                        /* Add to input parameters. */
                        jParams["request"]["operator"] = jOperators;

                        continue;
                    }

                    /* Check if the operator is available. */
                    if(!mapOperators.count(strOperators))
                        throw Exception(-118, "[", strOperators, "] operator not supported for this command-set");

                    /* Add to input parameters. */
                    jParams["request"]["operator"] = strOperators;

                    continue;
                }

                /* Check for malformed URI. */
                default:
                    throw Exception(-14, "Malformed request URI at: ", vMethods[n], " | ", VARIABLE(n));

            }
        }

        return strVerb;
    }
}
