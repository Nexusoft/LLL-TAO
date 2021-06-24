/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class Commands
     *
     *  Encapulsate all of our logic inside this commands class.
     *  All methods are static, so that we can manage our API instances.
     *
     **/
    class Commands
    {
        /** The different available API registries. **/
        static std::map<std::string, Base*> mapTypes;

    public:


        /** Base
         *
         *  Gets the base instance of commands, this contains standards and functions pointers, but not the actual functions.
         *
         *  @param[in] strAPI The API that we are getting base for.
         *
         *  @return a pointer to the base object indexed by commands name.
         *
         **/
        static Base* Get(const std::string& strAPI)
        {
            /* Check that set of commands exists. */
            if(!Commands::mapTypes.count(strAPI))
                return nullptr; //we don't throw here as this function won't be used directly in commands body, more of a helper

            return static_cast<Base*>(Commands::mapTypes[strAPI]->Get());
        }


        /** Get
         *
         *  Gets an instance of the API, indexed by our name.
         *
         **/
        template<typename Type>
        static Type* Get()
        {
            /* Grab a copy of our name. */
            const std::string strAPI = Type::Name();

            /* Check that this is in our map. */
            if(!Commands::mapTypes.count(strAPI))
                throw TAO::API::APIException(-4, "API Not Found: ", strAPI);

            return static_cast<Type*>(Commands::mapTypes[Type::Name()]->Get());
        }


        /** Register
         *
         *  Register an API instance, indexed by our name.
         *
         **/
        template<typename Type>
        static void Register()
        {
            /* Grab a copy of our name. */
            const std::string strAPI = Type::Name();
            if(Commands::mapTypes.count(strAPI))
                return; //we just exit if already registered

            /* Create and initialize our new type. */
            Commands::mapTypes[strAPI] = new Type();
            Commands::mapTypes[strAPI]->Initialize();
        }


        /** Invoke
         *
         *  Invoke a command by given API name, throw if none found.
         *
         *  @param[in] strAPI The API to invoke command in.
         *  @param[out] strMethod The method to invoke, can be modified during RewriteURL
         *  @param[out] jParams The parameters to pass to method, can be modified during RewriteURL
         *
         **/
        static encoding::json Invoke(const std::string& strAPI, std::string &strMethod, encoding::json &jParams)
        {
            /* Check that requested API is registered. */
            if(!Commands::mapTypes.count(strAPI))
                throw TAO::API::APIException(-4, "API Not Found: ", strAPI);

            return Commands::mapTypes[strAPI]->Execute(strMethod, jParams);
        }


        /** Status
         *
         *  Global helper function to access individual API by parameter for status messages.
         *
         *  @param[in] strAPI The current API we are checking status for.
         *  @param[in] strMethod The current method we are checking status for.
         *
         *  @return a string value representing the current status.
         *
         **/
        static std::string Status(const std::string& strAPI, const std::string& strMethod)
        {
            /* Check that requested API is registered. */
            if(!Commands::mapTypes.count(strAPI))
                throw TAO::API::APIException(-4, "API Not Found: ", strAPI);

            return Commands::mapTypes[strAPI]->Status(strMethod);
        }


        /** Shutdown
         *
         *  Shuts down and cleans up our command pointers.
         *
         **/
        static void Shutdown()
        {
            /* Loop through all entries and delete. */
            for(const auto& rAPI : Commands::mapTypes)
                delete rAPI.second;

            /* Clear the map now. */
            Commands::mapTypes.clear();
        }
    };
}
