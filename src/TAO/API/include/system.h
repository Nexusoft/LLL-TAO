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
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** System
         *
         *  System API Class.
         *  API provies access to system level data
         *
         **/
        class System : public Base
        {
        public:

            /** Default Constructor. **/
            System() { Initialize(); }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "System";
            }


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
            json::json GetInfo(const json::json& params, bool fHelp);


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
            json::json ListPeers(const json::json& params, bool fHelp);


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
            json::json LispEIDs(const json::json& params, bool fHelp);



        };
    }
}
