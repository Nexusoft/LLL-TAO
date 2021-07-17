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


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "ledger";
        }


        /** GetBlockHash
         *
         *  Retrieves the blockhash for the given height.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetBlockHash(const encoding::json& params, const bool fHelp);


        /** GetBlock
         *
         *  Retrieves the block data for a given hash or height.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetBlock(const encoding::json& params, const bool fHelp);


        /** ListBlocks
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
        encoding::json ListBlocks(const encoding::json& params, const bool fHelp);


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
        encoding::json GetTransaction(const encoding::json& params, const bool fHelp);


        /** SubmitTransaction
         *
         *  Submits a raw transaction to the network
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json SubmitTransaction(const encoding::json& params, const bool fHelp);


        /** GetMiningInfo
         *
         *  Returns an object containing mining-related information
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetMiningInfo(const encoding::json& params, const bool fHelp);


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
