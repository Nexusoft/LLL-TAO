/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** Market
     *
     *  Market API Class.
     *  Manages the function pointers for all P2P Marketplace Exchanges
     *
     **/
    class Market : public Derived<Market>
    {

        /** Handle for fee parameters. **/
        std::map<uint256_t, std::pair<uint256_t, uint64_t>> mapFees;


    public:

        /** Default Constructor. **/
        Market()
        : Derived<Market>()
        , mapFees        ()
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


        /* Generic handler for creating new indexes for this specific command-set. */
        void Index(const TAO::Operation::Contract& rContract, const uint32_t nContract) override;


        /** Create
         *
         *  Create an order on the market
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


        /** List
         *
         *  Lists an order for the given marketplace.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json List(const encoding::json& jParams, const bool fHelp);


        /** User
         *
         *  Lists a user's order for the given marketplace.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json User(const encoding::json& jParams, const bool fHelp);


        /** Execute
         *
         *  Executes an order on the market
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Execute(const encoding::json& jParams, const bool fHelp);


        /** Cancel
         *
         *  Cancels an order on the market
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Cancel(const encoding::json& jParams, const bool fHelp);


        /** OrderToJSON
         *
         *  Converts an order for marketplace into formatted JSON.
         *
         *  @param[in] rContract The contract to de-serialize
         *  @param[in] pairMarket The market pair ordering.
         *
         *  @return the formatted JSON object
         *
         **/
        __attribute__((pure)) encoding::json OrderToJSON(const TAO::Operation::Contract& rContract,
                                                         const std::pair<uint256_t, uint256_t>& pairMarket);


        /** OrderToJSON
         *
         *  Converts an order for marketplace into formatted JSON.
         *
         *  @param[in] rContract The contract to de-serialize
         *  @param[in] hashBase The market base pair for ordering.
         *
         *  @return the formatted JSON object
         *
         **/
        __attribute__((pure)) encoding::json OrderToJSON(const TAO::Operation::Contract& rContract, const uint256_t& hashBase);

    };
}
