/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_DEX_H
#define NEXUS_TAO_API_INCLUDE_DEX_H

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** DEX
         *
         *  DEX API Class.
         *  Manages the function pointers for all DEX exchanges
         *
         **/
        class DEX : public Base
        {
        public:

            /** Default Constructor. **/
            DEX()
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
                return "DEX";
            }


            /** Buy
             *
             *  Create a new BUY order on the DEX
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Buy(const json::json& params, bool fHelp);


            /** Sell
             *
             *  Create a new SELL order on the DEX
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Sell(const json::json& params, bool fHelp);

        };
    }
}

#endif
