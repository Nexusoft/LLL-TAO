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

        /** Voting
         *
         *  Voting API Class.
         *  Manages the function pointers for all Voting commands.
         *
         **/
        class Voting : public Base
        {
        public:

            /** Default Constructor. **/
            Voting()
            : Base()
            {
                Initialize();
            }


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
                return "Voting";
            }

            /** Count
             *
             *  Counts the number of votes (transactions) made to a given account.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Count(const json::json& params, bool fHelp);


            /** List
             *
             *  Returns a list of all votes made to an account
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json List(const json::json& params, bool fHelp);
        };
    }
}

