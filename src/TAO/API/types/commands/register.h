/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>
#include <TAO/API/types/commands.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Register
     *
     *  Register API Class.
     *  Lower level API to interact directly with registers.
     *
     **/
    class Register : public Derived<Register>
    {
    public:

        /** Default Constructor. **/
        Register()
        : Derived<Register>()
        {
        }


        /** Default Destructor. **/
        virtual ~Register()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "register";
        }


        /** Import
         *
         *  Import objects of an API instance, indexed by our name.
         *
         **/
        template<typename Type>
        void Import()
        {
            /* Get our typename from static type instance. */
            const std::string& strAPI = Type::Name();

            /* Grab objects by this class's instance converted by typename. */
            Import(strAPI);
        }


        /** Import
         *
         *  Import objects of an API instance, indexed by our name.
         *
         *  @param[in] strAPI The api instance string to use.
         *
         **/
        void Import(const std::string& strAPI);


        /** Get
         *
         *  Get any register in the global scope
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Get(const encoding::json& jParams, const bool fHelp);


        /** List
         *
         *  Lists all registers of supported type in global scope.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json List(const encoding::json& jParams, const bool fHelp);


        /** History
         *
         *  Gets a history of register states over the course of its lifetime.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *
         *  @return the json list of given object(s).
         *
         **/
        encoding::json History(const encoding::json& jParams, const bool fHelp);


        /** Transactions
         *
         *  Lists all transactions of supported type in any scope.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Transactions(const encoding::json& jParams, const bool fHelp);
    };
}
