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
    class JSONAPIBase;

    /** Base class for all JSON based API methods**/
    class JSONAPIMethod
    {
        public:
            JSONAPIMethod(){};
            JSONAPIMethod(std::function<nlohmann::json(bool, nlohmann::json)> function){JSONAPIMethod::function = function;} 
            std::function<nlohmann::json(bool, nlohmann::json)> function;
            bool enabled;
    };

    /* Base class for all JSON based API's.*/
    class JSONAPIBase
    {
    public:
        JSONAPIBase() : INITIALIZED(false) { }

        virtual void Initialize() = 0; //abstract initializor that all derived API's must implement to register their specific APICommands

        /* Name of this API.  Derivations should set this */
        virtual std::string GetAPIName() const = 0;

        /* Handles the specific API method */
        virtual nlohmann::json HandleJSONAPIMethod(std::string METHOD, nlohmann::json parameters) = 0; 
    protected:
        bool INITIALIZED;

        /* The function objects for the API. */
        std::map<std::string, JSONAPIMethod > mapFunctions;
    };

    class JSONAPINode : public LLP::HTTPNode
    {
    public:
        JSONAPINode() : LLP::HTTPNode() {}
        JSONAPINode( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false ) : LLP::HTTPNode( SOCKET_IN, DDOS_IN, isDDOS ){}


        void RegisterAPI(JSONAPIBase* API)
        {
            /* wrap the API pointer in a unique_ptr so that we don't have to concern ourselves with cleanup*/
            JSONAPI_HANDLERS[API->GetAPIName()] = std::unique_ptr<JSONAPIBase>(API);
        }

    protected:
        std::map<std::string, std::unique_ptr<JSONAPIBase>> JSONAPI_HANDLERS; 

    };
}


#endif
