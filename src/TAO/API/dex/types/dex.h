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

        /** DEXOrder
         *
         *  Encapsulates a DEX sell order, i.e. a conditional DEBIT / TRANSFER made to a wildcard address.
         *  The order is always considered to be a sell order since the contract is always a debit/transfer for a specific token
         *  or asset, with a condition to receive something in return.  However when viewing an orderbook, a DEXOrder may be a buy
         *  or sell order, depending on which way around the market pair is being viewed. For example if the market pair is NXS/ABC
         *  then a SELL order would be a debit of NXS in exchange for ABC, whereas a BUY order would be a debit of ABC in exchange
         *  for NXS.  Therefore to compile a complete NXS/ABC orderbook you would need to read from the DEXDb all NXS/ABC orders 
         *  for the sells and all ABC/NXS orders for the buys.   
         *
         **/
        class DEXOrder
        {
        public:

            /** The transaction ID containing this order **/
            uint512_t hashTx;

            /** The contract ID within the transaction for this order **/
            uint32_t nContract;

            /** The register address of the token or register being sold **/
            TAO::Register::Address hashSell;

            /** The quantity of tokens being sold.  If hashSell is a register instead of a token, this will be set to 1 **/
            uint64_t nSellAmount;

            /** The register address of the token being bought in exchange **/
            TAO::Register::Address hashBuy;

            /** The quantity of tokens being bought in exchange.  If hashBuy is a register instead of a token, this will be set to 1 **/
            uint64_t nBuyAmount;

        };

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


            /** IsDEX
             *
             *  Utility method to determine whether a contract is a DEX contract.  This is identified by the operation being a 
             *  conditional DEBIT or TRANSFER being made to a wildcard address  
             *
             *  @param[in] contract The contract to check.
             *
             *  @return True if the contract is a DEX contract, otherwise false.
             *
             **/
            static bool IsDEX(const TAO::Operation::Contract& contract);
            
            
            /** GetMarketID
             *
             *  Utility method to generate a unique identifier for the market pair.  This is used for the batch key for storing
             *  DEX orders in the DEX database, which requires a string for the key.  
             *
             *  @param[in] hashSell The register address of the token or register being sold.
             *  @param[in] hashBuy The register address of the token being bought in exchange.
             *
             *  @return A base58 encoded string represeting the market identifier.
             *
             **/
            static std::string GetMarketID(const TAO::Register::Address& hashSell, const TAO::Register::Address hashBuy);

        };
    }
}

#endif
