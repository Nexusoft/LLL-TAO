/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_TYPES_BASE_H
#define NEXUS_TAO_API_TYPES_BASE_H

#include <TAO/API/types/function.h>
#include <TAO/API/types/exception.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Base
         *
         *  Base class for all JSON based API's.
         *
         *  Instances of JSONAPIBase derivations must be registered with the
         *  JSONAPINode processing the HTTP requests. This class holds a map of
         *  JSONAPIMethod instances that in turn perform the processing for each
         *  API method request.
         *
         **/
        class Base
        {
        public:


            /** Default Constructor **/
            Base()
            : fInitialized(false)
            , mapFunctions()
            {
            }


            /** Default destructor **/
            virtual ~Base()
            {
                mapFunctions.clear();
            }


            /** Initialize
             *
             *  Abstract initializor that all derived API's must implement to
             *  register their specific APICommands.
             *
             **/
            virtual void Initialize() = 0;


            /** GetName
             *
             *  Name of this API.  Derivations should implement this and return
             *  an appropriate API name
             *
             **/
            virtual std::string GetName() const = 0;


            /** Execute
             *
             *  Handles the processing of the requested method.
             *  Derivations should implement this to lookup the requested method in the mapFunctions map and pass the processing on.
             *  Derivations should also form the response JSON according to the API specification
             *
             *  @param[in] strMethod The requested API method.
             *  @param[in] jsonParameters The parameters that the caller has passed to the API request.
             *  @param[in] fHelp Flag to determine if command help is requested.
             *
             *  @return JSON encoded response.
             *
             **/
            json::json Execute(const std::string& strMethod, const json::json& jsonParams, bool fHelp = false)
            {
                //XXX: WFT is this, everything paul did is completely fucked
                //to fix this, we need to use API/method as our key for mapFunctions, and derive the type
                //then pass this in as a parameter into the API's. RewriteURL needs some rethinking too
                //so that we can handle type as a name, not the idiotic API/method/type/name that currently is implemented
                //everything paul did is inconsistent, buggy, and redundant copy and paste code

                json::json jsonParamsUpdated = jsonParams;
                std::string strMethodToCall = strMethod;

                 /* If the incoming method is not in the function map then
                   give derived API's the opportunity to rewrite the URL to one that does*/
                if(mapFunctions.find(strMethodToCall) == mapFunctions.end())
                    strMethodToCall = RewriteURL(strMethod, jsonParamsUpdated);

                if(mapFunctions.find(strMethodToCall) != mapFunctions.end())
                    return mapFunctions[strMethodToCall].Execute(SanitizeParams(strMethodToCall, jsonParamsUpdated), fHelp);
                else
                    throw APIException(-2, debug::safe_printstr("Method not found: ", strMethodToCall));
            }


            /** RewriteURL
             *
             *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
             *  map directly to a function in the target API.  Insted this method can be overriden to
             *  parse the incoming URL and route to a different/generic method handler, adding parameter
             *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
             *  added to the jsonParams
             *  The return json contains the modifed method URL to be called.
             *
             *  @param[in] strMethod The name of the method being invoked.
             *  @param[in] jsonParams The json array of parameters being passed to this method.
             *
             *  @return the API method URL
             *
             **/
            virtual std::string RewriteURL(const std::string& strMethod, json::json& jsonParams)
            {
                return strMethod;
            };



            /** SanitizeParams
             *
             *  Allows derived API's to check the values in the parameters array for the method being called.
             *  The return json contains the sanitized parameter values, which derived implementations might convert to the correct type
             *  for the method being called.
             *
             *  @param[in] strMethod The name of the method being invoked.
             *  @param[in] jsonParams The json array of parameters being passed to this method.
             *
             *  @return the sanitized json parameters array.
             *
             **/
            virtual json::json SanitizeParams(const std::string& strMethod, const json::json& jsonParams)
            {
                /* If configured, remove any string populated with the text null. Some 3rd party apps such as
                   bubble do not handle dynamic values very well and will insert the text null if not populated
                   so this helper will remove them (set them to json::null) */
                if(config::GetBoolArg("-apiremovenullstring", false))
                {
                    /* Make a copy of the params to parse */
                    json::json jsonSanitizedParams = jsonParams;

                    /* Iterate all parameters */
                    for(auto param = jsonParams.begin(); param != jsonParams.end(); ++param)
                    {
                        if((*param).is_string() && (*param).get<std::string>() == "null")
                        {
                            jsonSanitizedParams[param.key()] = nullptr;
                        }
                    }

                    return jsonSanitizedParams;

                }
                else
                    return jsonParams;
            };

        protected:

            /** Initializer Flag. */
            bool fInitialized;


            /** Map of method names to method function pointer objects for each method supported by this API. **/
            std::map<std::string, Function > mapFunctions;
        };
    }
}


#endif
