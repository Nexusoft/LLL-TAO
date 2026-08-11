/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

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

#include <chrono>
#include <condition_variable>
#include <cstdint>

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
         *  @param[in] pKnownLast Optional authoritative predecessor transaction for this
         *             sigchain, already selected into the block's vtx by AddTransactions().
         *             When supplied, this is used verbatim as the chaining predecessor
         *             instead of independently re-querying sessions/mempool/disk, which
         *             closes a TOCTOU race where a newer sigchain transaction submitted
         *             between AddTransactions() and CreateTransaction() could be picked
         *             as "last" even though it never made it into the block, producing a
         *             producer.hashPrevTx that disagrees with what Check() validates
         *             against (see FindProducerGenesisTxInVtx() in create.cpp).
         *
         **/
        bool CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx, const uint8_t nScheme = TAO::Ledger::SIGNATURE::BRAINPOOL,
                               const TAO::Ledger::Transaction* pKnownLast = nullptr);


        /** CreateProducer
         *
         *  Create a producer transaction object from signature chain.
         *
         *  Dual-identity mining: user signs the block, hashDynamicGenesis receives rewards.
         *
         *  @param[in] user The signature chain to generate this tx
         *  @param[in] pin The pin number to generate with.
         *  @param[out] tx The transaction object being created
         *  @param[in] tStateBest The current best block state
         *  @param[in] nBlockVersion The block version the producer is being created for
         *  @param[in] nChannel The channel to create block for.
         *  @param[in] nExtraNonce An extra nonce to use for double iterating.
         *  @param[in] pCoinbaseRecipients The coinbase recipients, if any.
         *  @param[in] hashDynamicGenesis Reward recipient genesis (0 = use user genesis)
         *  @param[in] pKnownLast Optional authoritative predecessor transaction for the
         *             producer's sigchain, forwarded to CreateTransaction(). See that
         *             function's documentation for why this closes a sequencing race.
         *
         **/
        bool CreateProducer(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                               TAO::Ledger::Transaction& tx,
                               const TAO::Ledger::BlockState& tStateBest,
                               const uint32_t nBlockVersion,
                               const uint32_t nChannel,
                               const uint64_t nExtraNonce,
                               Legacy::Coinbase *pCoinbaseRecipients = nullptr,
                               const uint256_t& hashDynamicGenesis = uint256_t(0),
                               const TAO::Ledger::Transaction* pKnownLast = nullptr);


        /** FindProducerGenesisTxInVtx
         *
         *  Scans a block's already-selected vtx entries for the transaction whose
         *  hashGenesis matches the given genesis, returning the entry with the
         *  highest nSequence (there should be at most one per AddTransactions()'
         *  per-genesis chaining, but scanning defensively guards against future
         *  changes to that ordering).
         *
         *  This is the authoritative source for producer chaining: it reflects
         *  exactly what Check() will later validate the producer against, so
         *  using it to seed CreateTransaction() eliminates the TOCTOU race where
         *  an independent, later mempool/disk query could disagree with vtx.
         *
         *  @param[in] block The block whose vtx has already been populated by AddTransactions().
         *  @param[in] hashGenesis The sigchain genesis to search for.
         *  @param[out] txOut The matching transaction, if found.
         *
         *  @return true if a matching transaction was found in vtx.
         *
         **/
        bool FindProducerGenesisTxInVtx(const TAO::Ledger::TritiumBlock& block,
                                        const uint256_t& hashGenesis, TAO::Ledger::Transaction& txOut);



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


        /** CachedMiningTemplateRequiresProducerFinalization
         *
         *  Return true when a cached mining block template cannot safely reuse the
         *  cached producer transaction.  Producer finalization is keyed by
         *  (tip, reward address): a different reward address must finalize a fresh
         *  producer and merkle root from the cached base template.
         *
         *  Extra nonce differences are intentionally ignored so prime-mod retry
         *  paths can reuse the expensive producer work for the same tip+reward.
         *
         **/
        inline bool CachedMiningTemplateRequiresProducerFinalization(
            const uint256_t& hashCachedDynamicGenesis,
            const uint256_t& hashRequestedDynamicGenesis,
            const uint64_t /*nCachedExtraNonce*/,
            const uint64_t /*nRequestedExtraNonce*/)
        {
            return hashCachedDynamicGenesis != hashRequestedDynamicGenesis;
        }


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
         *  Dual-identity mining: user signs the block, hashDynamicGenesis receives rewards.
         *
         *  @param[in] user The signature chain to generate this block
         *  @param[in] pin The pin number to generate with.
         *  @param[in] nChannel The channel to create block for.
         *  @param[out] block The block object being created.
         *  @param[in] nExtraNonce An extra nonce to use for double iterating.
         *  @param[in] pCoinbaseRecipients The coinbase recipients, if any.
         *  @param[in] hashDynamicGenesis Reward recipient genesis (0 = use user genesis)
         *  @param[out] pfTipRaceRetry If non-null, set to true when this call failed
         *              specifically because the chain tip advanced while the fresh
         *              template was being built/signed (a transient race, not a real
         *              error). Callers may use this to immediately retry against the
         *              new tip instead of surfacing a hard failure. Left untouched
         *              (caller should pre-initialize to false) on any other failure.
         *
         **/
        bool CreateBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& pin,
                         const uint32_t nChannel, TAO::Ledger::TritiumBlock& block, const uint64_t nExtraNonce = 0,
                         Legacy::Coinbase *pCoinbaseRecipients = nullptr,
                         const uint256_t& hashDynamicGenesis = uint256_t(0),
                         bool* pfTipRaceRetry = nullptr);


        /** ClearMiningTemplateCaches
         *
         *  Drops cached PRIME/HASH mining templates so subsequent miner work is
         *  rebuilt from the current best-chain hash.
         *
         **/
        void ClearMiningTemplateCaches(const char* pszReason = nullptr);


        /** InvalidateMiningTemplateCacheEntry
         *
         *  Evicts a single cached mining template keyed by (nChannel, hashDynamicGenesis),
         *  if present. Unlike ClearMiningTemplateCaches(), this does not disturb other
         *  miners' cached templates on the same channel. Used to force the next
         *  CreateBlock() call for this (channel, reward) pair to rebuild a fresh
         *  producer/merkle root, since a plain cache-hit reuse ignores nExtraNonce
         *  by design (see CachedMiningTemplateRequiresProducerFinalization).
         *
         *  @param[in] nChannel The mining channel (1=Prime, 2=Hash). No-op otherwise.
         *  @param[in] hashDynamicGenesis The reward address whose cached entry to evict.
         *
         **/
        void InvalidateMiningTemplateCacheEntry(const uint32_t nChannel, const uint256_t& hashDynamicGenesis);


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

#ifdef UNIT_TESTS
        namespace Testing
        {
            using SingleflightToken = std::uint64_t;

            SingleflightToken BeginOrJoinMiningTemplateInFlight(const uint32_t nChannel,
                                                                const uint256_t& hashDynamicGenesis,
                                                                bool& fIsOwner);

            bool WaitForMiningTemplateInFlight(const uint32_t nChannel,
                                               const SingleflightToken nToken,
                                               const std::chrono::milliseconds nTimeout,
                                               uint256_t& hashOut);

            void CompleteMiningTemplateInFlight(const SingleflightToken nToken,
                                                const uint32_t nChannel,
                                                const uint256_t& hashDynamicGenesis,
                                                const uint64_t nExtraNonce);

            void AbandonMiningTemplateInFlight(const SingleflightToken nToken,
                                               const uint32_t nChannel);

            void StoreMiningTemplateCacheEntryForTesting(const uint32_t nChannel,
                                                         const uint256_t& hashDynamicGenesis,
                                                         const uint64_t nExtraNonce);

            void ClearMiningTemplateCacheForTesting(const uint32_t nChannel);
            std::size_t MiningTemplateInFlightCountForTesting(const uint32_t nChannel);
        }
#endif
    }
}

#endif
