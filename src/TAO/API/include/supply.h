/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_SUPPLY_H
#define NEXUS_TAO_API_INCLUDE_SUPPLY_H

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Supply
         *
         *  Supply API Class.
         *  Manages the function pointers for all Supply commands.
         *
         **/
        class Supply : public Base
        {
        public:

            /** Default Constructor. **/
            Supply() { Initialize(); }


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
                return "Supply";
            }


            /** GetItem
             *
             *  Gets the description of an item.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json GetItem(const json::json& params, bool fHelp);


            /** Transfer
             *
             *  Transfers an item.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Transfer(const json::json& params, bool fHelp);


            /** CreateItem
             *
             *  Creates an item.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json CreateItem(const json::json& params, bool fHelp);


            /** UpdateItem
             *
             *  Updates data to an item.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json UpdateItem(const json::json& params, bool fHelp);


            /** History
             *
             *  Gets the history of an item.
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
}

#endif
