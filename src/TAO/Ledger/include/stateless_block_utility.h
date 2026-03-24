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
         *  Payload grammar is now identical for both channels:
         *
         *  Prime: [block(216)][timestamp(8 LE)][sig_len(2 LE)][signature]
         *  Hash:  [block(216)][timestamp(8 LE)][sig_len(2 LE)][signature]
         *
         *  The node always derives vOffsets itself via GetOffsets(GetPrime(), vOffsets)
         *  after setting nNonce.  Miners no longer transmit vOffsets on the wire.
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


        /** BuildSolvedPrimeCandidateFromTemplate
         *
         *  Build a canonical solved Prime block candidate from the immutable stored
         *  template.  All consensus-critical fields (nVersion, hashPrevBlock,
         *  hashMerkleRoot, nChannel, nHeight, nBits, producer, ssSystem, vtx) are
         *  copied verbatim from the template.
         *
         *  nTime is preserved from the template rather than refreshed because:
         *  - For Prime: ProofHash = SK1024(nVersion..nBits), which does NOT include
         *    nTime.  The miner's solved proof is independent of nTime, so there is
         *    no reason to mutate this anchor field after template issuance.
         *  Callers that need a refreshed timestamp must call UpdateTime() separately
         *  after receiving the returned candidate.
         *
         *  vOffsets is always cleared on the returned candidate.  The node derives
         *  vOffsets deterministically after this call via:
         *    GetOffsets(pBlock->GetPrime(), pBlock->vOffsets)
         *  The miner no longer transmits vOffsets on the wire; the Prime wire format
         *  is now identical to Hash: [block(216)][timestamp(8)][sig_len(2)][signature].
         *
         *  The returned block's vchBlockSig is cleared because SignatureHash()
         *  covers nNonce and vOffsets; any prior signature is invalidated by
         *  applying the miner's nonce.  Call FinalizeWalletSignatureForSolvedBlock()
         *  to re-sign before submitting to ValidateMinedBlock() / AcceptMinedBlock().
         *
         *  @param[in] tmpl   The original wallet-signed template block
         *  @param[in] nNonce The miner-submitted solved nonce
         *
         *  @return A copy of the template with nNonce applied, vOffsets cleared,
         *          and vchBlockSig cleared
         *
         **/
        TritiumBlock BuildSolvedPrimeCandidateFromTemplate(
            const TritiumBlock& tmpl,
            uint64_t nNonce);


        /** BuildSolvedHashCandidateFromTemplate
         *
         *  Build a canonical solved Hash (channel 2) block candidate from the
         *  immutable stored template.  This is the Hash-channel counterpart to
         *  BuildSolvedPrimeCandidateFromTemplate and provides symmetric
         *  solved-candidate construction for channel 2.
         *
         *  All consensus-critical fields (nVersion, hashPrevBlock, hashMerkleRoot,
         *  nChannel, nHeight, nBits, nTime, producer, ssSystem, vtx) are copied
         *  verbatim from the template.
         *
         *  nTime is preserved from the template (not refreshed) because:
         *  - For Hash (channel 2): ProofHash = SK1024(nVersion..nNonce), which does
         *    NOT include nTime.  Preserving nTime avoids mutating anchor fields after
         *    template issuance without any proof-correctness benefit.
         *  Callers that require a fresh timestamp for network propagation may call
         *  UpdateTime() on the returned candidate after receiving it.
         *
         *  Hash-channel invariant: vOffsets is always cleared on the returned
         *  candidate, even if the incoming template or caller-supplied data carries
         *  residual Prime offset bytes.  Only Prime channel blocks carry vOffsets.
         *
         *  The returned block's vchBlockSig is cleared because SignatureHash()
         *  covers nNonce; the template's prior signature (created with nNonce=1) is
         *  invalidated by applying the miner's nonce.  Call
         *  FinalizeWalletSignatureForSolvedBlock() to re-sign before submitting to
         *  ValidateMinedBlock() / AcceptMinedBlock().
         *
         *  This function does NOT mutate the original template.
         *
         *  @param[in] tmpl   The original wallet-signed Hash-channel template block
         *  @param[in] nNonce The miner-submitted solved nonce
         *
         *  @return A copy of the template with nNonce applied, vOffsets cleared,
         *          and vchBlockSig cleared
         *
         **/
        TritiumBlock BuildSolvedHashCandidateFromTemplate(
            const TritiumBlock& tmpl,
            uint64_t nNonce);


        /** FinalizeWalletSignatureForSolvedBlock
         *
         *  Generate the canonical block signature (vchBlockSig) for a fully
         *  prepared solved TritiumBlock by unlocking the mining sigchain and
         *  signing SignatureHash() with the producer key.
         *
         *  This function must be called after nNonce and vOffsets are applied to
         *  the block, because SignatureHash() covers those fields.  Any prior
         *  vchBlockSig produced from a template with a different nNonce or vOffsets
         *  is invalid and must be replaced.
         *
         *  Design rationale:
         *  - Stateless transport differs from consensus block format: the miner's
         *    Falcon signature authenticates the stateless session payload, while
         *    the wallet signature (vchBlockSig) is the consensus-visible proof that
         *    the block was produced by an authorised Nexus sigchain.
         *  - Non-stateless peers do not need Stateless Node to verify relayed blocks
         *    because the final broadcast block is a standard canonical TritiumBlock
         *    whose vchBlockSig is verified against the public key in the producer
         *    transaction — exactly the same check performed by all network peers.
         *
         *  @param[in,out] block The solved block whose vchBlockSig will be set
         *
         *  @return true on success; false if credentials are unavailable or
         *          signature generation fails
         *
         **/
        bool FinalizeWalletSignatureForSolvedBlock(TritiumBlock& block);
    }
}

#endif
