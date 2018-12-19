/*__________________________________________________________________________________________
            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
            (c) Copyright The Nexus Developers 2014 - 2018
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_INCLUDE_SUPPLY_H
#define NEXUS_TAO_API_INCLUDE_SUPPLY_H

#include <TAO/API/types/base.h>

namespace TAO::API
{

    /** Supply API Class
     *
     *  Manages the function pointers for all Supply commands.
     *
     **/
    class Supply : public Base
    {
    public:
        Supply() {}

        void Initialize();

        std::string GetName() const
        {
            return "Supply";
        }

        /** Get Item
         *
         *  Get's the description of an item.
         *
         **/
        json::json GetItem(const json::json& jsonParams, bool fHelp);


        /** Transfer
         *
         *  Transfers an item.
         *
         **/
        json::json Transfer(const json::json& jsonParams, bool fHelp);


        /** Submit
         *
         *  Submits an item.
         *
         **/
        json::json Submit(const json::json& jsonParams, bool fHelp);


        /** History
         *
         *  Gets the history of an item.
         *
         **/
        json::json History(const json::json& jsonParams, bool fHelp);
    };

    extern Supply supply;
}

#endif
