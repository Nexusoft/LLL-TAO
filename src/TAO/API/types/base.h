/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/function.h>
#include <TAO/API/types/exception.h>

/* Global TAO namespace. */
namespace TAO::API
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
        Base ( )
        : fInitialized (false)
        , mapFunctions ( )
        , mapStandards ( )
        {
        }


        /** Default destructor **/
        virtual ~Base()
        {
            mapFunctions.clear();
            mapStandards.clear();
        }


        /** Initialize
         *
         *  Abstract initializor that all derived API's must implement to
         *  register their specific APICommands.
         *
         **/
        virtual void Initialize() = 0;


        /** Get
         *
         *  Abstract initializer so we don't need to copy this method for each derived class.
         *
         **/
        virtual Base* Get() = 0;


        /** Execute
         *
         *  Handles the processing of the requested method.
         *  Derivations should implement this to lookup the requested method in the mapFunctions map and pass the processing on.
         *  Derivations should also form the response JSON according to the API specification
         *
         *  @param[out] strMethod The requested API method.
         *  @param[out] jParams The parameters that the caller has passed to the API request.
         *  @param[in] fHelp Flag to determine if command help is requested.
         *
         *  @return JSON encoded response.
         *
         **/
        json::json Execute(std::string &strMethod, json::json &jParams, bool fHelp = false);


        /** RewriteURL
         *
         *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jParams
         *  The return json contains the modifed method URL to be called.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jParams The json array of parameters being passed to this method.
         *
         *  @return the API method URL
         *
         **/
        virtual std::string RewriteURL(const std::string& strMethod, json::json &jParams);



        /** SanitizeParams
         *
         *  Allows derived API's to check the values in the parameters array for the method being called.
         *  The return json contains the sanitized parameter values, which derived implementations might convert to the correct type
         *  for the method being called.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jParams The json array of parameters being passed to this method.
         *
         *  @return the sanitized json parameters array.
         *
         **/
        virtual json::json SanitizeParams(const std::string& strMethod, const json::json& jParams);

    protected:

        /** Initializer Flag. */
        bool fInitialized;


        /** Map of method names to method function pointer objects for each method supported by this API. **/
        std::map<std::string, Function> mapFunctions;


        /** Map of standard nouns to check for standard object types. **/
        std::map<std::string, uint8_t>  mapStandards;
    };


    /** Derived
     *
     *  Class for forcing the polymorphic get so we can cast to and from child and parent classes.
     *  This follows similar logic as polymorphic clone, but is indented to be used without copying.
     *
     **/
    template<class Type>
    class Derived : public Base
    {
    public:

        /** Get
         *
         *  Method to be used by commands class, to allow casting to and from parent and child classes.
         *
         **/
        Base* Get() final override
        {
            return static_cast<Type*>(this);
        }
    };
}
