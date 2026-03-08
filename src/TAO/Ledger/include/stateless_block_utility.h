/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H
#define NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H

#include <TAO/Ledger/types/tritium.h>

#include <string>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /** SubmitResult
         *
         *  Structured result for mined block submissions.
         *
         **/
        struct SubmitResult
        {
            bool accepted = false;
            std::string reason;
            uint32_t nChannel = 0;
            uint32_t nHeight = 0;
            uint32_t nUnifiedHeight = 0;  // Unified blockchain height (reference)
            uint512_t hashBlock = 0; // Merkle root (submission key).
        };


        /** BlockValidationResult
         *
         *  Result of validating a mined block before acceptance.
         *
         **/
        struct BlockValidationResult
        {
            bool valid = false;
            std::string reason;
            uint32_t nChannel = 0;
            uint32_t nHeight = 0;
            uint32_t nUnifiedHeight = 0;  // Unified blockchain height (reference)
            uint512_t hashBlock = 0; // Merkle root (submission key).
        };


        /** BlockAcceptanceResult
         *
         *  Result of accepting a mined block into the ledger.
         *
         **/
        struct BlockAcceptanceResult
        {
            bool accepted = false;
            std::string reason;
            uint32_t nChannel = 0;
            uint32_t nHeight = 0;
            uint32_t nUnifiedHeight = 0;  // Unified blockchain height (reference)
            uint512_t hashBlock = 0; // Merkle root (submission key).
            uint8_t status = 0;
        };


        /** ParseResult
         *
         *  Result of parsing a stateless miner work submission.
         *
         **/
        struct ParseResult
        {
            bool success = false;
            std::string reason;
            uint512_t hashMerkle = 0;
            uint64_t nonce = 0;
            uint64_t timestamp = 0; // Set when Falcon wrapper includes timestamp; legacy payloads remain 0.
            uint32_t nUnifiedHeight = 0; ///< Canonical unified height from the block payload (0 if absent)
            uint32_t nChannelHeight = 0; ///< Channel height from the block payload (0 if absent)
        };


        /** FalconWrappedSubmitBlockParseResult
         *
         *  Result of parsing a Falcon-wrapped full-block SUBMIT_BLOCK payload.
         *
         *  Payload grammar is channel-aware and shared across both legacy and
         *  stateless mining lanes:
         *
         *  Hash:  [block(216)][timestamp(8 LE)][sig_len(2 LE)][signature]
         *  Prime: [block(216)][vOffsets(N)][timestamp(8 LE)][sig_len(2 LE)][signature]
         *
         **/
        struct FalconWrappedSubmitBlockParseResult
        {
            bool success = false;
            std::string reason;
            std::vector<uint8_t> vBlockBytes;
            std::vector<uint8_t> vBlockBody;
            std::vector<uint8_t> vOffsets;
            std::vector<uint8_t> vSignature;
            uint512_t hashMerkle = 0;
            uint64_t nonce = 0;
            uint64_t timestamp = 0;
            uint16_t nSignatureLength = 0;
            uint32_t nChannel = 0;
            uint32_t nUnifiedHeight = 0;
        };


        /** CreateBlockForStatelessMining
         *
         *  Create wallet-signed block for stateless mining.
         *
         *  IMPORTANT: "Stateless" refers to MINER state (no blockchain required),
         *  NOT block signing. All blocks MUST be wallet-signed per Nexus consensus.
         *
         *  Architecture:
         *  - Falcon authentication: Miner session security
         *  - Wallet signing: Block consensus requirement (Autologin required)
         *
         *  Node requirements:
         *  - Must have wallet unlocked via -autologin=username:password
         *  - Miners authenticate with Falcon keys (separate from wallet)
         *  - Rewards routed to hashRewardAddress (can be miner's address)
         *
         *  @param[in] nChannel The mining channel (1 = Prime, 2 = Hash)
         *  @param[in] nExtraNonce Extra nonce for block iteration
         *  @param[in] hashRewardAddress Miner's reward recipient address
         *
         *  @return Pointer to wallet-signed block, or nullptr on failure
         *
         **/
        TritiumBlock* CreateBlockForStatelessMining(
            const uint32_t nChannel,
            const uint64_t nExtraNonce,
            const uint256_t& hashRewardAddress);


        /** ValidateMinedBlock
         *
         *  Validate a mined Tritium block prior to acceptance.
         *
         *  @param[in] block The mined block to validate
         *
         *  @return Structured validation result
         *
         **/
        BlockValidationResult ValidateMinedBlock(const TAO::Ledger::TritiumBlock& block);


        /** AcceptMinedBlock
         *
         *  Accept a mined Tritium block into the ledger.
         *
         *  @param[in] block The mined block to accept
         *
         *  @return Structured acceptance result
         *
         **/
        BlockAcceptanceResult AcceptMinedBlock(TAO::Ledger::TritiumBlock& block);


        /** ParseStatelessWorkSubmission
         *
         *  Parse stateless miner work submission payloads (merkle + nonce).
         *
         *  This helper validates and extracts Falcon wrapper data when present.
         *  For legacy payloads, it extracts merkle and nonce from the start of
         *  the payload.
         *
         *  @param[in] vData Raw submission payload
         *
         *  @return Parsed result with merkle root and nonce
         *
         **/
        ParseResult ParseStatelessWorkSubmission(const std::vector<uint8_t>& vData);


        /** ParseFalconWrappedSubmitBlock
         *
         *  Tail-parse a decrypted Falcon-wrapped full-block SUBMIT_BLOCK payload.
         *  The first 216 bytes are always the serialized Tritium block body. Any
         *  bytes between that 216-byte body and the Falcon trailer are treated as
         *  Prime offsets when nChannel == 1.
         *
         *  @param[in] vPayload Decrypted full-block payload
         *
         *  @return Structured parse result with block body, offsets, timestamp,
         *          signature length, and signature bytes
         *
         **/
        FalconWrappedSubmitBlockParseResult ParseFalconWrappedSubmitBlock(const std::vector<uint8_t>& vPayload);


        /** VerifyFalconWrappedSubmitBlock
         *
         *  Parse and verify a decrypted Falcon-wrapped full-block SUBMIT_BLOCK
         *  payload against the provided Falcon public key.
         *
         *  @param[in]  vPayload Decrypted full-block payload
         *  @param[in]  vPubKey  Session or miner Falcon public key
         *  @param[out] result   Populated parsed result on success
         *
         *  @return true if the payload parses and the Falcon signature verifies
         *
         **/
        bool VerifyFalconWrappedSubmitBlock(
            const std::vector<uint8_t>& vPayload,
            const std::vector<uint8_t>& vPubKey,
            FalconWrappedSubmitBlockParseResult& result);


        /** SubmitMinedBlockForStatelessMining
         *
         *  Canonical acceptance entrypoint for mined Tritium blocks.
         *
         *  @param[in] block The mined block to validate and accept
         *
         *  @return Structured submission result
         *
         **/
        SubmitResult SubmitMinedBlockForStatelessMining(TAO::Ledger::TritiumBlock& block);
    }
}

#endif
