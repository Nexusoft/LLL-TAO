/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_APIBASE_H
#define NEXUS_TAO_API_TYPES_APIBASE_H

#include <LLP/include/http.h>
#include <Util/include/json.h>
#include <functional>
#include <memory>

namespace TAO::API
{
    /** Base class for all JSON based API methods
     *
     *  Encapsulates the function pointer to the method to process the API request
     *
     **/
    class JSONAPIMethod
    {
        public:
            JSONAPIMethod(){};
            JSONAPIMethod(std::function<nlohmann::json(bool, nlohmann::json)> function)
            {
                JSONAPIMethod::function = function;
            }
            
            std::function<nlohmann::json(bool, nlohmann::json)> function;
            bool fEnabled;
    };


    /** Base class for all JSON based API's.
     *
     *  Instances of JSONAPIBase derivations must be registered with the JSONAPINode processing the HTTP requests.
     *  This class holds a map of JSONAPIMethod instances that in turn perform the processing for each API method request.
     *
     **/
    class JSONAPIBase
    {
    public:
        JSONAPIBase() : fInitialized(false) { }


        /** Abstract initializor that all derived API's must implement to register their specific APICommands. **/
        virtual void Initialize() = 0;


        /** Name of this API.  Derivations should implement this and return an appropriate API name */
        virtual std::string GetAPIName() const = 0;


        /** HandleJSONAPIMethod
         *
         *  Handles the processing of the requested method.
         *  Derivations should implement this to lookup the requested method in the mapFunctions map and pass the processing on.
         *  Derivations should also form the response JSON according to the API specification
         *
         *  @param[in] strMethod The requested API method.
         *  @param[in] jsonParameters The parameters that the caller has passed to the API request.
         *
         *  @return JSON encoded response.
         *
         **/
        virtual nlohmann::json HandleJSONAPIMethod(std::string strMethod, nlohmann::json jsonParameters) = 0;

    protected:
        bool fInitialized;

        /* Map of method names to method function pointer objects for each method supported by this API. */
        std::map<std::string, JSONAPIMethod > mapFunctions;
    };


    class JSONAPINode : public LLP::HTTPNode
    {
    public:
        JSONAPINode() : LLP::HTTPNode() {}
        JSONAPINode( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false ) : LLP::HTTPNode( SOCKET_IN, DDOS_IN, isDDOS ){}


        /** RegisterAPI
        *
        *  Registers the API instance with this API Node by adding it to an internal map, keyed by the API instance's unique name.
        *  The calling code is not responsible for cleaning up the pAPI parameter as that is taken care of by this class.
        *
        *  @param[in] pAPI Pointer to the JSONAPIBase instance being registered.
        *
        **/
        void RegisterAPI(JSONAPIBase* pAPI)
        {
            /* wrap the API pointer in a unique_ptr so that we don't have to concern ourselves with cleanup*/
            mapJSONAPIHandlers[pAPI->GetAPIName()] = std::unique_ptr<JSONAPIBase>(pAPI);
        }



    protected:
        std::map<std::string, std::unique_ptr<JSONAPIBase>> mapJSONAPIHandlers;

    };
}


#endif
