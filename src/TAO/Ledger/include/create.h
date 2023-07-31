/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_CREATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CREATE_H

#include <Legacy/types/coinbase.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/credentials.h>

#include <Util/include/allocators.h>

#include <condition_variable>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Condition variable for private blocks. */
        extern std::condition_variable PRIVATE_CONDITION;


        /** CreateTransaction
         *
         *  Create a new transaction object from signature chain.
         *
         *  @param[in] user The signature chain to generate this tx
         *  @param[in] pin The pin number to generate with.
         *  @param[out] tx The transaction object being created
         *  @param[in] nScheme The key scheme to be used.
         *
         **/
        bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx, const uint8_t nScheme = TAO::Ledger::SIGNATURE::BRAINPOOL);


        /** CreateProducer
         *
         *  Create a producer transaction object from signature chain.
         *
         *  @param[in] user The signature chain to generate this tx
         *  @param[in] pin The pin number to generate with.
         *  @param[out] tx The transaction object being created
         *  @param[in] tStateBest The current best block state
         *  @param[in] nBlockVersion The block version the producer is being created for
         *  @param[in] nChannel The channel to create block for.
         *  @param[in] nExtraNonce An extra nonce to use for double iterating.
         *  @param[in] pCoinbaseRecipients The coinbase recipients, if any.
         *
         **/
        bool CreateProducer(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx,
                               const TAO::Ledger::BlockState& tStateBest,
                               const uint32_t nBlockVersion,
                               const uint32_t nChannel,
                               const uint64_t nExtraNonce,
                               Legacy::Coinbase *pCoinbaseRecipients = nullptr);


        /** AddTransactions
         *
         *  Gets a list of transactions from memory pool for current block.
         *
         *  @param[out] block The block to add the transactions to.
         *
         **/
        void AddTransactions(TAO::Ledger::TritiumBlock& block);


        /** AddBlockData
         *
         *  Populate block header data for a new block.
         *
         *  @param[in] tStateBest the current best state of the chain at the time of block creation
         *  @param[in] nChannel The channel creating the block.
         *  @param[out] block The block object being created.
         *
         **/
        void AddBlockData(const TAO::Ledger::BlockState& tStateBest, const uint32_t nChannel, TAO::Ledger::TritiumBlock& block);


        /** CreateBlock
         *
         *  Create a new block object from the chain.
         *
         *  This method does not create stake blocks. Channel 0 (Proof of Stake) generates invalid channel.
         *  Only use for Coinbase (channel 1 or 2) or private (channel 3) producer.
         *
         *  When called for Coinbase or private blocks, this method completes all block setup, including creating the block
         *  producer with producer operations and adding transactions to the block.
         *
         *  @param[in] user The signature chain to generate this block
         *  @param[in] pin The pin number to generate with.
         *  @param[in] nChannel The channel to create block for.
         *  @param[out] block The block object being created.
         *  @param[in] nExtraNonce An extra nonce to use for double iterating.
         *  @param[in] pCoinbaseRecipients The coinbase recipients, if any.
         *
         **/
        bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                         const uint32_t nChannel, TAO::Ledger::TritiumBlock& block, const uint64_t nExtraNonce = 0,
                         Legacy::Coinbase *pCoinbaseRecipients = nullptr);


        /** CreateStakeBlock
         *
         *  Create a new Proof of Stake (channel 0) block object from the chain.
         *
         *  For Proof of Stake, the create block process sets up all the block basics, adds transaction, and creates
         *  the producer. It does not complete the producer operations, though. The stake minter must determine
         *  operation data and complete producer setup, then also calculate the block hashMerkleRoot from completed data.
         *
         *  @param[in] user The signature chain to generate this block
         *  @param[in] pin The pin number to generate with.
         *  @param[out] block The block object being created.
         *  @param[in] fGenesis Set true if staking for Genesis, false if staking for Trust
         *
         **/
        bool CreateStakeBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                              TAO::Ledger::TritiumBlock& block, const bool fGenesis = false);


        /** CreateGenesis
         *
         *  Creates the genesis block
         *
         **/
        bool CreateGenesis();


        /** ThreadGenerator
         *
         *  Handles the creation of a private block chain.
         *  Only executes when a transaction is broadcast.
         *
         **/
        void ThreadGenerator();


        /** UpdateProducerTimestamp
         *
         *  Updates the producer timestamp, making sure it is not earlier than the previous block.
         *
         *  @param[out] txProducer The producer transaction to have its timestamp updated.
         *
         **/
        void UpdateProducerTimestamp(TAO::Ledger::Transaction& txProducer);


        /** UpdateProducerTimestamp
         *
         *  Updates the producer timestamp, making sure it is not earlier than the previous block.
         *
         *  For v9+ this only updates the block finding producer (last producer).
         *
         *  @param[out] block The block to have its producer timestamp updated.
         *
         **/
        void UpdateProducerTimestamp(TAO::Ledger::TritiumBlock& block);
    }
}

#endif
