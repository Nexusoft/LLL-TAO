/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_BASE_H
#define NEXUS_TAO_API_TYPES_BASE_H

#include <TAO/API/types/function.h>
#include <TAO/API/types/exception.h>
#include <Util/include/debug.h>

namespace TAO::API
{

    /** Base class for all JSON based API's.
     *
     *  Instances of JSONAPIBase derivations must be registered with the JSONAPINode processing the HTTP requests.
     *  This class holds a map of JSONAPIMethod instances that in turn perform the processing for each API method request.
     *
     **/
    class Base
    {
    public:
        Base() : fInitialized(false) { }


        /** Abstract initializor that all derived API's must implement to register their specific APICommands. **/
        virtual void Initialize() = 0;


        /** Name of this API.  Derivations should implement this and return an appropriate API name */
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
            if(mapFunctions.find(strMethod) != mapFunctions.end())
                return mapFunctions[strMethod].Execute(jsonParams, fHelp);
            else
                throw APIException(-32601, debug::strprintf("Method not found: %s", strMethod.c_str()));
        }

    protected:

        /** Initializer Flag. */
        bool fInitialized;


        /** Map of method names to method function pointer objects for each method supported by this API. **/
        std::map<std::string, Function > mapFunctions;
    };
}


#endif
