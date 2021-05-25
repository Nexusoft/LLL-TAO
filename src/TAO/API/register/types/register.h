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
        void Initialize() final
        {
        }


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "register";
        }


        /** Create
         *
         *  Creates a register with given RAW state.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        json::json Create(const json::json& params, const bool fHelp);


        /** Read
         *
         *  Read a register's raw data
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        json::json Read(const json::json& params, const bool fHelp);


        /** Write
         *
         *  Writes a register's raw data
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        json::json Write(const json::json& params, const bool fHelp);


        /** Transfer
         *
         *  Transfers a register to destination.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        json::json Transfer(const json::json& params, const bool fHelp);


        /** History
         *
         *  Gets the history of a register's states
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        json::json History(const json::json& params, const bool fHelp);
    };
}
