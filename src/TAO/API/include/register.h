/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_INCLUDE_REGISTER_H
#define NEXUS_TAO_API_INCLUDE_REGISTER_H

#include <TAO/API/types/base.h>

namespace TAO::API
{

    /** Register API Class
     *
     *  Lower level API to interact directly with registers.
     *
     **/
    class Register : public Base
    {
    public:

        /** Default Constructor. **/
        Register() { Initialize(); }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Get Name
         *
         *  Returns the name of this API.
         *
         **/
        std::string GetName() const final
        {
            return "Register";
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
        json::json Create(const json::json& params, bool fHelp);


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
        json::json Read(const json::json& params, bool fHelp);


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
        json::json Write(const json::json& params, bool fHelp);


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
        json::json Transfer(const json::json& params, bool fHelp);


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
        json::json History(const json::json& params, bool fHelp);
    };

    extern Supply supply;
}

#endif
