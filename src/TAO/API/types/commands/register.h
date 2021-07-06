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


        /** Transactions
         *
         *  Lists all transactions of supported type in global scope.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Transactions(const encoding::json& jParams, const bool fHelp);


        /** AccountToJSON
         *
         *  Returns the JSON representation of an account, either trust or account
         *
         *  @param[in] rObject The state register containing the invoice data
         *  @param[in] hashRegister The register address of the invoice state register
         *
         *  @return the invoice JSON
         *
         **/
        static encoding::json AccountToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister);
    };
}
