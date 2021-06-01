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
    /** DEX
     *
     *  DEX API Class.
     *  Manages the function pointers for all DEX exchanges
     *
     **/
    class Market : public Derived<Market>
    {
    public:

        /** Default Constructor. **/
        Market()
        : Derived<Market>()
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
            return "market";
        }


        /** Buy
         *
         *  Create a new BUY order on the Market
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Buy(const encoding::json& params, const bool fHelp);


        /** Sell
         *
         *  Create a new SELL order on the Market
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Sell(const encoding::json& params, const bool fHelp);

    };
}
