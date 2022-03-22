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
    /** System
     *
     *  System API Class.
     *  API provies access to system level data
     *
     **/
    class System : public Derived<System>
    {
    public:

        /** Default Constructor. **/
        System()
        : Derived<System>()
        {
            Initialize();
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
            return "system";
        }


        /** Stop
         *
         *  Stops the node and exits.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Stop(const encoding::json& params, const bool fHelp);


        /** GetInfo
         *
         *  Reurns a summary of node and ledger information for the currently running node
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetInfo(const encoding::json& params, const bool fHelp);


        /** GetInfo
         *
         *  Reurns information about the peers currently connected to this node
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json ListPeers(const encoding::json& params, const bool fHelp);


        /** LispEIDs
         *
         *  Queries the lisp api and returns the EID's for this node
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json LispEIDs(const encoding::json& params, const bool fHelp);


        /** Validate
         *
         *  Validates a register / legacy address
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Validate(const encoding::json& params, const bool fHelp);



        /** Metrics
         *
         *  Returns local database and other metrics
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Metrics(const encoding::json& params, const bool fHelp);

        /** VerifyPin
         *
         *  Returns boolean if supplied pin is the same as the active Pin 
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json VerifyPin(const encoding::json& params, const bool fHelp);


    private:

        /** count_registers
        *
        *  Returns the count of registers of the given type in the register DB
        *
        *  @param[in] strType the register type to count
        *
        *  @return number of unique registers in the register DB of the specified type.
        *
        **/
        uint64_t count_registers(const std::string& strType);

    };
}
