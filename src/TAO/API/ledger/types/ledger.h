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
    /** Ledger
     *
     *  Ledger API Class.
     *  Lower level API to interact directly with registers.
     *
     **/
    class Ledger : public Derived<Ledger>
    {
    public:

        /** Default Constructor. **/
        Ledger()
        : Derived<Ledger>()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** RewriteURL
         *
         *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jsonParams
         *  The return json contains the modifed method URL to be called.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jsonParams The json array of parameters being passed to this method.
         *
         *  @return the API method URL
         *
         **/
        std::string RewriteURL(const std::string& strMethod, encoding::json& jsonParams) override;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "ledger";
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
        encoding::json Create(const encoding::json& params, const bool fHelp);


        /** BlockHash
         *
         *  Retrieves the blockhash for the given height.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json BlockHash(const encoding::json& params, const bool fHelp);


        /** Block
         *
         *  Retrieves the block data for a given hash or height.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Block(const encoding::json& params, const bool fHelp);


        /** Blocks
         *
         *  Retrieves the block data for a sequential range of blocks
         *  starting at a given hash or height.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Blocks(const encoding::json& params, const bool fHelp);


        /** Transaction
         *
         *  Retrieves the transaction data for a given hash.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Transaction(const encoding::json& params, const bool fHelp);


        /** Submit
         *
         *  Submits a raw transaction to the network
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Submit(const encoding::json& params, const bool fHelp);


        /** MiningInfo
         *
         *  Returns an object containing mining-related information
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json MiningInfo(const encoding::json& params, const bool fHelp);


        /** VoidTransaction
         *
         *  Reverses a debit or transfer transaction that the caller has made
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json VoidTransaction(const encoding::json& params, const bool fHelp);


        /** SyncSigChain
         *
         *  Synchronizes the signature chain for the currently logged in user.  Only applicable in lite / client mode
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json SyncSigChain(const encoding::json& params, const bool fHelp);


        /** SyncHeaders
         *
         *  Synchronizes the block header data from a peer. NOTE: the method call will return as soon as the synchronization
         *  process is initiated with a peer, NOT when synchronization is complete.  Only applicable in lite / client mode
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json SyncHeaders(const encoding::json& params, const bool fHelp);


    };
}
